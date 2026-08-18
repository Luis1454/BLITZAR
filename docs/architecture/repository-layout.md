# Repository Layout

BLITZAR uses one responsibility-oriented tree for every production module.

## Canonical Module

```text
engine/
  <domain>/
    <module>/
      <responsibility>/
        PascalCase.hpp
        PascalCase.cpp
        PascalCase.cu
        PrefixFragment.inl
      tests/
        snake_case.cpp
      Module.cmake
```

The only permitted variation is the semantic responsibility name and the
presence of a real backend. Empty placeholder directories are forbidden.

## Aggregators

Domain aggregators contain only child manifests and stable facade headers.
They do not contain production implementations. A module such as
`engine/server/simulation` owns its own manifest even when its parent domain
also has an aggregator manifest.

## Responsibilities

Responsibility folders are explicit and shallow. Examples include `model`,
`build`, `force`, `runtime`, `parsing`, `persistence`, `cuda`, `jit`, and
`tests`. A CUDA backend may add one responsibility level below `cuda`, such as
`cuda/fft` or `cuda/linear`.

The following directory names are forbidden for production code:
`src`, `include`, `private`, `public`, `api`, `details`, and `fragments`.

File extensions are kept in the filename. No `cpp/` or `hpp/` directory is
created solely to repeat an extension.

## Naming

Production C++ and CUDA files use PascalCase. Generic technical files and all
`.inl` fragments begin with a three-letter responsibility prefix. Current
prefixes are `Bat`, `Cfg`, `Cli`, `Cmd`, `Cud`, `Ffi`, `Fmm`, `Fnd`, `Gfx`,
`Gui`, `Jit`, `Oct`, `Phy`, `Plt`, `Ptc`, `Pxy`, `Srv`, `Sph`, `Thm`, `Tpm`,
and `Typ`.

Tests use snake_case and remain under the owning module's `tests/` directory.

## Migration

Every move uses `git mv` and updates includes, CMake manifests, tests,
traceability, and the `@file` declaration in the same change. The old
production path must be absent after migration. Compatibility copies are not
allowed.
