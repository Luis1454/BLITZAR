# Repository Layout

BLITZAR is organized by functional domain first. File extensions and explicit
technical words describe the next level only when they carry real meaning.

## Module Template

```text
engine/
  <domain>/
    <module>/
      include/             public headers used by other modules, when present
      src/                 private headers and host implementation, when present
      cuda/                CUDA implementation for this module, when present
        fragments/         included .inl source fragments
      tests/               tests owned by this module
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
  args/                         # leaf responsibility
    include/ src/ Module.cmake
  registry/
    include/ src/ Module.cmake
  validation/
    include/ src/ Module.cmake
```

An aggregator owns composition and manifests, not `.cpp`, `.cu`, or `.inl`
implementation files. Its children own their nearby `include/`, `src/`,
optional `cuda/`, and `tests/` directories. A small number of direct public
facade headers is allowed when they preserve a stable cross-module include.
Child namespaces may remain below a child `include/` directory when required
to preserve that public include contract; this is not a new generic layer.

The repository root keeps cross-module tests, release tooling, applications,
and deployment assets. A module-local `tests/` directory is for tests that have
one module as their ownership boundary; cross-module, GUI, performance, and
scientific qualification tests remain under the root `tests/` tree.

## Naming And Placement

- The first directory is the functional domain, not a build-system concept.
- A module is the smallest independently understandable responsibility.
- `include/` and `src/` are local to the leaf responsibility. An aggregator
  may contain leaf directories, but must not become a second implementation
  root. Do not add another generic `include/` or `src/` layer below a leaf.
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
| `Cfg` | configuration implementation |
| `Cud` | shared CUDA implementation |
| `Jit` | CUDA JIT specialization |
| `Oct` | octree implementation |
| `Sph` | SPH implementation |
| `Srv` | simulation server implementation |
| `Thm` | thermal implementation |
| `Tpm` | TreePM implementation |

Examples are `OctBuffer.inl`, `SphBuffer.inl`, and `TpmGridBuild.inl`.
Primary class files keep the class name (`Octree.cpp`, `FmmCpu.hpp`); the
prefix rule targets generic implementation names such as `Buffer`, `Build`,
`Force`, `Grid`, `State`, and `Update`. New responsibility codes require
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
