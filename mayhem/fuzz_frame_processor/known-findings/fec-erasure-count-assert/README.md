# `chiaki_frame_processor_fec` aborts on `assert(erasure_index == erasures_count)` for crafted unit sequences

`chiaki_frame_processor_put_unit` increments `units_source_received`/`units_fec_received` per call
without checking that the slot for `unit_index` was already filled, so a duplicate unit (or one whose
metadata disagrees with `alloc_frame`) makes `received` exceed the number of non-empty slots.
`chiaki_frame_processor_fec` then computes `erasures_count` from the counters but counts erasures from
the slots, and the final `assert(erasure_index == erasures_count)` fires (abort; with `NDEBUG` it would
hand a wrongly sized erasure list to `jerasure`).

Reproduce: `/mayhem/fuzz_frame_processor-standalone mayhem/fuzz_frame_processor/known-findings/fec-erasure-count-assert/crash`

Upstream fix: ignore a unit whose slot already has `data_size != 0` in `put_unit`, and return an error
instead of asserting in `chiaki_frame_processor_fec`.
