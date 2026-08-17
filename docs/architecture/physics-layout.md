# Physics Module Layout

Physics modules follow the repository-wide responsibility grammar.

```text
engine/physics/<method>/
  model/
  build/
  force/
  runtime/
  cuda/
    <responsibility>/
  tests/
  Module.cmake
```

Only responsibilities implemented by the method are materialized. CUDA
fragments are stored directly in their owning responsibility directory; the
generic `fragments/` directory is forbidden.

TreePM and octree keep their algorithm-specific responsibilities separate so
that grid deposition, FFT, Morton layout, force evaluation, and integration
can be tested and compiled independently.
