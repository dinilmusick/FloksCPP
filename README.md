# FloksCPP: portable semantic UI foundation

C++17, standard-library-only renderer-independent component tree, typed props, theme inheritance, host snapshot boundary, validated control events/commands, snapshot wire format, editable directed node graph with history and typed ports, and a MIDI piano-roll/timeline model. The optional `<floks/frnl_bridge.hpp>` binds button activation directly to the verified `frnl::SemanticRuntime` surface; include and link against `FrnlCPP::semantic` in consumers.

Build: `cmake -S . -B build -DBUILD_TESTING=ON && cmake --build build && ctest --test-dir build --output-on-failure`.

Install: `cmake --install build --prefix prefix` then `cmake -S consumer -B consumer-build -DCMAKE_PREFIX_PATH=$PWD/prefix && cmake --build consumer-build && ./consumer-build/floks_consumer`.

No Qt, SDL, browser, React or device backend is required for the core. Applications implement `floks::Renderer` to receive immutable snapshots and revisions. The portable core is locally verified. Remote CI and merge certification must be checked against the exact GitHub head; this README is not release evidence.

## Source pins

- `Floks` `7ab84526d7d9820a14de47b0ab33b568b8367460`
- `FrnlCPP` `536a46228a8fde46fddc95447ca264fa2ed5fd6b`
- `GutCPP` `443ad9397c081dd596e784c50cff14c1ecad95c6`
- `FloksCPP` initial `main` `fa11f217b3590502b5022ebbe11d8a4a89cb0dee`

## Remaining parity gaps

Core includes reusable semantic control types, not native visual rendering. Reconciliation against every canonical Floks export (including service/auth UI) is incomplete. UI accessibility rich contracts, full graph JSON import/export and exhaustive upstream semantic parity remain future work. MSVC, real installed FrnlCPP bridge, remote CI and post-merge main verification require evidence from the exact published commit. Repository writes require fenced ATAX task ownership and a separate atomic repository-write lease.

Additional API: `<floks/controls.hpp>` for validated interactive control semantics; `<floks/serialization.hpp>` for graph and MIDI state roundtrip. Native integration and parity inventory: `docs/PARITY.md`. GitHub matrix: `.github/workflows/native.yml` (requires publishing after acquiring a valid repository-wide exclusive lease).
