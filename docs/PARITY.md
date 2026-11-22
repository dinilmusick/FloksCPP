# FloksCPP native semantic parity inventory

Pinned canonical Floks `7ab84526d7d9820a14de47b0ab33b568b8367460`; FrnlCPP `536a46228a8fde46fddc95447ca264fa2ed5fd6b`; GutCPP `443ad9397c081dd596e784c50cff14c1ecad95c6`.

## Functional native counterparts

| Canonical Floks area | Native counterpart | Status / qualification |
|---|---|---|
| `packages/ui/src/components/base/BaseButton.tsx` | `Button`, `Controls::activate_button` | Native semantic operation, tests; no browser presentation |
| `BaseInput.tsx` | `TextInput`, `Controls::text_input` | Native input changes and events |
| `BaseToggle.tsx` | `Toggle`, `Controls::toggle` | State changes and stale-revision rejection |
| `BaseCard.tsx`, `layout/*` | `Panel`, `Tree`, `Layout`, `Renderer` | Renderer-neutral tree, geometry descriptors, renderer host |
| `packages/ui/src/components/tiles/TilesMenu.tsx` | `TileMenu` typed component contract | No full tile navigation or visual parity |
| `packages/canvas-ui/src/frnl-editable-node-graph/FrnlEditableNodeGraph.tsx` | `Graph`, `Node`, `Port`, `Link` | Typed connection validation, cycle rejection, selection/viewport, undo/redo, wire roundtrip |
| `packages/ui/src/components/timeline/MidiTimeline.tsx` | `PianoRoll` | 128 MIDI pitches, note editing, velocity, quantization, loop/playhead, wire roundtrip |
| `packages/ui/src/components/timeline/MusicalTimeline.tsx` | `PianoRoll::markers`, loop, transport | Marker CRUD and beat timeline, no time-signature metadata |
| FrnlTS / FrnlCPP event semantics | `Tree::dispatch`, `Tree::execute`, `FrnlBridge` | Independent event snapshot, reentrancy guard and revision fence; optional real Frnl bridge requires installed dependency |

## Native scope exclusions and known gaps

Canonical Floks remains the authoritative React component implementation; FloksCPP ports functional semantic behavior, not DOM, CSS, or React. Graph socket morphism, graph subgroups, advanced snapping, multimodal graph interaction, native text editing/accessibility hooks, rich audio waveform rendering, score notation, marker drag operations, and exact parity for every Floks UI/auth/service/widget export are **not implemented**. The native snapshot wire format is versioned but not a general-purpose JSON interchange. Host UI event loops, persistence media, concurrency control and native graphics backends are supplied externally.

## Verification state

Local GCC/Clang compilation, strict warnings, Debug/Release tests, native ASan/UBSan and a separate installed-core consumer can be executed offline. Installed real FrnlCPP bridge, macOS/MSVC and GitHub exact-head CI are only defined in workflow and must run before claiming certification. Remote release requires canonical fenced task ownership, a repository-wide write lease, exact-head CI and independent main verification.
