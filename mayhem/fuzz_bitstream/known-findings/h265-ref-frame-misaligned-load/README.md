# `slice_set_reference_frame_h265` reads the bit buffer through a misaligned `uint32_t *`

`slice_set_reference_frame_h265()` (`lib/src/bitstream.c:351`) casts `rbsp.nal.data` (a byte pointer into
the slice buffer) to `uint32_t *` and reads `d[-2]`/`d[-1]`, which is only valid if the slice data happens
to be 4-byte aligned (UBSan `load of misaligned address ... requires 4 byte alignment`); it then writes the
modified words back the same way. The slice buffer comes from the network, so the alignment is arbitrary.
Harmless on x86, undefined behaviour in C and faulting on strict-alignment targets.

Reproduce: `/mayhem/fuzz_bitstream-standalone mayhem/fuzz_bitstream/known-findings/h265-ref-frame-misaligned-load/crash`

Upstream fix: `memcpy` the two words in and out instead of dereferencing a cast pointer.
