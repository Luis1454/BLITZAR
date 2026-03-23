# Server JSON Protocol (TCP)

Canonical compatibility governance for this surface is defined in `docs/quality/interface_contracts.md` (`CTR-PROT-001`). The Rust mirror for this contract lives in `rust/blitzar-protocol`.
The machine-readable schema artifact for the current wire contract is `docs/server_protocol_schema.json`.
The optional browser transport wrapper that preserves this contract is documented in `docs/web_gateway.md`.

Transport:
- TCP, one JSON object per line (`\n` delimited)
- Request shape: `{"cmd":"<name>", ...fields }`
- Response shape success: `{"ok":true,"cmd":"<name>", ...payload }`
- Response shape error: `{"ok":false,"cmd":"<name>","error":"<message>" }`
- Physical quantities use SI units unless a field name explicitly carries another unit such as `_ms`.
- Optional auth:
  - When server is started with `--server-token <secret>`, every request must include
    `token:"<secret>"` (for example `{"cmd":"status","token":"secret"}`).
  - Missing/invalid token is rejected with `{"ok":false,"cmd":"auth","error":"unauthorized"}`.

Core commands:
- `status`
  - Response payload:
    - `steps` (`uint64`)
    - `dt` (`float`, seconds `[s]`)
    - `paused` (`bool`)
    - `faulted` (`bool`)
    - `fault_step` (`uint64`)
    - `fault_reason` (`string`)
    - `sph` (`bool`)
    - `server_fps` (`float`, simulation steps per second)
    - `performance_profile` (`string`)
    - `substep_target_dt` (`float`, seconds `[s]`)
    - `substep_dt` (`float`, seconds `[s]`)
    - `substeps` (`uint32`)
    - `max_substeps` (`uint32`)
    - `snapshot_publish_period_ms` (`uint32`, milliseconds `[ms]`)
    - `particles` (`uint32`)
    - `solver` (`string`)
    - `ekin`, `epot`, `eth`, `erad`, `etot` (`float`, joules `[J]`)
    - `drift_pct` (`float`, percent `[%]`)
    - `estimated` (`bool`)
    - Optional GPU telemetry fields when enabled:
      - `gpu_telemetry_enabled` (`bool`)
      - `gpu_telemetry_available` (`bool`)
      - `gpu_kernel_ms` (`float`, milliseconds `[ms]`)
      - `gpu_copy_ms` (`float`, milliseconds `[ms]`)
      - `gpu_vram_used_bytes` (`uint64`, bytes)
      - `gpu_vram_total_bytes` (`uint64`, bytes)
- `get_snapshot`
  - Fields:
    - `max_points` (`uint32`, clamped to `[1,20000]`)
  - Response payload:
    - `has_snapshot` (`bool`)
    - `count` (`uint32`)
    - `particles`: array of `[x,y,z,mass,pressureNorm,temperature]` with `x/y/z` in `[m]`, `mass` in `[kg]`, and `temperature` in `[K]`

Control commands:
- `pause`, `resume`, `toggle`
- `reset`
- `recover` (explicit fault recovery path; triggers server reset/reinit)
- `step` (`count:int`, clamped by server)
- `shutdown`

Runtime config commands:
- `set_dt` (`value:float`)
- `set_dt` (`value:float`, seconds `[s]`)
- `set_solver` (`value:string`)
- `set_integrator` (`value:string`)
- `set_particle_count` (`value:uint64`)
- `set_sph` (`value:bool`)
- `set_octree` (`theta:float` dimensionless, `softening:float` in `[m]`)
- `set_sph_params` (`h` in `[m]`, `rest_density` in `[kg/m^3]`, `gas_constant` in solver units, `viscosity` in solver units)
- `set_energy_measure` (`every_steps:uint32`, `sample_limit:uint32`)
- `set_gpu_telemetry` (`value:bool`)

I/O commands:
- `load` (`path:string`, `format:string=auto`) triggers reset on server
- `load_checkpoint` (`path:string`)
  - loads a versioned binary checkpoint
  - rejects unsupported checkpoint versions or metadata that this build cannot restore
- `export` (`path?:string`, `format?:string`)
  - enqueues a background file-write job after the authoritative server thread captures the current state
  - status replies expose `export_queue_depth`, `export_active`, cumulative completion/failure counters, and the last export path/state/message
  - orderly shutdown drains queued export jobs before the process exits; abrupt termination may still interrupt pending writes
- `save_checkpoint` (`path:string`)
  - saves a versioned binary checkpoint containing particles and restartable runtime state

Reference:
- Constants and clamp rules are defined in `runtime/include/protocol/ServerProtocol.hpp`.
- Rust encode/decode parity for the `server-json-v1` schema is exercised in `rust/blitzar-protocol/tests/protocol.rs`.

Compatibility rules:
- Additive commands are compatible within `server-json-v1` when existing command names and mandatory fields stay unchanged.
- Additive optional request or response fields are compatible within `server-json-v1`.
- Renaming or removing command identifiers, renaming or removing mandatory fields, changing auth behavior, or changing the particle tuple layout is breaking and requires a new schema version.

Deprecation strategy:
- A deprecated command or field must be marked in this document and kept wire-compatible for at least one release.
- Removal of a deprecated command or field requires a new schema version and synchronized contract/test updates.
- The committed schema artifact in `docs/server_protocol_schema.json` is the source of truth for service transport adapters and client/runtime integrations.
- Web adapters must preserve backend command identifiers, mandatory fields, and payload shapes; any translation that changes those semantics is breaking.



