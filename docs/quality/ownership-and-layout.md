# Ownership and Human Layout

Production C++ and CUDA code follows these rules:

- Ownership uses `std::unique_ptr` or a value member.
- Non-owning internal APIs use references or explicit view types.
- Qt members that observe parent-owned objects use `QPointer<T>`.
- Raw pointers are confined to ABI, process, OS, and Qt construction boundaries.
- Internal scene-editor code has a stricter contract: no raw pointer declaration is permitted;
  Qt object references use `QPointer<T>` and transferred non-`QObject` ownership uses RAII.
- A boundary converts incoming text, arrays, and opaque state before entering business logic.
- Local declarations are one per statement and initialized at the point of computation.
- `const` is used whenever the value is not reassigned.
- Related initialization statements are grouped by logical phase; declarations are not hoisted solely for style.
- Multiple declarations separated by commas and raw ownership assignments are policy errors.

`scripts/analysis/refactor_map.py` reports pointer candidates and possible dead files. It never deletes code. A deletion requires no source, manifest, registration, export, signal, or test reference.
