# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build System

This is a C++17 project using CMake with SDL3 and SDL3_image (via pkg-config). The build directory is pre-configured at `build/`.

```sh
# Configure (first time or after CMakeLists.txt changes)
cmake -B build

# Build everything
cmake --build build

# Build a specific target
cmake --build build --target ambassador
cmake --build build --target dambassador

# Run the game (requires a .damb file)
./build/test/ambassador build/sandbox.damb

# Run the DAMB tool
./build/test/dambassador
```

There are no automated tests. Smoke-test by running the executables against `build/sandbox.damb`.

## Two Executables

- **`ambassador`** — the game runtime (`src/main.cxx` entrypoint, SDL3 callback model)
- **`dambassador`** — the DAMB asset packer/inspector CLI tool (`src/dambassador_main.cxx` entrypoint)

## DAMB Binary Format

DAMB (`.damb`) is the project's custom binary asset container format. Key concepts:

- **File layout**: Fixed 64-byte `Header` → TOC (array of 48-byte `TocEntry`) → chunk data
- **Chunk types**: `IMAG` (image bytes), `ATLS` (atlas tile rects), `MAPL` (map layer cells), `ENTS` (entity spawns)
- **Alignment**: All chunks are 8-byte aligned (`Align8`/`PadTo8` helpers in `damb_format.hxx`)
- **IDs**: Chunks carry `u16` IDs; ATLS references IMAG by `image_id`, MAPL references ATLS by `atlas_id`
- **Format constants** live exclusively in `src/damb_format.hxx`; binary POD structs use `static_assert(sizeof(...))` and `is_trivially_copyable` guards

The `.mamb` manifest file (text format) is consumed by `dambassador create` to pack a `.damb`.

## Code Architecture

### Static Libraries

| Library | Sources | Purpose |
|---|---|---|
| `ambcore` | `ambassador.*`, `event.cxx`, `loop.cxx`, `render.cxx` | App lifecycle, SDL3 callback integration, loop/render orchestration |
| `ambconfig` | `config.*` | Central constants (tile size, window defaults); only source of config truth |
| `ambdata` | `damb_loader*`, `runtime_*.hxx` | DAMB parsing, runtime struct construction |
| `ambutility` | `utility_binary.hxx`, `utility_parse.*`, `utility_string.*` | Binary I/O helpers, text parsing, string utilities |
| `ambgame` | `entity.*`, `entity.cxx` | Entity behavior/control abstractions |

### Runtime Object Hierarchy

```
RuntimeObject (base, runtime_object.hxx)
  └─ EntityRuntime  (hot-path: world_x/y, heading, speed, roll)
```

### Visual Layer Hierarchy

```
VisualLayer (abstract, render() pure virtual)
  ├─ MapLayer        (tile grid rendering from MapRuntime)
  ├─ SpriteLayer     (entity sprite rendering — stub)
  └─ EffectLayer     (effects — stub)
```

Layers own `ImageRuntime` (SDL_Texture RAII) and `AtlasRuntime` (source rects). `Ambassador` holds a `std::vector<VisualLayerPtr>` and iterates it each frame.

### Entity System Separation (Critical)

This project enforces **strict three-way separation**:

1. **Entity behavior** (`Entity`, `PlayerEntity` in `entity.hxx/.cxx`) — control logic, throttle/yaw/roll interface, holds `u16` scene-local ID and pointer to its `EntityRuntime`
2. **Runtime data** (`EntityRuntime` in `runtime_entity.hxx`) — hot-path AoS struct with only `world_x/y`, `heading_degrees`, `roll_degrees`, `speed`; **no identity fields**
3. **Visual layers** — render from runtime data only; never touch behavior objects

`Ambassador` owns both vectors:
- `m_entity_runtime` — `std::vector<EntityRuntime>` (scene-owned storage; must outlive wrappers)
- `m_entities` — `std::vector<EntityPtr>` (behavior wrappers; cleared before runtime storage)

### Main Loop (SDL3 Callback Model)

SDL3 calls `SDL_AppInit`, `SDL_AppIterate`, `SDL_AppEvent`, `SDL_AppQuit`. The loop targets **120 Hz fixed-step updates** with uncapped rendering. Catch-up policy: hard cap on steps per frame + drop excess accumulated time.

## Coding Conventions

See `docs/coding_standards.md` for the full contract. Key rules:

- **Naming**: `PascalCase` types, `m_` prefix for members, `lowerCamelCase` methods, `ALL_CAPS` constants, `snake_case` locals
- **Namespaces**: `amb::config`, `amb::game`, `amb::data`, `amb::damb`, `amb::runtime`, `amb::entity`
- **Headers**: `.hxx` extension, `#ifndef FILE_NAME_HXX_INCLUDED` guards, project headers before system headers
- **No gameplay logic in visual layers** — visual-layer code is visual-only
- **Hot-path data is minimal** — only update/render-critical fields in runtime structs; control flags stay in abstraction layers

## Implementation Workflow (Entity System)

The entity system is being built incrementally per `docs/entity_injection.md`. **Hard constraints**:

- Implement exactly one checklist item at a time (`docs/entity_implementation.md`)
- Every commit must leave the codebase compilable
- Do not modify `docs/*` or `data/*` without explicit request
- If behavior is not specified in `docs/entity_injection.md`, `docs/entity_architecture.md`, or `docs/entity_implementation.md`, ask before implementing

## Key Domain Facts

- **Coordinate system**: `(0,0)` top-left, +x right, +y down
- **Heading**: degrees, `0/360 = up`, clockwise positive
- **Update timebase**: milliseconds
- **Tile space** (integer grid) → **World space** (continuous float) → **Screen space** (post-camera pixels)
- This is a **top-down 2D flight simulation**, not a tile-RPG — movement model is scalar speed + heading with decoupled rotation
