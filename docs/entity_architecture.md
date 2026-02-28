# Entity Architecture Contract (Flight Loop Foundation)

> **Status:** Active guidance (spec + notes)  
> **Scope:** Flight/scene entity pipeline only (not RPG/adventure systems)  
> **Primary goal:** Tight, skill-based 2D flight control with strict separation of control, state update, and rendering.

---

## 1) Architecture Contract

## 1.1 Core Separation (Normative)

- **Entity classes** are behavior/control abstractions.
- **Entity runtime objects** are hot-path state containers for update/render.
- **Visual layers** are visual-only and must render from runtime data, not behavior classes.
- **Control, update, and render are separate concerns** and must not be intermixed.
- **No assumptions allowed**: if not explicitly defined in checklist/spec/prompt, confirm before implementation.

## 1.2 Ownership & Lifetime

- Scene/system owns runtime object storage.
- Entity wrappers hold references/pointers to owned runtime objects.
- Visual systems consume runtime views/references only.
- Runtime objects must outlive all wrappers/references that point to them.
- Scene teardown/destruction order must clear behavior wrappers before clearing runtime storage.

## 1.3 Data Layout & Identity

- Runtime storage strategy (v1): **Array of Structs** (`std::vector<...>`) for fast contiguous iteration.
- Entity identity (scene-local): **stable `u16` numeric ID** stored in entity behavior/wrapper objects (not in the hot runtime struct).
- Hot path policy: keep only the least amount of data needed in runtime objects.
- Extra/slow-changing control flags/settings stay in entity abstraction layers, not runtime hot structs.

## 1.4 Rendering Contract

- Entities do **not** render themselves.
- Entity behavior may expose a visibility toggle/state, but draw calls are layer-owned.
- Layer type naming is intentional:
  - `VisualLayer`: visual concerns only.
  - Future logical orchestration belongs in separate logical systems/layers.

## 1.5 Style & Structure Constraints

- Prefer OO boundaries and clear encapsulation.
- Deviate for efficiency only when justified.
- Avoid “function-sprawl”; optimize for coherent object interactions.
- Many focused files are acceptable when they improve maintainability.
- Centralize configuration in config modules.
- Namespace scope is mandatory:
  - entity-domain constants/types should live under `amb::entity` (or stricter sub-namespace) to prevent collisions.

## 1.6 Domain Reminder (Non-Normative)

This is **not** a tile-RPG movement model.  
This subsystem is a **top-down 2D flight control simulation** with high skill expression and demanding control precision.

---

## 2) Units & Glossary

## 2.1 Units (Normative)

| Concept | Unit / Convention |
|---|---|
| Heading angle | **Degrees** |
| Heading reference | `0/360 = up`, positive rotation = clockwise |
| Position | `float` |
| Velocity | `float` |
| Update math timebase | **Milliseconds** |
| Numeric precision | `float` (v1 default) |

## 2.2 Coordinate Spaces

| Space | Meaning |
|---|---|
| Tile space | Integer map-grid coordinates (`tile_x`, `tile_y`) |
| World space | Continuous gameplay coordinates (`world_x`, `world_y`) |
| Screen space | Post-camera pixel/render coordinates |
| Camera/view space | World transformed by camera state |

## 2.3 Origin & Bounds

- `(0,0)` is top-left.
- Positive directions: +x right, +y down.
- Negative traversal is out-of-scope for current flight map bounds.

## 2.4 Motion Representation (v1)

- Keep both:
  - **Continuous heading float** for actual movement path.
  - **Discrete facing bucket** for sprite selection.
- Discrete facing is recalculated from heading on direction-change update/input steps.

## 2.5 Flight Semantics Note

- Primary movement model: scalar speed + heading.
- Rotation and movement are intentionally decoupled (space-flight feel).
- Roll visuals and reverse-roll behavior are handled via control logic/state transitions (not by introducing extra world axes).

---

## 3) Decision Log (ADR-lite, same file)

> Keep this section updated when decisions are accepted or superseded.

| ID | Date | Status | Decision | Rationale | Consequences | Revisit Trigger |
|---|---|---|---|---|---|---|
| ENT-001 | 2026-02-26 | accepted | Separate behavior entities from runtime render/update data. | Clean architecture + hot path efficiency. | Requires strict ownership/reference discipline. | If control/render coupling appears in PRs repeatedly. |
| ENT-002 | 2026-02-26 | accepted | Runtime storage v1 uses AoS vector. | Simplicity + contiguous iteration. | Later migration may be needed for very large scenes. | Profiling shows cache pressure at scale. |
| ENT-003 | 2026-02-26 | accepted | Entity IDs are stable `u16` per scene. | Compact and sufficient for expected scene size. | Must guard against overflow in tooling/loaders. | If scene entity counts approach limit. |
| ENT-004 | 2026-02-26 | accepted | Main loop is single-threaded for input/update/render. | Deterministic control feel and simpler ordering. | Background work must not mutate hot runtime directly. | If CPU budget is repeatedly exceeded. |
| ENT-005 | 2026-02-26 | accepted | Fixed-step 120 Hz update, render uncapped. | Strong responsiveness target with stable simulation. | Requires catch-up cap policy. | If update starvation/jitter observed. |
| ENT-006 | 2026-02-26 | accepted | Catch-up policy: hard cap + drop excess accumulated time. | Prevents death spirals and long-lag recovery tails. | Rare heavy frames may lose sim time. | If sim-time loss impacts gameplay feel. |
| ENT-007 | 2026-02-26 | accepted | Input model: event-driven command queue with anti-mash semantics. | Reward precision over button spam. | Requires command coalescing/conflict rules. | If controls feel sticky/unresponsive. |
| ENT-008 | 2026-02-26 | accepted | Visibility: rebuild index views each update, iterate by `visible_count`. | No per-frame allocations, simple hot loops. | Requires preallocated buffers and count discipline. | If full scans become too expensive. |
| ENT-009 | 2026-02-28 | accepted | Keep entity IDs in behavior wrappers and keep `EntityRuntime` free of identity fields. | Preserve runtime hot-path compactness while retaining stable control-layer identity. | Systems that need identity must consume entity wrappers or side mappings. | If runtime-side ID lookups become a measured bottleneck. |

---

## 4) Done Criteria Template (Per Atomic Step)

> Use this template for each checklist item before marking complete.

### 4.1 Step Header
- **Step ID:**  
- **Objective:**  
- **Scope (in):**  
- **Scope (out):**

### 4.2 Contract Checks
- [ ] Separation preserved (control vs update vs render).
- [ ] No cross-layer leakage of responsibilities.
- [ ] Namespace/config conventions respected.
- [ ] No undocumented assumptions introduced.

### 4.3 Functional Acceptance
- [ ] Required behavior implemented.
- [ ] Expected edge cases handled.
- [ ] Error/failure behavior defined where applicable.

### 4.4 Runtime/Performance Acceptance
- [ ] Hot-path data remains minimal.
- [ ] No avoidable per-frame allocations introduced.
- [ ] Iteration strategy aligns with current policy.

### 4.5 Integration Acceptance
- [ ] Builds logically with current architecture.
- [ ] Does not force unintended changes in unrelated systems.
- [ ] Future extension points remain coherent.

### 4.6 Evidence / Notes
- **What changed:**  
- **How validated:**  
- **Tradeoffs accepted:**  
- **Follow-up tasks created:**  

> **Completion rule:** Item is complete when maintainer confirms it is complete.

---

## 5) Input / Control Timing Contract

## 5.1 Pipeline Model (Normative)

1. Poll input events.
2. Convert to command queue entries (event-driven).
3. Run fixed update ticks at 120 Hz target.
4. Consume/apply commands in update.
5. Update runtime state.
6. Render as fast as possible from latest state.

## 5.2 Fixed-Step Policy

- Target update frequency: **120 Hz**.
- Update timebase: **milliseconds**.
- Render/update are decoupled.
- Catch-up handling:
  - **Hard cap on update steps per frame**.
  - **Drop excess accumulated time** beyond cap.
  - Keep diagnostics to tune cap later.

## 5.3 Input Queue Semantics (Anti-Mash Contract)

- Queue is event-driven, not raw-state spam.
- Repeated same-direction thrust commands do not stack infinitely.
- Conflicting commands resolve by explicit rule order (example: brake can negate pending thrust step).
- Inputs are designed to reward precision and timing, not button mashing.

## 5.4 Responsiveness Policy

- **Balanced fixed-step policy**, leaning deterministic:
  - preserve stable update ordering and predictability,
  - while keeping latency practical through frequent event polling and high update rate.
- If tradeoffs are needed, preserve control consistency first unless profiling/user testing proves otherwise.

## 5.5 Threading Boundaries

- Main gameplay loop (input/update/render): **single thread**.
- Candidate background threads (optional): preload, sound, background messaging/timers.
- Background workers must communicate via safe handoff mechanisms and must not directly mutate hot runtime data used in the current tick/render.

---

## 6) Visibility Iteration Contract

## 6.1 Master Data

- Keep master runtime vector stable (contiguous, scene-owned).
- Do not remove/invalidate runtime records to express visibility changes.

## 6.2 Derived Visibility Views

- Preallocate visibility index buffers (capacity >= master runtime count).
- Each update tick:
  - reset `visible_count = 0`,
  - overwrite indices for currently visible entities,
  - render iterates `0 .. visible_count-1`.

## 6.3 Allocation Policy

- No per-frame heap churn for visibility lists.
- Reuse allocated buffers; overwrite contents each update.

## 6.4 Growth Policy

Progression path when scale demands it:
1. Full scan + rebuilt visible indices.
2. Spatial partition assist (e.g., uniform grid) to reduce candidate set.
3. Additional specialization only if profiling shows need.

---

## 7) Practical Notes (Non-Normative)

- Favor “simple and correct” before “clever and fragile.”
- Challenge assumptions early, but once a decision is accepted, execute cleanly.
- The goal is not theoretical perfection; the goal is coherent systems that produce elite-feel controls.
