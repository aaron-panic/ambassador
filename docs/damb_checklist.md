# DATA-AMB Runtime Refactor Checklist (OOP + Fast Path)

Goal: Implement a clean object-oriented runtime pipeline where `.damb` binary data is loaded once and transformed into compact, render-focused runtime structures. This checklist is ordered so you can finish each step before moving to the next.

---

## 0) Runtime Principles (Lock These In First)

- [ ] Keep file-format structs and runtime structs separate.
- [ ] Runtime data stores only what the render path needs.
- [ ] Loader owns translation from format (`damb_format.hxx`) to runtime (`damb_runtime.hxx`).
- [ ] Keep polymorphism at the layer/object boundary, not per tile in the hot loop.
- [ ] Map cells store atlas record indices (compact handle), not pointers.
- [ ] Render loop iterates visible tile ranges only.

---

## 1) File-by-File Implementation Order (Do Not Skip Around)

1. [ ] `src/damb_runtime.hxx` (define runtime containers first)
2. [ ] `src/renderable.hxx` (confirm/adjust base render interface)
3. [ ] `src/visual_layers.hxx` (define OOP layer hierarchy + map layer API)
4. [ ] `src/damb_loader.hxx` and/or `src/damb_loader.cxx` (translate format → runtime)
5. [ ] Ambassador app integration file(s) (hold layers, camera, call render pipeline)

---

## 2) `src/damb_runtime.hxx` — Runtime Struct Definitions

Define only fast-path data containers.

### 2.1 Core runtime structs

- [x] `struct ImageRuntime`
  - [x] Data members:
    - [x] `SDL_Texture* texture`

- [x] `struct AtlasRuntime`
  - [x] Data members:
    - [x] `std::vector<damb::AtlasRecord> records`
  - [x] Notes:
    - [x] No image reference here (image is held by layer runtime context)

- [x] `struct MapCellRuntime`
  - [x] Data members:
    - [x] atlas record handle/index (recommended compact integer)
  - [x] Notes:
    - [x] No per-cell pointers

- [x] `struct MapRuntime`
  - [x] Data members:
    - [x] `u32 width`
    - [x] `u32 height`
    - [x] cell size (tile width/height or one scalar if square)
    - [x] `std::vector<MapCellRuntime> cells` (row-major flattened)

### 2.2 Optional helpers (runtime-only convenience)

- [ ] Add lightweight utility methods (bounds/index helpers) only if they are inline and trivial.
- [ ] Avoid putting loader validation logic here.

Exit criteria:

- [ ] Runtime structs are compile-only containers with no file-format baggage.

---

## 3) `src/renderable.hxx` — Base Interface

### 3.1 Base object contract

- [ ] Keep `class Renderable` with virtual render entry.
- [ ] Confirm render signature supports renderer access (camera/viewport context can be passed directly or supplied by layer state).

### 3.2 Performance guardrail

- [ ] Ensure this interface is used for layer-level dispatch, not one virtual call per map tile.

Exit criteria:

- [ ] Base class is stable and minimal.

---

## 4) `src/visual_layers.hxx` — OOP Layer Hierarchy

Define class responsibilities before implementation.

### 4.1 `class VisualLayer : public Renderable`

- [ ] Data members:
  - [ ] common layer controls (e.g., visibility, z/order if needed)
- [ ] Methods:
  - [ ] virtual `render(...)` override contract

### 4.2 `class MapLayer : public VisualLayer`

- [ ] Data members:
  - [ ] `ImageRuntime` (or handle/ref to one)
  - [ ] `AtlasRuntime`
  - [ ] `MapRuntime`
- [ ] Methods (map-specific):
  - [ ] `render(...)`
  - [ ] helper to compute visible tile bounds from camera + viewport
  - [ ] helper to convert tile/world position to destination rect

### 4.3 Stubs for future layers

- [ ] `class SpriteLayer : public VisualLayer`
- [ ] `class EffectLayer : public VisualLayer`
- [ ] Keep them minimal now (no accidental feature creep)

Exit criteria:

- [ ] MapLayer holds all map render runtime data in one place.

---

## 5) Loader Translation Plan (`src/damb_loader.hxx/.cxx`)

Implement this strictly as format-to-runtime conversion.

### 5.1 Load sequence

- [ ] Read/validate file header and TOC from `damb_format.hxx` definitions.
- [ ] Parse chunks in order.
- [ ] Build temporary decoded chunk state as needed.
- [ ] Emit final runtime structs for layers.
- [ ] Discard raw buffers/chunk temporaries immediately after conversion.

### 5.2 Chunk conversion targets

- [ ] `IMAG` → `ImageRuntime.texture`
- [ ] `ATLS` header + records → `AtlasRuntime.records`
- [ ] `MAPL` header + cells → `MapRuntime`:
  - [ ] width/height
  - [ ] cell size
  - [ ] flattened map cells (atlas indices)

### 5.3 Validation at load time

- [ ] Validate map cell atlas index range against atlas record count.
- [ ] Validate map cell vector size equals `width * height`.
- [ ] Reject unsupported map encodings for now.

Exit criteria:

- [ ] Loader returns runtime-ready layer objects with no format structs required during render.

---

## 6) Ambassador Integration Order (App-Level)

### 6.1 Data ownership

- [ ] App holds `std::vector<std::unique_ptr<VisualLayer>>` (or equivalent owning container).
- [ ] Loader output is inserted as concrete `MapLayer` instances.

### 6.2 Render pipeline wiring

- [ ] App `render()` loops over layers in order.
- [ ] For each layer, call `layer->render(...)` with renderer and camera/viewport context.

### 6.3 Camera + viewport state

- [ ] App owns camera position and viewport size.
- [ ] App updates these before layer render pass.

Exit criteria:

- [ ] Single app-level loop dispatches all layers through shared interface.

---

## 7) MapLayer Render Algorithm (Hot Path Checklist)

Implement in this exact order:

1. [ ] Compute visible world rectangle from camera + viewport.
2. [ ] Compute min/max tile coordinates for visible rectangle.
3. [ ] Clamp tile range to map bounds.
4. [ ] Iterate rows then columns across visible range.
5. [ ] For each tile:
   - [ ] Read map cell atlas index from flattened vector.
   - [ ] Resolve source rect from `AtlasRuntime.records[index]`.
   - [ ] Compute destination rect from tile position + camera.
   - [ ] Issue `SDL_RenderTexture`.

Performance notes:

- [ ] Keep row-major access contiguous.
- [ ] Avoid allocations and virtual dispatch inside tile loop.
- [ ] Avoid pointer chasing for per-cell data.

Exit criteria:

- [ ] Only visible tiles render, using one atlas texture source.

---

## 8) Sanity/Validation Checklist

- [ ] Map with known pattern draws correctly.
- [ ] Camera movement updates visible tile region correctly.
- [ ] Viewport resize does not break bounds/clamping.
- [ ] No out-of-range atlas access when map data is valid.
- [ ] Invalid `.damb` map cell indices fail at load time (not render time).

---

## 9) Final “Done” Definition

- [ ] `damb_runtime.hxx` contains finalized minimal runtime structs.
- [ ] `VisualLayer` OOP hierarchy is in place with `MapLayer` implemented.
- [ ] Loader fully translates format chunks to runtime containers.
- [ ] App render path is layer-driven and camera-aware.
- [ ] Map rendering is visible-bounds-only and uses flattened cell iteration.