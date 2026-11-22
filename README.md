# FloksCPP: portable semantic UI foundation

C++17, standard-library-only renderer-independent component tree, typed props, theme inheritance, host snapshot boundary, validated control events/commands, versioned wire format, editable directed node graph with typed ports, multi-selection, rewire, viewport, undo/redo, and a MIDI piano-roll/timeline model with note/marker editing, tempo, signatures and loop transport. The optional `<floks/frnl_bridge.hpp>` binds button activation directly to the verified `frnl::SemanticRuntime` surface; include and link against `FrnlCPP::semantic` in consumers.

Build: `cmake -S . -B build -DBUILD_TESTING=ON && cmake --build build && ctest --test-dir build --output-on-failure`.

Install: `cmake --install build --prefix prefix` then `cmake -S consumer -B consumer-build -DCMAKE_PREFIX_PATH=$PWD/prefix && cmake --build consumer-build && ./consumer-build/floks_consumer`.

No Qt, SDL, browser, React or device backend is required for the core. Applications implement `floks::Renderer` to receive immutable snapshots and revisions. The initial native foundation merged as PR #2 at `1b30581f64471d28269949db66da4034b76d4d55` with 8/8 exact-head and post-merge CI checks green (Linux GCC/Clang Debug/Release, macOS Clang, Windows/MSVC, GCC/Clang sanitizers). Follow-up editor parity changes must independently pass exact-head and post-main checks.

## Source pins

- `Floks` `7ab84526d7d9820a14de47b0ab33b568b8367460`
- `FrnlCPP` `536a46228a8fde46fddc95447ca264fa2ed5fd6b`
- `GutCPP` `443ad9397c081dd596e784c50cff14c1ecad95c6`
- `FloksCPP` initial `main` `fa11f217b3590502b5022ebbe11d8a4a89cb0dee`

## Remaining parity gaps

Core includes reusable semantic control types, not native visual rendering. Reconciliation against every canonical Floks export (including service/auth UI) is incomplete. UI accessibility rich contracts, full graph JSON import/export and exhaustive upstream semantic parity remain future work. MSVC and real installed FrnlCPP bridge passed on PR #2 and merged main; each subsequent release must rerun identical gates. Repository writes require fenced ATAX task ownership and a separate atomic repository-write lease.

Additional API: `<floks/controls.hpp>` for validated interactive control semantics; `<floks/serialization.hpp>` for graph and MIDI state roundtrip. Native integration and parity inventory: `docs/PARITY.md`. GitHub matrix: `.github/workflows/native.yml` (requires publishing after acquiring a valid repository-wide exclusive lease).