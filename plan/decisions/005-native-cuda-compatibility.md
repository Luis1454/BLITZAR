# Decision 005: Native NVIDIA CUDA Compatibility

Status: accepted
Plan version: 1.0.4

## Decision

When `HIP_PLATFORM=nvidia` is selected and `hipcc` is unavailable, CMake uses
the CUDA language with `nvcc` and `CUDAToolkit`. The internal
`src/accelerators/gpu/hip/bridge/Compatibility.hpp` maps the small runtime surface used by BLITZAR to CUDA
equivalents. GPU kernels remain `.hip` files and no CUDA or compatibility type
is exposed by the public SDK.

## Consequences

- Native NVIDIA builds require only a working CUDA Toolkit and host compiler.
- HIP/ROCm builds retain the existing HIP path and do not include the
  compatibility layer.
- `HIP_PLATFORM=nvidia` is required to select CUDA-only mode; otherwise `AUTO`
  remains CPU-safe when no HIP platform is selected.
- The same Direct/Barnes-Hut parity test qualifies both runtime paths when a
  GPU is visible.
