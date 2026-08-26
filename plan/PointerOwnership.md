# Pointer and Ownership Contract

The internal C++ contract does not use raw pointers for ownership. Production
ownership is represented by values, containers, or `std::unique_ptr` with an
explicit destruction order. Pointer-shaped interfaces are permitted only at a
registered ABI or execution boundary.

## Registered Boundaries

| Boundary | Allowed representation | Ownership rule |
| --- | --- | --- |
| `include/blitzar/blitzar.h` and its adapters | C pointers plus explicit counts | Borrowed for the duration of the call; adapters immediately create bounded `std::span` views. |
| CUDA/HIP runtime bridge | `void*`, `void**`, and address records | Runtime addresses are owned by `Buffers` and are never destroyed by request records. |
| GPU kernel records | device pointers | Borrowed views; counts and capacities live in the launch request and are validated before dispatch. |
| MPI native calls | `.data()` from validated spans | MPI owns only its request handles; wire storage remains in the owning transport until completion. |
| test allocator and process entry points | platform-mandated pointers | Instrumentation or process lifetime only; no production ownership is inferred. |

The C ABI handle implementation is the only production location allowed to use
`new` and `delete` directly. Internal code uses RAII containers and smart
pointers. `std::span` is the required representation for an internal buffer
and count pair whenever an external ABI does not impose a pointer-shaped
signature.

The gate in `tools/gates/pointer_ownership_gate.py` scans every C/C++/CUDA source
root, reports each registered pointer declaration with its category, and
rejects unregistered declarations, owner-like raw pointer fields, and direct
`new`/`delete` outside the C ABI handle implementation. The allowlist is
machine-readable in `plan/pointer_ownership.json` and is deliberately
file-specific so a new boundary requires an explicit contract update.
