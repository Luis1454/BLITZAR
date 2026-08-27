# Decision 039: Executable Directive Configuration

Status: accepted
Issue: #637
Plan version: 1.0.23

## Context

The directive-file parser is a syntax contract only. A usable headless product
also needs a bounded semantic contract that can configure and execute the
rewrite without importing the old repository's scene, generation, or runtime
implementation.

## Decision

The CLI accepts a closed set of rewrite-owned directives:

- `simulation(particle_count, dt, solver, integrator)` is required.
- `gravity(gravitational_constant, softening)` is required.
- `units(length_scale, mass_scale, time_scale)` is required.
- `generation(seed, deterministic)` is required and only `true` is supported.
- `barnes_hut(opening_angle, max_particles, max_cells, leaf_capacity, max_depth)` is optional.
- `run(steps)` is optional and defaults to one step.

Each directive may occur once. Its argument set is exact, values are parsed
with strict numeric and boolean conversion, and the typed result is assigned
only after successful validation. `direct`, `barnes_hut`, and `fmm` are the
supported solver values. `pm`, `treepm`, and non-KDK integrators return
`BLITZAR_STATUS_UNSUPPORTED`; unknown directives and malformed values return
the corresponding invalid or unsupported status without partial configuration.

The CLI generates a deterministic rewrite-native structure-of-arrays state
from the configured seed, then calls the existing public C++ `Simulation`
facade for configuration, particle injection, stepping, and state extraction.
No old source, old runtime, or legacy semantic behavior is linked or copied.
The no-argument context smoke path remains available. A successful configured
run emits one machine-readable summary line; failures identify the phase and
public status.

## Consequences

Configuration semantics remain an internal adapter and do not expand the C
ABI. Particle generation is intentionally a small deterministic fixture for
the headless executable, not a promise to reproduce historical scenes. Future
input formats must define a new semantic adapter and cannot silently widen
this contract.
