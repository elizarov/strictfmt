# Break Solver Performance Research

## Goal

Make release-build `strictfmt -n -r external/userver` and `strictfmt -n -r external/pfr` each at least twice as fast under the current break-selection model, without changing formatted output.

## Method

- Build with `scripts/build.sh` (`Release`, `-O3`, `NDEBUG`).
- Benchmark the exact recursive dry-run commands with default hardware concurrency after one warm-up run.
- Record several wall-clock samples for each corpus and compare medians on the same machine and checkout.
- Use single-worker runs and sampling profiles to locate solver work without parallel scheduling noise.
- Validate every accepted optimization with the golden suite and byte-for-byte recursive output checks where applicable.

## Trials

### Baseline

Release build at commit `6e9b986`, on a 14-logical-CPU machine. One warm-up preceded five measured runs.

| Corpus | Exact-command samples | Median | 2x target |
| --- | --- | ---: | ---: |
| `external/userver` | 1.685s, 1.695s, 1.735s, 1.689s, 1.592s | 1.689s | 0.844s |
| `external/pfr` | 3.040s, 2.997s, 3.030s, 2.938s, 2.992s | 2.997s | 1.498s |

Single-worker diagnostic runs took 10.566s for userver and 3.185s for pfr. PFR is dominated by `include/boost/pfr/detail/core17_generated.hpp` (3.022s of a 3.297s verbose run). Userver has distributed cost; its slowest single files were 136ms each, so both pathological search and broad per-segment overhead matter.
