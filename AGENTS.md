# Clean-Room Contribution Rules

This repository is implemented from `PLAN.md` and `plan/manifest.json` only.
Do not inspect, copy, translate, or use the old BLITZAR repository as context.

## Workflow

- One coherent feature per branch.
- Define the contract and acceptance tests before implementation.
- Keep the direct CPU solver as the numerical reference.
- Batch related edits, then run the relevant focused checks; do not rebuild after
  every cosmetic change.
- Do not commit generated build trees or local environment files.

## Design Rules

- Use composition and constructor dependency injection.
- Keep ownership explicit with RAII and `std::unique_ptr` where indirection is
  necessary; owning raw pointers are forbidden.
- Do not create god structs. Separate storage, policy, execution context, and
  reporting when their lifetimes or responsibilities differ.
- Keep the C ABI opaque and stable; put C++ ergonomics in the wrapper.
- Public headers may include only registered standard-library headers and the
  public C ABI header. MPI, HIP, CUDA, `src/`, and internal implementation
  headers are forbidden at that boundary. ABI pointer and V1 parameter
  exceptions must be registered in `plan/public_boundary.json` or
  `plan/parameter_exceptions.json`.
- Native MPI headers, handles, and preprocessor symbols are confined to the
  registered units in `plan/quality.json`. `MpiContext`, domain decomposition,
  packet contracts, and ghost state consume only non-MPI descriptors; new MPI
  calls require an update to the native-boundary gate and its evidence.
- Keep one-level namespaces and never use `using namespace`.
- Do not add `utils`, `common`, `misc`, `private`, or `details` catch-alls.
- Do not split files mechanically by line count. Review responsibility,
  function count, branching, and allocation behavior together.
- Do not allocate dynamically after initialization unless the contract records
  and tests the exception.
- Avoid recursion and unbounded loops in production simulation paths.

## Naming

C++, CUDA, and header files use PascalCase names that are unique by complete
filename across the code tree and match their primary type when they declare
one. A repeated stem is allowed only for an explicitly configured
implementation/header pair. Public ABI spellings and build-template names are
registered exceptions, not implicit overrides. Maximum filename lengths are
defined per repository profile in `plan/quality.json`; `short` is not a
subjective exception. Rust, Python, and configuration files use their
language's established lowercase convention. Names must not repeat the
containing component without adding meaning, and legacy/server or catch-all
prefixes and path components are forbidden.

## Validation

Every implementation phase requires deterministic tests, sanitizer or static
analysis coverage appropriate to the target, and a numerical comparison against
the reference where applicable. A feature is not complete because its files
compile; its observable behavior must be demonstrated. Source files use UTF-8
without a BOM, LF line endings, a final newline, the pinned clang-format version
from `plan/quality.json`, and the recursive grouping checker.
