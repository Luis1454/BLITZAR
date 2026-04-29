# API Documentation Standard

This document defines the lightweight comment format used for public C++ APIs and split implementation fragments.

## File Header Format

Every public header or split implementation fragment should start with a short responsibility block:

```cpp
/*
 * Module: ui
 * Responsibility: Render the recent energy history exposed by the client runtime.
 */
```

Rules:
- `Module` names the functional area, not the directory path.
- `Responsibility` states one stable reason for the file to exist.
- Keep the block short enough to survive refactors without constant churn.

## Public API Comment Format

Document each public type and public function with concise block comments using normalized tags:

```cpp
/*
 * @brief Renders the rolling energy and drift history for the current simulation session.
 * @param None This contract does not take explicit parameters.
 * @return Not applicable; this block documents a type contract.
 * @note Keep ownership and runtime side effects explicit.
 */
class EnergyGraphWidget : public QWidget {
public:
    /*
     * @brief Appends a telemetry sample to the visible energy history.
     * @param stats Latest telemetry sample received from the runtime.
     * @return No return value.
     * @note Preserve deterministic retention limits when updating graph history.
     */
    void pushSample(const SimulationStats &stats);
};
```

Rules:
- Describe purpose, not implementation detail.
- Mention units or constraints when they are not obvious.
- Mention ownership transfer when constructors or functions take ownership.
- Use `@brief`, `@param`, `@return`, and `@note` consistently.

## Layout and Spacing

Use spacing to keep the code readable instead of dense:

- Keep `if` / `else if` / `else` chains contiguous.
- Insert a blank line when a block finishes and the next block starts a different responsibility.
- Keep related variable declarations together, but separate different conceptual phases with a blank line.
- Prefer splitting dense methods into smaller helpers instead of stacking long cascades of statements.

## Enforcement

The repository quality gate enforces this format for public headers under:

- `engine/include/`
- `runtime/include/`
- `modules/qt/include/ui/`

The same gate also checks file header blocks across the main C++ sources so missing documentation is surfaced early.

## Scope

Apply this format first to:
- public headers under `engine/include/`
- public headers under `runtime/include/`
- Qt-facing headers under `modules/qt/include/ui/`
- split CUDA/core fragments where a file-level responsibility header clarifies the partition

## Non-Goals

- Do not duplicate implementation comments already obvious from the code.
- Do not add long narrative blocks to private helpers.
- Do not turn headers into design documents; detailed rationale stays in `docs/quality/`.
