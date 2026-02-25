# Roadmap to Sandbox

---

- [x] **Phase 1 — Build & Structural Blockers (Must Fix First)**

  - [x] 1. Fix build reproducibility
        - Remove or restore missing CMake source references:
          - `src/scenedata.cxx`
          - `src/dambassador.cxx`
        - Ensure SDL3 pkg-config / find_package works on clean system.
        - Verify clean clone → configure → build works without manual tweaks.

  - [x] 2. Fix ODR / linkage correctness
        - `MapRuntime` methods currently marked `inline` in header but defined in `.cxx`.
        - Either:
          - Move definitions into header (true inline), or
          - Remove `inline` and keep normal out-of-line definitions.

  - [x] 3. Define resource lifetime policy
        - Clearly define ownership of:
          - `SDL_Renderer`
          - `SDL_Texture`
        - Ensure `ImageRuntime.texture` cleanup is deterministic.
        - Document lifetime relationship: renderer outlives textures.

---

- [x] **Phase 2 — Format-Level Correctness & Validation**

  - [x] 4. Define spawn-point representation (format-level)
        - Spawn needs to live in ENTS layer not yet implemented
        - Defer implementation for now.
        - Create fallback spawn point of map centre.

  - [x] 5. Enforce spawn invariant in loader (DEFERRED)
        - Validate **exactly one spawn cell** exists.
        - Fail fast with clear error message if:
          - Zero spawn cells.
          - More than one spawn cell.

  - [x] 6. Atlas ↔ Map validation contract
        - During MAPL load:
          - Validate each cell’s `atlas_record_index` < atlas record count.
        - Fail fast on invalid indices.

  - [x] 7. Single source of truth for tile size
        - Eliminate drift between:
          - `amb::game::TILE_SIZE`
          - Map-layer tile dimensions
        - Make tile size come from:
          - Either format metadata, or
          - Central constant used everywhere.

---

- [x] **Phase 3 — Minimal Tooling for Iteration**

  - [x] 8. Implement minimal `.damb` packer tool
        - Small script or utility that:
          - Writes valid header
          - Writes chunk table (TOC)
          - Applies correct alignment
          - Applies CRC / flags policy (even if minimal)
        - Support:
          - IMAG
          - ATLS
          - MAPL
        - Produce `sandbox.damb`.

  - [x] 9. Create known-pattern test map
        - Tiny map (e.g., 8×8 or 16×16).
        - Obvious tile pattern:
          - Borders
          - Diagonal
          - Repeating atlas indices
        - Used to detect:
          - Axis flips
          - Off-by-one errors
          - Atlas index bugs

---

- [x] **Phase 4 — Complete Loader → Runtime Translation Chain**

  - [x] 10. Load IMAG chunk
         - Decode image.
         - Create `SDL_Texture`.
         - Store in `ImageRuntime`.

  - [x] 11. Load ATLS chunk
         - Parse atlas records.
         - Store source rects in `AtlasRuntime`.

  - [x] 12. Load MAPL chunk
         - Parse dimensions.
         - Validate:
           - Spawn invariant.
           - Atlas index bounds.
         - Build `MapRuntime`.

  - [x] 13. Return fully populated runtime objects
         - No placeholder empty structs.
         - Loader returns usable runtime graph.
         - Refactor into coherent file structure, object oriented design

---

- [ ] **Phase 5 — Application Wiring**

  - [ ] 14. Add layer ownership to Ambassador
         - `std::vector<VisualLayerPtr>` in `Ambassador`.
         - Load map layer during init/startup.
         - Store layer instance.

  - [ ] 15. Render pipeline iteration
         - In `render()`:
           - Iterate layers.
           - Call each layer’s `render()`.

  - [ ] 16. Separate Ambassador bootstrap from runtime orchestration
         - Move initialization/bootstrap concerns out of `Ambassador` where practical.
         - Isolate viewport/runtime state orchestration for cleaner lifecycle boundaries.
         - Remove temporary debug I/O from core application object.

---

- [ ] **Phase 6 — Rendering Visible Map**

  - [ ] 17. Implement `MapLayer::render()`
         - Compute visible tile bounds from:
           - Camera position
           - Viewport dimensions
           - Tile size
         - Render only visible tiles.
         - Do not draw entire map each frame.

  - [ ] 18. Viewport resize handling
         - On resize event:
           - Recalculate visible grid dimensions.
           - Update any cached viewport math.

---

- [ ] **Phase 7 — Camera & Controls**

  - [ ] 19. Add camera state
         - Position (world-space).
         - Optional velocity.

  - [ ] 20. Keyboard input handling
         - WASD / arrow keys.
         - Modify camera position per frame.

  - [ ] 21. Use spawn cell as initial origin
         - On load:
           - Convert spawn tile → world position.
           - Initialize camera/player position.

---

- [ ] **Phase 8 — Smoke Tests & Failure Modes**

  - [ ] 22. Good file renders
         - App launches.
         - Loads `sandbox.damb`.
         - Map appears.

  - [ ] 23. Camera pans
         - Keyboard input moves view.

  - [ ] 24. Invalid files fail fast
         - Missing spawn.
         - Duplicate spawn.
         - Invalid atlas index.
         - Corrupt chunk offsets.

  - [ ] 25. Render loop confirmed efficient
         - Only visible tiles drawn.
         - No full-map iteration.

---

# Milestone Definition – “Sandbox Playable”

Milestone is complete when:

- App builds from clean clone.
- `sandbox.damb` loads successfully.
- Map renders on screen.
- Camera pans with keyboard.
- Exactly one spawn cell is validated and used.
- Invalid files fail clearly and immediately.
- Only visible tiles are rendered.