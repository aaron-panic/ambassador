# Roadmap to Entities

---

- [ ] **Phase 1 — Runtime/Interface Foundations**

  - [x] 1. Define `EntityRuntime` as hot runtime data
        - Position as floats (`world_x`, `world_y`).
        - Direction as float angle (degrees or radians; pick one and document).
        - Velocity as floats (at minimum scalar speed; optional vx/vy later).
        - Derive from `RuntimeObject` for consistent runtime polymorphism identity.

  - [x] 2. Define entity behavior interface contract
        - Require behavior-side methods (e.g. `turn`, `setThrottle`, `integrate`).
        - Require access to runtime object (`runtime()` accessor).
        - Keep interface focused on control/simulation operations, not rendering calls.

  - [x] 3. Implement base `Entity` class
        - Own pointer/reference to `EntityRuntime`.
        - Implement shared movement helpers:
          - apply turn input
          - apply velocity/throttle input
          - update position from direction + velocity + dt
        - Keep fast-changing data in runtime object; keep configuration/flags in entity class.

  - [x] 4. Implement `PlayerEntity` derived class
        - No extra fields initially.
        - Just concrete type + hooks for future control complexity.

  - [ ] 5. Define ownership/lifetime policy
        - Runtime objects allocated by scene/entity system (not visual layer).
        - Entity wrappers reference runtime objects and never outlive them.
        - Document destruction order and pointer validity expectations.

---

- [ ] **Phase 2 — ENTS Format (Minimal Spawn First)**

  - [ ] 6. Add minimal ENTS chunk spec for initial spawn use-case
        - ENTS record includes:
          - entity type (player for now)
          - map reference (`map_id`) for validation
          - spawn tile (`tile_x`, `tile_y`)
        - Mark map reference as validation-only (discard after load if not needed).

  - [ ] 7. Define strict ENTS ordering/validation rules
        - ENTS must be loaded after MAPL (or dependency-resolved equivalently).
        - Validate spawn tile is in map bounds.
        - Fail fast on invalid map reference/spawn coordinates.

  - [ ] 8. Decide initial facing encoding
        - Start with continuous angle in runtime.
        - Add optional discrete facing bucket index for sprite selection.
        - Keep hitbox/schema extensions explicitly deferred.

  - [ ] 9. Plan sprite-state extensibility in ENTS/atlas linkage
        - Do not fully implement animation now.
        - Reserve model for:
          - state (idle/thrust/etc)
          - directional frame sets
          - frame index/time
        - Keep v1 ENTS minimal to avoid overfitting too early.

---

- [ ] **Phase 3 — Loader to Runtime Translation**

  - [ ] 10. Parse ENTS records and construct runtime objects
         - Pre-allocate runtime container for all entities in scene.
         - Create corresponding `EntityRuntime` entries.
         - Apply spawn conversion tile -> world position.

  - [ ] 11. Instantiate entity behavior objects
         - Build `PlayerEntity` wrapper bound to player runtime object.
         - Store behavior objects separately from runtime array.

  - [ ] 12. Enforce spawn invariants for player
         - Exactly one player spawn for initial implementation.
         - Clear loader errors for zero/multiple player spawns.

  - [ ] 13. Add loader smoke cases
         - Valid ENTS + MAPL.
         - ENTS references missing map.
         - Out-of-bounds spawn.
         - Duplicate/missing player spawn.

---

- [ ] **Phase 4 — Sprite Layer and Layer Ordering**

  - [ ] 14. Implement concrete entity `SpriteLayer`
         - Input: span/view over `EntityRuntime` objects.
         - Render only from runtime fields (position/facing/state refs), no entity logic access.

  - [ ] 15. Introduce deterministic layer ordering policy
         - Sort by layer type + z/order key rather than load-order assumptions.
         - Support multiple map layers before sprite layers.

  - [ ] 16. Integrate sprite layer creation into Ambassador load path
         - Construct map layers and sprite layers.
         - Insert into `m_layers` in deterministic render order.

  - [ ] 17. Reserve capacity pragmatically
         - Reserve `m_layers` with expected count when known.
         - Treat this as minor optimization, not a blocker.

---

- [ ] **Phase 5 — Visibility, Scanner Bands, and Iteration Efficiency**

  - [ ] 18. Define visibility bands (gameplay-facing)
         - `visible` band: fully rendered entities.
         - `scanner` band: alternate representation/rules.
         - outside scanner: skipped for rendering, may still simulate.

  - [ ] 19. Build per-frame visible index lists
         - `visible_indices` (dense vector of runtime indices).
         - `scanner_indices` (optional second dense vector).
         - Render loops iterate these lists directly (no full scan in render pass).

  - [ ] 20. Add broadphase for culling candidate entities
         - Start with simple spatial grid buckets.
         - Query buckets intersecting view/scanner range.
         - Avoid O(N) full-scene visibility checks at scale.

  - [ ] 21. Add scanner/scramble hooks
         - Runtime flags/strength fields for scramble/cloak behavior.
         - Scanner render path consumes these to alter representation.

  - [ ] 22. Profile before micro-optimizing
         - Measure full scan vs indexed render lists.
         - Only add more complex skip-structures if profiling proves need.

---

- [ ] **Phase 6 — Controls, Input Sampling, and Fixed-Step Sync**

  - [ ] 23. Define input command structure
         - Convert raw input events to frame/update commands.
         - Queue commands for fixed-step consumption.

  - [ ] 24. Implement player command application in update step
         - Commands mutate `PlayerEntity` interface.
         - Entity interface mutates runtime state deterministically.

  - [ ] 25. Finalize fixed timestep policy
         - Decide update Hz target.
         - Define catch-up cap to prevent spiral-of-death.
         - Keep control response deterministic under variable frame render time.

  - [ ] 26. Add responsiveness tuning pass
         - Turn rate curves.
         - Throttle response curves.
         - Input buffering/deadzone policies (future joystick integration path).

---

- [ ] **Phase 7 — Validation and Milestones**

  - [ ] 27. End-to-end spawn validation
         - ENTS player spawn appears at correct world position.
         - Player control updates runtime; sprite reflects updates.

  - [ ] 28. Rendering correctness validation
         - Multiple map layers + sprite layer order verified.
         - Facing-driven sprite selection validated for basic cases.

  - [ ] 29. Performance validation
         - Verify render pass iterates only `visible_indices`/`scanner_indices`.
         - Confirm no unnecessary reallocations in steady-state gameplay loops.

---

# Milestone Definition – “Controllable Spawned Player”

Milestone is complete when:
- ENTS defines exactly one valid player spawn.
- Loader creates entity runtime + player behavior object successfully.
- Sprite layer renders player from runtime-only data path.
- Fixed-step input/update loop yields responsive, deterministic control.
- Visibility/scanner iteration path is data-driven and profile-verified.
