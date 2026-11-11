# av_packet_parse over-reads (and underflows `data_size`) when the NALU-info / haptics prefix is not covered by the size check

`chiaki_takion_v{7,9,12}_av_packet_parse` (`lib/src/takion.c`, `av_packet_parse()`) only checks
`av_size >= av_header_size + 1`. It then skips a further 3 bytes when `uses_nalu_info_structs` is set
(`av += 3; av_size -= 3;`) and 1 byte for v12 audio (`*av == 0x02; av += 1`) without re-checking, so a
datagram that is exactly one byte past the header reads out of bounds (ASan heap-buffer-overflow at
`takion.c:1839`) and `packet->data_size` underflows to a huge `size_t`, which later callers
(`takion_handle_packet_av`, the video/audio receivers) use as a length.

Reachable from the network: `takion_handle_packet` passes every received VIDEO/AUDIO datagram to it.

The ASan crash is the v12 audio read (`takion.c:1839`, `*av == 0x02` one byte past the buffer). The NALU-info
case (`av += 3`) only underflows `data_size` inside the parser; the harnesses do not read `data[]` after a
successful parse (a consumer loop in the harness would be a second, harness-attributed defect), so that case is
visible only by code reading and through whatever the library's own consumers do with `data_size`.

Reproduce:
- `/mayhem/fuzz_av_packet-standalone mayhem/fuzz_av_packet/known-findings/av-packet-nalu-offset-overread/crash`
- `/mayhem/fuzz_takion_packet-standalone mayhem/fuzz_av_packet/known-findings/av-packet-nalu-offset-overread/crash-takion-packet`
  (a v12 audio datagram one byte past the header, inside a `takion_handle_packet` stream)

Upstream fix (one line each): require `av_size >= av_header_size + 1 + 3` when `uses_nalu_info_structs`
(and `+ 1` for v12 audio) before advancing `av`.
