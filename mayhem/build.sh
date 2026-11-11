#!/usr/bin/env bash
#
# mayhem/build.sh — builds chiaki-ng's C library twice (sanitized for the fuzz harnesses, normal for
# the upstream munit suite) straight from lib/'s sources, because the upstream CMake build needs the
# git submodules (empty in a fork checkout). Third-party code comes from /opt/toolchains/deps.
set -euo pipefail

[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH

: "${SANITIZER_FLAGS=-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer}"
: "${DEBUG_FLAGS:=-g -gdwarf-3}"
: "${CC:=clang}" ; : "${CXX:=clang++}" ; : "${LIB_FUZZING_ENGINE:=-fsanitize=fuzzer}"
: "${MAYHEM_JOBS:=$(nproc)}"
: "${COVERAGE_FLAGS=}"
export SANITIZER_FLAGS DEBUG_FLAGS CC CXX LIB_FUZZING_ENGINE MAYHEM_JOBS COVERAGE_FLAGS

cd "$SRC"
DEPS=/opt/toolchains/deps
NPB=$(echo "$DEPS"/nanopb-*)
JER=$(echo "$DEPS"/jerasure-*)
GFC=$(echo "$DEPS"/gf-complete-*)
MUN=$(echo "$DEPS"/munit-*)
FUZZ=$SRC/build-fuzz
TEST=$SRC/build-test
rm -rf "$FUZZ" "$TEST"
mkdir -p "$FUZZ/obj" "$FUZZ/pb" "$TEST/obj"

# takion.pb.{c,h}: protoc + the nanopb generator from the cached nanopb source (what lib/protobuf/CMakeLists.txt runs)
protoc "-o$FUZZ/pb/takion.pb" "-I$SRC/lib/protobuf" "$SRC/lib/protobuf/takion.proto"
python3 "$NPB/src/generator/nanopb_generator.py" -D "$FUZZ/pb" "$FUZZ/pb/takion.pb" >/dev/null


LIB_SRCS="common sock session thread base64 http log ctrl rpcrypt takion senkusha streamconnection ecdh
 launchspec random gkcrypt audio audioreceiver audiosender videoreceiver frameprocessor packetstats discovery
 congestioncontrol stoppipe reorderqueue discoveryservice feedback feedbacksender controller takionsendbuffer
 time fec regist opusdecoder opusencoder orientation bitstream aia aia_openssl
 remote/holepunch remote/rudp remote/rudpsendbuffer"

PKGS="libcurl openssl json-c miniupnpc libevent opus"
INC="-Wno-deprecated-declarations -I$SRC/lib/include -I$FUZZ/pb -I$NPB/include -I$JER/include -I$GFC/include -DPB_C99_STATIC_ASSERT $(pkg-config --cflags $PKGS)"
LIBS="$JER/lib/libjerasure.a $GFC/lib/libgf_complete.a $NPB/lib/libprotobuf-nanopb.a $(pkg-config --libs $PKGS) -lm -lpthread"

# compile every library source into <objdir> with the given flags, in parallel
compile_lib() { # <objdir> <flags...>
  export CF="$*" OD="$1" FUZZ SRC CC INC
  CF="${CF#"$1" }"
  { for s in $LIB_SRCS; do echo "$s"; done; echo takion.pb; } |
    xargs -P "$MAYHEM_JOBS" -I{} bash -c '
      if [ "$1" = takion.pb ]; then f=$FUZZ/pb/takion.pb.c; else f=$SRC/lib/src/$1.c; fi
      $CC $CF $INC -c "$f" -o "$OD/$(basename "$1").o"' _ {}
}

# 1) the library with the fuzz flags, plus the harnesses
# shellcheck disable=SC2086
compile_lib "$FUZZ/obj" $SANITIZER_FLAGS -fsanitize=fuzzer-no-link $DEBUG_FLAGS -O1
rm -f "$FUZZ/libchiaki.a"
ar rcs "$FUZZ/libchiaki.a" "$FUZZ"/obj/*.o

# shellcheck disable=SC2086
$CXX $SANITIZER_FLAGS -c "$SRC/mayhem/lsan_off.cc" -o "$FUZZ/lsan_off.o"
# shellcheck disable=SC2086
$CC $SANITIZER_FLAGS $DEBUG_FLAGS -c "$STANDALONE_FUZZ_MAIN" -o "$FUZZ/standalone_main.o"

for h in fuzz_http_response fuzz_discovery_response fuzz_regist_response fuzz_rudp_message \
         fuzz_av_packet fuzz_takion_packet fuzz_frame_processor fuzz_bitstream; do
  # shellcheck disable=SC2086
  $CC $SANITIZER_FLAGS -fsanitize=fuzzer-no-link $DEBUG_FLAGS -O1 $INC -I"$SRC/mayhem" \
      -c "$SRC/mayhem/$h/$h.c" -o "$FUZZ/$h.o"
  # shellcheck disable=SC2086
  $CC $SANITIZER_FLAGS $DEBUG_FLAGS $LIB_FUZZING_ENGINE "$FUZZ/$h.o" "$FUZZ/lsan_off.o" \
      "$FUZZ/libchiaki.a" $LIBS -o "$SRC/$h"
  # shellcheck disable=SC2086
  $CC $SANITIZER_FLAGS $DEBUG_FLAGS "$FUZZ/$h.o" "$FUZZ/standalone_main.o" "$FUZZ/lsan_off.o" \
      "$FUZZ/libchiaki.a" $LIBS -o "$SRC/$h-standalone"
done

# 2) upstream's munit suite (test/) against a normal-flags build of the library
# shellcheck disable=SC2086
compile_lib "$TEST/obj" -O2 $COVERAGE_FLAGS
TEST_SRCS="main http rpcrypt gkcrypt takion seqnum keystate feedback reorderqueue fec test_log bitstream regist aia"
tobjs=()
for t in $TEST_SRCS; do
  # shellcheck disable=SC2086
  $CC -O2 $COVERAGE_FLAGS $INC -I"$SRC/test" -I"$MUN/src" -c "$SRC/test/$t.c" -o "$TEST/t_$t.o"
  tobjs+=("$TEST/t_$t.o")
done
# shellcheck disable=SC2086
$CC -O2 $COVERAGE_FLAGS -c "$MUN/src/munit.c" -o "$TEST/munit.o"
# shellcheck disable=SC2086
$CC -O2 $COVERAGE_FLAGS "${tobjs[@]}" "$TEST/munit.o" "$TEST"/obj/*.o $LIBS -o "$TEST/chiaki-unit"
