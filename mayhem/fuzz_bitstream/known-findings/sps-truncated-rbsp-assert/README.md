# The RBSP reader asserts instead of failing when a NAL is truncated (`vl_vlc_valid_bits(vlc) >= num_bits`)

The H.264/H.265 header and slice parsers (`lib/src/bitstream.c`, on top of `lib/src/vl_rbsp.h`) never check
how many bits are left before reading. On a short or malformed NAL one of two asserts fires, both with the
same condition `vl_vlc_valid_bits(vlc) >= num_bits` and the same root cause (no end-of-data handling in the
reader; Mayhem reports 4 defects for them because the assert sites differ):

- `vl_vlc_eatbits` (`vl_rbsp.h:168`), via `skip_startcode()` on a NAL shorter than the 4-byte start code
  (`header_h264:30`, `header_h265:89`, `slice_h264:160`, `slice_h265:232`);
- `vl_vlc_get_uimsbf` (`vl_rbsp.h:182`), via `vl_rbsp_ue()`/`vl_rbsp_u()` running past the end of the NAL
  (`header_h265:132/138/144`, `slice_h265:255/275`).

The header bytes come from the console's stream-info packet and the slice bytes from video packets, i.e.
from the remote peer. The assert is compiled in (never `-DNDEBUG`), so this is a remotely triggerable abort.

Reproduce: `/mayhem/fuzz_bitstream-standalone mayhem/fuzz_bitstream/known-findings/sps-truncated-rbsp-assert/crash`

Upstream fix: make the reader return an error (and the parsers return false) when fewer than `num_bits`
bits remain, instead of asserting.
