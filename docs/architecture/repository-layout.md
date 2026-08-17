# Repository Layout

BLITZAR is organized by functional domain first. File extensions and explicit
technical words describe the next level only when they carry real meaning.

## Module Template

```text
engine/
  <domain>/
      <module>/
      <Responsibility>*.hpp  public and internal headers for the responsibility
      <Responsibility>*.cpp  host implementation for the responsibility
      cuda/                  CUDA implementation, when present
        fragments/           included .inl source fragments
      tests/                 tests owned by this module
      Module.cmake         module source and visibility manifest
```

Examples:

```text
engine/physics/octree/
engine/physics/treepm/
engine/config/registry/
engine/server/simulation/
engine/platform/
```

## Aggregators

Some domains contain several independent leaf responsibilities. They may use
an explicit aggregator directory rather than a dense implementation directory:

```text
engine/config/                 # aggregator: Module.cmake and public facade headers only
  args/                         # leaf responsibility and Module.cmake
  registry/
    Module.cmake
  validation/
    include/ src/ Module.cmake
```

An aggregator owns composition and manifests, not `.cpp`, `.cu`, or `.inl`
implementation files. Its children own their responsibility files directly,
with optional `cuda/`, `fragments/`, and `tests/` directories. A small number
of direct public facade headers is allowed when they preserve a stable
cross-module include.

The repository root keeps cross-module tests, release tooling, applications,
and deployment assets. A module-local `tests/` directory is for tests that have
one module as their ownership boundary; cross-module, GUI, performance, and
scientific qualification tests remain under the root `tests/` tree.

## Naming And Placement

- The first directory is the functional domain, not a build-system concept.
- A module is the smallest independently understandable responsibility.
- Do not create generic `include/` or `src/` directories inside a responsibility.
  The responsibility directory itself is the source boundary and contains its
  headers and implementations together.
- `cuda/` is used when a module owns CUDA code. It may contain `.cu`, `.cuh`,
  and `.hpp` files when they belong to the CUDA implementation.
- `cuda/fragments/` is reserved for `.inl` files included by a parent
  translation unit. It does not imply that a file contains only `__device__`
  functions or only `__global__` kernels.
- Do not create `api/`, `public/`, `private/`, `detail/`, `utils/`, or
  `fragments/` directories as generic catch-alls.
- Create another directory only when it represents a stable responsibility,
  backend, or source-composition boundary.
- File extensions remain visible in the filenames; do not create `cpp/` or
  `hpp/` directories solely to repeat the extension.

## Responsibility Prefixes

PascalCase remains mandatory. Generic technical files and all `.inl` fragments
must begin with a three-letter responsibility prefix so that a filename stays
unambiguous outside its parent directory:

| Prefix | Responsibility |
| --- | --- |
| `Bat` | batch execution |
| `Cfg` | configuration implementation |
| `Cli` | client runtime |
| `Cmd` | command execution |
| `Cud` | shared CUDA implementation |
| `Ffi` | foreign-function interface |
| `Fmm` | fast multipole method |
| `Fnd` | foundational engine types |
| `Gui` | Qt user interface |
| `Gfx` | graphics support |
| `Jit` | CUDA JIT specialization |
| `Oct` | octree implementation |
| `Phy` | shared physics |
| `Plt` | platform abstraction |
| `Ptc` | protocol |
| `Sph` | SPH implementation |
| `Srv` | simulation server implementation |
| `Thm` | thermal implementation |
| `Tpm` | TreePM implementation |
| `Typ` | shared simulation types |

Examples are `OctBuffer.inl`, `SphBuffer.inl`, and `TpmGridBuild.inl`.
Primary class files keep the class name when it already starts with the
responsibility code (`Octree.cpp`, `FmmCpu.hpp`); otherwise every production
file uses the code as a prefix. The prefix rule targets generic implementation
names such as `Buffer`, `Build`, `Force`, `Grid`, `State`, and `Update`. New responsibility codes require
an update to this table and to the automated architecture check.

## Visibility

Visibility is declared in `Module.cmake` and enforced by CMake target include
directories. Directory names describe source ownership; they do not model C++
class access control.

## Migration Rule

Moves are performed module by module with `git mv`, followed in the same change
by include-path, CMake, test, and traceability updates. No compatibility copy is
allowed after the module has migrated. A migration is complete only when the
old production path is absent and the relevant platform builds pass.
