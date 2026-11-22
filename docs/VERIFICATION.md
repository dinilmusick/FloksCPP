# Local verification record — 2026-11-22

## Checked source (immutable upstream GitHub references)

- Floks main: `7ab84526d7d9820a14de47b0ab33b568b8367460`
- FrnlCPP main: `536a46228a8fde46fddc95447ca264fa2ed5fd6b`
- GutCPP main: `443ad9397c081dd596e784c50cff14c1ecad95c6`
- FloksCPP main before work: `fa11f217b3590502b5022ebbe11d8a4a89cb0dee` (README only)

## Executed on local runtime

| Gate | Result |
|---|---|
| GCC 14.2, Debug CMake + strict warnings + CTest | 5/5 PASS |
| GCC 14.2, Release CMake + strict warnings + CTest | 5/5 PASS |
| Clang, Debug CMake + strict warnings + CTest | 5/5 PASS |
| Clang, Release CMake + strict warnings + CTest | 5/5 PASS |
| GCC ASan + UBSan, CTest | 5/5 PASS |
| Clang ASan + UBSan, CTest | 5/5 PASS |
| Installed-package external consumer, separate CMake project | 1/1 PASS |

All local results apply to the candidate package, **not** the GitHub `main` repository. The external consumer did not have an installed FrnlCPP in the local environment, so it exercised `FloksCPP::core` only. There is an optional `FrnlBridge` using the verified upstream `frnl::SemanticRuntime` signature. Repository CI is configured to install FrnlCPP at the immutable revision and build the actual integration tests, but the workflow has not been uploaded or executed.

## Open hard blockers

1. ATAX has atomic task claims but no independently confirmed **repository-wide exclusive write lease** enforceable across concurrent agents. No GitHub pushes, pull requests, merges or releases were made.
2. The local container cannot resolve `github.com`, so a real FrnlCPP local checkout/install could not be performed. Upstream exported interfaces were independently inspected via GitHub connector.
3. GitHub CI, Windows/MSVC, macOS, exact-head PR checks, and independent merged-main confirmation remain unverified.
4. Expanded Floks upstream export parity is incomplete. See `PARITY.md`.

Do not claim Wave 3 final acceptance until these are resolved and all missing checks pass.

## Follow-up hardening run (2026-11-22)

The new `floks_hardening_tests` checks mutation protection under event dispatch, invalid floating-point properties and themes, expanded semantic controls, MIDI marker movements, and extreme coordinate handling.

GCC and Clang Debug/Release: 6/6 CTests in each configuration. GCC and Clang AddressSanitizer/UndefinedBehaviorSanitizer: 6/6 CTests in each configuration. This gives **36 of 36 local test executions passed**, plus an independently configured installed-core consumer (1/1). An initial sanitizer build timed out under high parallelism; the same code passed when rebuilt serially.

The FrnlCPP-required CMake option `-DFLOKSCPP_REQUIRE_FRNL=ON` is set in CI, so missing upstream installation must fail configuration rather than silently skip its integration test.

These are candidate-only results; the actual installed FrnlCPP C++ consumer, Windows/MSVC, macOS, GitHub PR CI, and merged-main verification remain separate gates.
