# `vl_vlc_removebits` shifts a `uint64_t` by 64 when emulation-prevention bytes sit at the end of the buffer

`vl_rbsp_fillbits()` strips H.26x emulation-prevention bytes with `vl_vlc_removebits(vlc, pos, num_bits)`
(`lib/src/vl_rbsp.h:264`), which evaluates `UINT64_MAX >> (pos + num_bits)` and `UINT64_MAX << (64 - pos)`.
When the escape's last byte is the last byte of the 64-bit window, `pos + num_bits` is 64 and
`UINT64_MAX >> 64` is an out-of-range shift (UBSan `shift exponent 64 is too large for 64-bit type`). Reached from
`vl_rbsp_u` in `header_h265` (`bitstream.c:120`) with a `00 00 03` sequence in the SPS.

Reproduce: `/mayhem/fuzz_bitstream-standalone mayhem/fuzz_bitstream/known-findings/rbsp-removebits-shift-64/crash`

Upstream fix: special-case `pos + num_bits == 64` in the mask computation.
