# `vl_rbsp_ue` shifts a 32-bit `1` by the number of leading zeros without bounding it

`vl_rbsp_ue()` (`lib/src/vl_rbsp.h:398`) counts leading zero bits of an exp-Golomb code and returns
`(1 << bits) - 1 + vl_rbsp_u(rbsp, bits)`. A code with 31 or more leading zeros (a run of zero bytes where an
SPS field is expected, `header_h265` at `bitstream.c:132` and `:137`) makes `1 << bits` a UBSan shift-exponent / signed-overflow error (`shift exponent 34 is too large for
32-bit type 'int'`, or `left shift of 1 by 31 places`), and `vl_rbsp_u(rbsp, bits)` is then asked for more
than 32 bits.

Reproduce: `/mayhem/fuzz_bitstream-standalone mayhem/fuzz_bitstream/known-findings/rbsp-ue-shift-overflow/crash`

Upstream fix: reject `bits > 31` (return an error / 0) before shifting, and use `1u`.
