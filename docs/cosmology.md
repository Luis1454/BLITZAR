# Cosmology Scenario

BLITZAR provides a deterministic Newtonian cosmology scenario for collisionless matter.
It is intended to study the transition from a nearly homogeneous expanding patch to
gravitationally amplified structure, not to replace a relativistic Boltzmann solver or a
full hydrodynamic cosmological code.

## Model

The `cosmology` initialization mode samples either a uniform sphere or cube. A small,
deterministic set of coherent sinusoidal perturbations is applied to the Lagrangian
positions. The initial state stores a Zel'dovich-like peculiar velocity component. The
Hubble flow is applied by the runtime scale-factor operator, so it is not integrated a
second time by the ordinary N-body drift. The background follows the flat Friedmann
approximation:

`H(a) = H0 sqrt(Omega_r/a^4 + Omega_m/a^3 + Omega_Lambda)`

At every integration step the runtime advances `a(t)` with a midpoint step and applies
the corresponding expansion to positions. Peculiar velocities are damped by `1/a` while
the direct N-body force evolves local perturbations. This is an operator-split Newtonian
collisionless model. Units are simulation units; `h0` is an inverse simulation-time.

The supplied case starts at `a=0.01` (`z=99`), after recombination. Calling this “from the
Big Bang” describes the modeled cosmic history, not an integration from `a=0`, which is
singular and outside this Newtonian approximation.

The supplied production case is configured for a reproducible preview workload. For
million-body production runs, increase `simulation.particle_count` explicitly and select
the TreePM grid resolution according to the target density. Each body has mass `2^-20`,
preserving a unit total matter mass. The GUI transfers a deterministic 4096-body sample
for display; it does not reduce the physical calculation. Use
`scene_cosmology_preview.ini` for fast GUI iteration.

The CUDA TreePM FFT mesh doubles the requested resolution in each dimension to isolate
the periodic convolution. For example, `grid_size=64` creates a `128^3` FFT mesh; using
`grid_size=128` creates a `256^3` mesh and is substantially more expensive per step.
The supplied preview uses `grid_size=32` so that the same case remains testable on CPU.
The CPU implementation computes one inverse potential field and samples its spatial
gradient instead of running three inverse transforms. For reproducible CPU throughput,
set the OpenMP worker count explicitly, for example `OMP_NUM_THREADS=16` on a 16-thread
machine; the equivalent syntax is `$env:OMP_NUM_THREADS='16'` in PowerShell.

## Run

The reproducible case is [scene_cosmology_big_bang.ini](../tests/data/scene_cosmology_big_bang.ini).
Use it from the GUI's configuration loader or with the headless binary. The supplied
preview starts with 10,000 bodies, no artificial central mass, and a spherical domain.
It uses the GPU TreePM hybrid model with cuFFT by default. For a million-body run,
increase `simulation.particle_count` explicitly after validating the chosen mesh
resolution and available device memory.

For a cube, change `geometry=sphere` to `geometry=cube`; the cube uses
`box_half_extent`, while the sphere uses `sphere_radius`.
