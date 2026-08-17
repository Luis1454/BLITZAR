# Physics Module Layout

Physics code follows the repository-wide module contract described in
`docs/architecture/repository-layout.md`.

```text
engine/physics/
  core/
    include/
    src/
    cuda/                 core-specific CUDA code
      fragments/
  octree/
    include/
    src/
    cuda/
      fragments/          octree CUDA fragments and linear-tree fragments
  treepm/
    include/
    src/
      fragments/          CPU TreePM source fragments
    cuda/
      fragments/          TreePM CUDA fragments
  fmm/
    include/
    src/
  cuda/
    include/              shared CUDA contracts
    src/                  shared CUDA runtime and ParticleSystem translation unit
    fragments/            system, integration, and JIT source fragments
  sph/
    cuda/fragments/
  thermal/
    cuda/fragments/
```

## Placement Rules

1. `core`, `octree`, `treepm`, and `fmm` are numerical modules, not source
   extension buckets.
2. CUDA fragments stay with the numerical method they implement. Shared
   system, integration, and JIT fragments stay in `physics/cuda/fragments/`.
3. An `.inl` file is a source-composition fragment. It may contain host
   launch code, `__global__` kernels, and `__device__` helpers; the directory
   does not claim a single CUDA execution qualifier.
4. Public headers are directly under the module's `include/` directory. The
   module include directory must not repeat the namespace path.
5. Private headers and host implementation are under the module's `src/`.
6. Tests owned by one numerical method live in that module's `tests/`; cross-
   method and scientific qualification tests remain under the root `tests/`.

Every move must update its CMake source manifest, include paths, traceability
paths, and file header. No old production path is retained as a compatibility
copy.
