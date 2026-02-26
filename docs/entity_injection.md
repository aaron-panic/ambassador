# Entity Implementation Context Injection

We are implementing the entity system in Ambassador using strict architectural boundaries and an atomic execution plan.

## Hard Constraints (Non-Negotiable)

1. **Implement exactly one checklist target at a time** using the format:
   - `Phase X, Item Y` from `src/entity_implementation.md`.
2. **Do not bundle multiple checklist items** in one implementation pass unless explicitly approved.
3. **Maintain compileability at all times**:
   - Every merge/patch step must leave the codebase in a compilable state, even if the feature is partial.
4. **No undocumented assumptions**:
   - If behavior is not explicitly defined in:
     - this file (`src/entity_injection.md`),
     - the architecture spec (`src/entity_architecture.md`),
     - or the checklist (`src/entity_implementation.md`),
     ask for clarification before implementing.
5. **Preserve strict separation of concerns**:
   - Control logic (entity abstractions),
   - State mutation/update (runtime objects + update systems),
   - Rendering (visual layers).
   These concerns must not be intermixed.
6. **Visual layers are visual only**:
   - No gameplay/control logic in visual-layer code paths.
7. **Keep hot-path runtime data minimal**:
   - Runtime object stores only data needed for update/render throughput.
   - Secondary configuration/flags belong in abstraction layers.
8. **Prefer coherent OO structure**, but allow targeted data-oriented deviations where they materially improve performance and maintainability.

---

## Reference Documents (Three-File Overview)

These four files define the implementation contract and execution process:

1. **`docs/entity_injection.md`**  
   This operational alignment document: scope, constraints, implementation protocol, and current intent.

2. **`docs/entity_architecture.md`**  
   The architecture specification: domain model, ownership boundaries, units/glossary, decision log, and timing contract.

3. **`docs/entity_implementation.md`**  
   The checklist/roadmap: concrete phased implementation tasks executed one item at a time.

3. **`docs/coding_conventions.md`**  
   An overview of coding styles and conventions used in source files for reference.
---

## Implementation Plan Summary (High-Level)

- **Stage 1:** Establish runtime + abstraction foundations for entities.
- **Stage 2:** Introduce minimal ENTS spawn semantics and loader translation.
- **Stage 3:** Build sprite-layer integration using runtime-only render inputs.
- **Stage 4:** Add deterministic input command flow and fixed-step update handling.
- **Stage 5:** Tune visibility iteration and control responsiveness.
- **Stage 6:** Extend toward richer entity state (animation/facing buckets/scanner behavior) incrementally.

---

## Current Source File Inventory (Purpose + Rationale)

> One-line purpose and rationale for each current `src/` file.

- **`src/amb_types.hxx`** — Shared primitive/type aliases to keep type usage consistent and explicit across the codebase.
- **`src/ambassador.hxx`** — Core application class declaration that coordinates bootstrap, loop phases, and layer ownership.
- **`src/ambassador.cxx`** — Core application orchestration implementation for lifecycle, loading, and high-level runtime control.
- **`src/config.hxx`** — Central configuration declarations to avoid scattered constants and enforce single-source tuning.
- **`src/config.cxx`** — Configuration definitions/values to keep policy knobs centralized and maintainable.
- **`src/damb_format.hxx`** — Shared/core DAMB definitions (file header/TOC/chunk header/constants/alignment) used across component chunk schemas.
- **`src/damb_imag.hxx`** — IMAG chunk schema/types/constants isolated into a dedicated component header for format modularity.
- **`src/damb_atls.hxx`** — ATLS chunk schema/types/constants isolated into a dedicated component header for format modularity.
- **`src/damb_mapl.hxx`** — MAPL chunk schema/types/constants isolated into a dedicated component header for format modularity.
- **`src/damb_loader.hxx`** — Loader interface for translating DAMB file content into runtime-ready structures, now wired to component chunk headers.
- **`src/damb_loader.cxx`** — Loader orchestration/shared validation and cross-chunk dependency flow (file header/TOC-level control path).
- **`src/damb_loader_imag.cxx`** — IMAG-specific loader implementation (image payload decoding/runtime texture translation).
- **`src/damb_loader_atls.cxx`** — ATLS-specific loader implementation (atlas record parsing/runtime rect construction).
- **`src/damb_loader_mapl.cxx`** — MAPL-specific loader implementation (map header/cell payload parsing/runtime map construction).
- **`src/damb_spec.hxx`** — Manifest/spec data structures used by tooling for DAMB packaging and representation.
- **`src/dambassador.hxx`** — DAMB tooling command interface declaration (create/extract/inspect workflow entry).
- **`src/dambassador.cxx`** — DAMB tooling implementation for manifest parsing and binary chunk output.
- **`src/dambassador_main.cxx`** — CLI entrypoint for DAMB tooling so tooling concerns are isolated from game runtime.
- **`src/runtime_object.hxx`** — Base runtime object interface for polymorphic runtime identity and extension points.
- **`src/runtime_map.hxx`** — Map runtime data + map-space utility math for lookup, bounds, and spawn fallback behavior.
- **`src/runtime_atlas.hxx`** — Runtime atlas data container for source rectangles used by rendering.
- **`src/runtime_image.hxx`** — Runtime image/texture ownership wrapper for renderable asset state.
- **`src/runtime_entity.hxx`** — Current entity runtime/abstraction placeholder area to be refactored into final entity architecture.
- **`src/visual_layers.hxx`** — Render-layer interfaces/implementations that compose visual passes under unified rendering flow.
- **`src/event.cxx`** — Event phase handling for app loop input/event routing.
- **`src/loop.cxx`** — Fixed-step loop pacing/update trigger behavior and timing flow.
- **`src/render.cxx`** — Render phase execution and layer traversal for frame output.
- **`src/main.cxx`** — Runtime application entrypoint for game execution.
- **`src/utility_binary.hxx`** — Binary read/write helpers to keep file-IO POD handling consistent and safe.
- **`src/utility_parse.hxx`** — Parser helper declarations for manifest/token parsing workflows.
- **`src/utility_parse.cxx`** — Parser helper implementations used by tooling/manifest ingestion.
- **`src/utility_string.hxx`** — String helper declarations for trimming/splitting/token cleanup operations.
- **`src/utility_string.cxx`** — String helper implementations to keep parsing utilities reusable and centralized.

---

## Planned New Files for Entity System (Proposed, Adjustable)

> Proposed from architecture + checklist intent; adjust as needed before execution.

### Core Entity Domain

- **`src/entity_types.hxx`** — Entity-domain enums/constants (IDs, direction bucket constants, visibility states) scoped under `amb::entity`.
- **`src/entity_runtime.hxx`** — Hot-path `EntityRuntime` structure(s) with minimal update/render state (AoS-friendly layout).
- **`src/entity.hxx`** — Base entity abstraction interface/class for control semantics acting on runtime references.
- **`src/entity_player.hxx`** — Player entity declaration deriving from base entity abstraction.
- **`src/entity_player.cxx`** — Player behavior implementation (control mutation rules, domain-specific updates).

### Runtime Storage / Scene Integration

- **`src/runtime_entity.hxx`** — Scene-owned container wrapper for entity runtime vectors, ID mapping, and stable indexing policy.
- **`src/entity_registry.hxx`** — Optional ownership/index service to map stable `u16` IDs to runtime/entity handles cleanly.

### Loader / Format Integration

- **`src/damb_ents.hxx`** — ENTS chunk decode declarations and schema translation helpers (format-facing boundary).
- **`src/damb_ents.cxx`** — ENTS parse/validate/build implementation, including spawn sanity checks against MAPL context.

### Visual Integration

- **`src/visual_sprite_layer.hxx`** — Sprite-layer declaration that consumes runtime entity views and renders without gameplay logic.
- **`src/visual_sprite_layer.cxx`** — Sprite-layer implementation with visibility-index iteration and facing-bucket sprite selection.

### Input / Update Pipeline

- **`src/input_commands.hxx`** — Event-driven command queue types and command dedupe/conflict policies (anti-mash contract).
- **`src/input_commands.cxx`** — Command queue processing/coalescing implementation.
- **`src/entity_update.hxx`** — Fixed-step entity update interface (apply commands + integrate runtime state).
- **`src/entity_update.cxx`** — Deterministic entity update implementation for 120 Hz loop integration.

### Visibility / Culling

- **`src/entity_visibility.hxx`** — Visibility/scanner query interfaces and index-buffer contract.
- **`src/entity_visibility.cxx`** — Full-scan initial implementation that rebuilds `visible_indices` with `visible_count`.

---

## Execution Protocol (Per Step)

For each implementation turn:

1. Identify exact target (`Phase X, Item Y`) from `src/entity_implementation.md`.
2. Confirm scope boundaries (in/out).
3. Implement only that item.
4. Ensure project remains compilable after the change.
5. Stop and report results before proceeding to the next checklist item.

---

## Final Alignment Reminder

This subsystem is a **top-down 2D flight control loop** with high-skill gameplay intent, not a tile-RPG movement model.  
All design decisions should prioritize control precision, deterministic update behavior, and clean architectural boundaries.
