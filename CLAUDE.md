# Ambassador — CLAUDE.md

This file establishes working context, constraints, and conventions for Claude when working on the Ambassador codebase.

---

## Project Overview

Ambassador is a **top-down 2D flight control game** written in C++17, built on SDL3/SDL3-image. It is not a tile-RPG — the core gameplay intent is tight, skill-based flight with high-precision controls.

The project also ships a standalone binary packaging tool called **dambassador** for authoring `.damb` asset bundles.

---

## Build System

```bash
mkdir build && cd build
cmake ..
make
```

- Requires: `cmake 3.14+`, `pkg-config`, `SDL3`, `SDL3-image`
- Outputs to `build/test/`:
  - `ambassador <sandbox.damb>` — game executable
  - `dambassador` — asset packaging tool
- Standard: **C++17**

There is no automated test suite. Validation is done by running the executable with a test data file (`data/sandbox.mamb`).

---

## Source File Map

> See `docs/entity_injection.md` for a full per-file rationale. Short reference:
| File | Purpose |
|---|---|
| `src/ambassador.hxx/.cxx` | Core app class: bootstrap, loop, load, layer ownership |
| `src/main.cxx` | SDL3 callback entry point (`SDL_AppInit` etc.) |
| `src/config.hxx/.cxx` | Centralized constants — single source of truth for tuning |
| `src/amb_types.hxx` | Primitive type aliases (`u8`, `u16`, etc.) and SDL smart pointer deleters |
| `src/damb_format.hxx` | DAMB binary format spec (magic, TOC, chunk headers, alignment) |
| `src/damb_imag/atls/mapl.hxx` | Per-chunk schema types (isolated for format modularity) |
| `src/damb_loader.hxx/.cxx` | Loader orchestration — validates + dispatches chunk loading |
| `src/damb_loader_imag/atls/mapl.cxx` | Per-chunk loader implementations |
| `src/damb_spec.hxx` | Tooling manifest/spec structures |
| `src/dambassador.hxx/.cxx` | DAMB tool: create/extract/inspect entry points |
| `src/dambassador_main.cxx` | CLI entry point for dambassador (isolated from game) |
| `src/runtime_object.hxx` | Base `RuntimeObject` polymorphic interface |
| `src/runtime_image/atlas/map.hxx` | Runtime data containers for loaded assets |
| `src/runtime_entity.hxx` | Entity runtime + abstraction (being refactored; see entity docs) |
| `src/visual_layers.hxx` | Visual layer interfaces and `MapLayer`/`SpriteLayer` implementations |
| `src/event.cxx` | Event phase — input/event routing |
| `src/loop.cxx` | Fixed-step loop pacing and update trigger |
| `src/render.cxx` | Render phase — layer traversal |
| `src/utility_binary.hxx` | Binary read/write helpers |
| `src/utility_parse.hxx/.cxx` | Parser helpers for manifest/token workflows |
| `src/utility_string.hxx/.cxx` | String trim/split/token helpers |

---

## Architecture: Three Hard Separations

These three concerns **must never be mixed**:

1. **Control/behavior** — entity abstraction classes; own logic, flags, control rules.
2. **State mutation/update** — runtime objects + update systems; hot-path data only.
3. **Rendering** — visual layers; render from runtime data only, no gameplay logic.

Visual layers are **visual-only**. No control logic belongs in any rendering code path.

---

## Entity System (Active Work)

The entity system is under active implementation. Reference these docs in order:

1. `docs/entity_injection.md` — operational constraints and execution protocol
2. `docs/entity_architecture.md` — architecture spec, units, ADR log, timing contract
3. `docs/entity_implementation.md` — phased checklist (implement **one item at a time**)

### Entity Implementation Rules

- **One checklist item per pass** (`Phase X, Item Y`). Never bundle steps.
- **Codebase must compile after every step**, even if the feature is partial.
- **Confirm before implementing** anything not explicitly defined in the three docs above.
- Entity IDs: stable `u16` per scene.
- Runtime storage: `std::vector<EntityRuntime>` (Array of Structs, contiguous).
- Update loop: fixed-step **120 Hz** target; render is uncapped and decoupled.
- Catch-up policy: hard cap on steps per frame + drop excess accumulated time.
- Input model: event-driven command queue with **anti-mash semantics** (precision rewarded, spam is not).

### Coordinate System & Units

| Concept | Convention |
|---|---|
| Origin | `(0,0)` top-left |
| Positive directions | +x right, +y down |
| Position | `float` |
| Velocity | `float` (scalar speed v1) |
| Heading | `float` degrees; `0/360 = up`, clockwise positive |
| Facing (sprite) | Discrete bucket derived from heading on direction-change |
| Time base | Milliseconds |

---

## Coding Standards

Full reference: `docs/coding_standards.md`. Summary:

### Naming

| Construct | Convention | Example |
|---|---|---|
| Class / struct / type alias | PascalCase | `DambLoader`, `MapRuntime` |
| Member data | `m_` + snake_case | `m_window`, `m_map_runtime` |
| Functions / methods | lowerCamelCase | `loadSandbox`, `defaultSpawnPoint` |
| Constants | ALL_CAPS_WITH_UNDERSCORES | `MAP_TILE_SIZE`, `APP_TITLE` |
| Local variables | snake_case | `cell_count`, `viewport_w` |

### Namespaces

All new symbols belong in an `amb::` namespace. Entity-domain symbols use `amb::entity` (or deeper sub-namespace). Existing pattern: `amb::config`, `amb::game`, `amb::data`, `amb::damb`, `amb::runtime`.

### Headers

- Guards use `<FILE_NAME_UPPER>_INCLUDED` suffix.
- `.hxx` for declarations/type definitions/inline helpers.
- `.cxx` for implementation logic.
- Project headers first, then system/external headers.
- One clear domain concept per header.

### Class Design

- Private data + public methods by default.
- Member initializer lists for construction.
- Pure virtual interfaces for pluggable systems; `override` on derived virtuals.
- `std::unique_ptr` for unique ownership (e.g. layer pointers).

### Performance

- Hot-path runtime structs: minimal fields only. Configuration/flags stay in abstraction layers.
- No per-frame heap allocations in steady-state loops.
- Visibility iteration: preallocated index buffers, reset `visible_count` each update tick, iterate `0..visible_count-1`.

### Error Handling

- Validate external/file data aggressively — fail fast with `std::runtime_error`.
- Use SDL logging for runtime/app-level failures; messages must be specific and actionable.
- Null checks and bounds checks before any dereference.

### Formatting

- Opening brace on same line.
- Explicit braces on all conditionals/loops.
- Prefer readable multiline argument lists over compressed one-liners.
- Small trivial accessors may be inline in headers; non-trivial logic goes in `.cxx`.

### DAMB Format Structs

- Use `static_assert(sizeof(...))` and trivially-copyable checks on all binary format structs.
- Keep binary format constants in `damb_format.hxx`, not scattered.

---

## Do-Not-Modify Zones

- **`docs/*`** — Updated manually as part of feature work, never auto-modified.
- **`data/*`** — Binary data files; do not touch unless explicitly instructed.

---

## "When in Doubt" Rules

1. Match the existing file-local style before introducing a new pattern.
2. Prefer maintainability and clear intent over clever but opaque code.
3. If behavior is not specified in a prompt, spec, or checklist: **ask before implementing**.
4. Keep naming and namespace scoping explicit to reduce ambiguity.
5. Simple and correct before clever and fragile.