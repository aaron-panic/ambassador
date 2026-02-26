# Ambassador C++ Coding Standards Guide

This document formalizes existing project conventions observed in the current codebase and should be treated as the default style contract unless explicitly overridden.

---

## 1) Naming Conventions

## 1.1 Class, struct, and type names
- Use **PascalCase** for classes/structs/type aliases.
- Examples: `Ambassador`, `DambLoader`, `MapRuntime`, `AtlasRuntime`, `ImageRuntime`, `MapLayer`, `VisualLayer`.

## 1.2 Member data
- Use `m_` prefix for class data members.
- Use lowercase snake_case after `m_`.
- Examples: `m_window`, `m_renderer`, `m_lasttick`, `m_map_runtime`, `m_spawn_point`.

## 1.3 Functions/methods
- Use **lowerCamelCase** for functions and methods.
- Examples: `checkInit`, `needUpdate`, `configureViewportGrid`, `layerViewportFor`, `loadMapLayer`, `defaultSpawnPoint`.

## 1.4 Constants
- Global/namespace constants use **ALL_CAPS_WITH_UNDERSCORES**.
- Examples: `APP_TITLE`, `DEFAULT_APP_WIDTH`, `MAP_TILE_SIZE`, `HEADER_SIZE`, `MAPL_HEADER_SIZE`.

## 1.5 Local variables
- Use lowercase snake_case.
- Examples: `raw_window`, `raw_renderer`, `map_entry`, `cell_count`, `viewport_w`.

## 1.6 Namespace usage
- Group domain concepts into namespaces.
- Existing pattern: `amb::config`, `amb::game`, `amb::data`, `amb::damb`, `amb::runtime`.
- New entity-domain symbols should be scoped under `amb::entity` (or deeper sub-namespace) to avoid collisions.

---

## 2) File & Header Structure

## 2.1 Header guards
- Use macro guards with `_INCLUDED` suffix.
- Pattern: `<FILE_NAME_UPPER>_INCLUDED`.
- Example: `AMBASSADOR_HXX_INCLUDED`, `FORMAT_DAMB_HXX_INCLUDED`.

## 2.2 Includes
- Project headers first, then external/system headers.
- Keep includes minimal and relevant to translation unit responsibilities.

## 2.3 Header/API split
- `.hxx`: declarations, type definitions, inline helpers.
- `.cxx`: implementation logic, orchestration, parsing, runtime behavior.

## 2.4 Keep headers coherent
- A header should represent one clear domain concept.
- Avoid dumping unrelated APIs into a single file.

---

## 3) Class Design & Encapsulation

## 3.1 Encapsulation-first
- Prefer private data + public methods.
- Expose mutable references only when intentionally part of API design.

## 3.2 Constructor initialization
- Use member initializer lists for class construction.
- Example pattern:
  - initialize members directly in constructor initializer list.

## 3.3 Virtual interfaces
- Use pure virtual base interfaces for pluggable systems (`render()`, `typeName()` style contracts).
- Add `override` on derived virtuals.

## 3.4 Ownership model
- Use RAII wrappers and smart pointers for resource ownership.
- Use `std::unique_ptr` for unique ownership (e.g., layer pointers).

---

## 4) Runtime & Performance-Oriented Conventions

## 4.1 Hot path data clarity
- Keep per-frame runtime data compact and explicit.
- Avoid mixing low-frequency configuration with high-frequency iteration structs.

## 4.2 Avoid unnecessary allocations in loops
- Reuse vectors/buffers where practical.
- Prefer reserve/reuse patterns over repeated allocations in hot paths.

## 4.3 Utility extraction
- Generic reusable parsing/string/binary helpers should live in utility modules, not be duplicated in domain files.
- Existing pattern:
  - `utility_parse.*`
  - `utility_string.*`
  - `utility_binary.hxx`

---

## 5) Error Handling & Validation

## 5.1 Validation first
- Validate external/file data aggressively and fail fast.
- Use clear runtime errors for malformed format inputs.

## 5.2 Logging
- Use SDL logging consistently for runtime/app-level failures.
- Keep messages specific and actionable.

## 5.3 Safety checks
- Null checks and bounds checks are expected before dereference/usage.
- Guard render/update paths against invalid state.

---

## 6) Formatting Style

## 6.1 Braces and blocks
- Opening brace on same line for function/control declarations.
- Use explicit braces for conditionals/loops even when single-line complexity may grow.

## 6.2 Readability over compression
- Prefer readable multiline argument lists when calls are long.
- Use intermediate local variables where it improves clarity.

## 6.3 Inline methods
- Small trivial accessors may be inline in headers.
- Keep non-trivial logic in `.cxx` unless intentionally header-only.

---

## 7) Format/Schema Definitions (DAMB-specific)

## 7.1 POD schema structs
- Keep binary format structs tightly defined and deterministic.
- Use `static_assert(sizeof(...))` and trivially-copyable checks for format structs.

## 7.2 Centralized format constants
- Store binary format constants in one place (`format_damb.hxx`), not scattered.

## 7.3 Alignment helpers
- Keep alignment/padding logic explicit and centralized.

---

## 8) Architectural Guardrails

- Preserve strict separation:
  - **Control/behavior**
  - **Runtime state mutation**
  - **Rendering**
- Visual-layer code should stay visual-only.
- Do not intermingle gameplay logic into rendering paths.
- Keep implementation atomic and maintain compileability after each merged change.

---

## 9) “When in doubt” rules

1. Match existing file-local style before introducing a new pattern.
2. Prefer maintainability and clear intent over clever but opaque code.
3. If behavior is not specified in prompt/spec/checklist, confirm before implementing.
4. Keep naming and namespace scoping explicit to reduce ambiguity.
