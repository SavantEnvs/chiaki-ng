# syntax=docker/dockerfile:1.7
# Commit image for chiaki-ng: the org base plus apt -dev libraries and the git-submodule
# dependencies (nanopb, jerasure, gf-complete, munit), which are empty directories in a fork
# checkout, cached under /opt/toolchains/deps from pinned commits.
FROM ghcr.io/savantenvs/base:20261023

ARG COMMIT
ARG REPO_URL
ARG SOURCE_DATE_EPOCH
ARG SANITIZER_FLAGS
ARG COVERAGE_FLAGS=
ARG MAYHEM_JOBS=

USER root
RUN [ -n "$SOURCE_DATE_EPOCH" ] && repro-sources-list.sh || true ; \
    apt-get update && apt-get install -yq --no-install-recommends \
        ca-certificates curl pkg-config protobuf-compiler python3-protobuf \
        libssl-dev libcurl4-openssl-dev libjson-c-dev libminiupnpc-dev libevent-dev libopus-dev \
 && rm -rf /var/lib/apt/lists/*

# Static, uninstrumented, DWARF-3 builds of the bundled third-party code (SPEC 6.2 item 19).
ARG GFC_VER=fa54a4670a56
ARG GFC_REV=fa54a4670a5705c84abf6c24b92b0cd479625478
ARG GFC_SHA256=32937a9e949392b149e4a025901f5104d4a7ec9827887d6f4e1e70d583688f6f
ARG JER_VER=505ccb4bc69e
ARG JER_REV=505ccb4bc69eee8d7ea40a6089a056b99671134f
ARG JER_SHA256=d9526bf52f338ee78ba6ba733fafc0321719344debd32e0746f437dd56a4fe43
ARG NPB_VER=160d4f09e5fa
ARG NPB_REV=160d4f09e5fabb2b66aa2dea32d4f38ace2c4b3f
ARG NPB_SHA256=3916241f10ae420ad1504246cfb98ab26c41a877e0c3a2f5dde6a99f610942e1
ARG MUN_VER=cfc3717cc0cf
ARG MUN_REV=cfc3717cc0cff59ea1e88d5ea13e93621157f3cb
ARG MUN_SHA256=8d3645004bb51137cb29e6ae3fdf2aef6cfdaf75263a6bebbc693f37a6193a75
RUN set -eux; \
    fetch() { \
      d=/opt/toolchains/deps/$1; mkdir -p "$d/src" "$d/lib" "$d/include"; \
      curl -fsSL -o /tmp/dep.tgz "$2"; echo "$3  /tmp/dep.tgz" | sha256sum -c -; \
      tar -xzf /tmp/dep.tgz --strip-components=1 -C "$d/src"; rm -f /tmp/dep.tgz; }; \
    CF="-O2 -g -gdwarf-3"; \
    # gf-complete
    d=/opt/toolchains/deps/gf-complete-${GFC_VER}; \
    fetch gf-complete-${GFC_VER} https://codeload.github.com/streetpea/gf-complete/tar.gz/${GFC_REV} ${GFC_SHA256}; \
    mkdir /tmp/b && cd /tmp/b; \
    for f in gf gf_wgen gf_w4 gf_w8 gf_w16 gf_w32 gf_w64 gf_w128 gf_rand gf_general gf_cpu; do \
      clang $CF -I$d/src/include -c $d/src/src/$f.c -o $f.o; done; \
    ar rcs $d/lib/libgf_complete.a *.o; cp $d/src/include/*.h $d/include/; cd /; rm -rf /tmp/b; \
    # jerasure
    d=/opt/toolchains/deps/jerasure-${JER_VER}; \
    fetch jerasure-${JER_VER} https://codeload.github.com/streetpea/jerasure/tar.gz/${JER_REV} ${JER_SHA256}; \
    mkdir /tmp/b && cd /tmp/b; \
    for f in galois jerasure reed_sol cauchy liberation; do \
      clang $CF -I$d/src/include -I/opt/toolchains/deps/gf-complete-${GFC_VER}/include -c $d/src/src/$f.c -o $f.o; done; \
    ar rcs $d/lib/libjerasure.a *.o; cp $d/src/include/*.h $d/include/; cd /; rm -rf /tmp/b; \
    # nanopb: runtime library + generator (nanopb_pb2.py pre-generated so the generator never writes into deps/)
    d=/opt/toolchains/deps/nanopb-${NPB_VER}; \
    fetch nanopb-${NPB_VER} https://codeload.github.com/nanopb/nanopb/tar.gz/${NPB_REV} ${NPB_SHA256}; \
    mkdir /tmp/b && cd /tmp/b; \
    for f in pb_common pb_decode pb_encode; do clang $CF -DPB_C99_STATIC_ASSERT -I$d/src -c $d/src/$f.c -o $f.o; done; \
    ar rcs $d/lib/libprotobuf-nanopb.a *.o; cp $d/src/pb.h $d/src/pb_common.h $d/src/pb_decode.h $d/src/pb_encode.h $d/include/; \
    cd $d/src/generator/proto && protoc --python_out=. -I. nanopb.proto; cd /; rm -rf /tmp/b; \
    # munit (test-suite framework, compiled by build.sh)
    fetch munit-${MUN_VER} https://codeload.github.com/streetpea/munit/tar.gz/${MUN_REV} ${MUN_SHA256}; \
    chmod -R a+rX /opt/toolchains/deps
USER mayhem

USER mayhem
WORKDIR /mayhem
COPY --chown=2000:2000 . /mayhem

RUN if [ -x /opt/mayhem/ccache/prefill ]; then /opt/mayhem/ccache/prefill bash mayhem/build.sh; \
    else bash mayhem/build.sh; fi

RUN if [ -f mayhem/test.sh ]; then bash mayhem/test.sh; else echo "no mayhem/test.sh"; fi

LABEL org.opencontainers.image.source="${REPO_URL}" \
      org.opencontainers.image.revision="${COMMIT}" \
      rlenv.tier="commit" \
      rlenv.source_date_epoch="${SOURCE_DATE_EPOCH}" \
      rlenv.user="mayhem"

USER mayhem
WORKDIR /mayhem
