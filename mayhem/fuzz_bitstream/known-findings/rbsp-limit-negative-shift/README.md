# `vl_vlc_limit` left-shifts a negative value (`~0L << ...`) whenever the NAL is shorter than the bit buffer

`vl_vlc_limit()` (`lib/src/vl_rbsp.h:280`), called from `vl_rbsp_init()` for every NAL the SPS and slice
parsers open, computes `vlc->buffer &= ~0L << (vlc->invalid_bits + 32)`. `~0L` is `-1`, and left-shifting a
negative signed value is undefined behaviour (UBSan `left shift of negative value -1`). It is reached when the end of the NAL (the next start code) lies inside
the buffered window (`bits_left < vl_vlc_valid_bits`), i.e. for any SPS or slice that is followed closely by another
start code (`header_h264:47`, and the equivalent `vl_rbsp_init` calls in the H.265 and slice parsers). Compilers happen to produce the intended mask,
so this is a latent UB defect, not a memory-safety one.

Reproduce: `/mayhem/fuzz_bitstream-standalone mayhem/fuzz_bitstream/known-findings/rbsp-limit-negative-shift/crash`

Upstream fix: `~UINT64_C(0) << (vlc->invalid_bits + 32)`.
