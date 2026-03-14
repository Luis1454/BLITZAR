# Server JSON Protocol (TCP)

Canonical compatibility governance for this surface is defined in `docs/quality/interface_contracts.md` (`CTR-PROT-001`). The Rust mirror for this contract lives in `rust/blitzar-protocol`.
The machine-readable schema artifact for the current wire contract is `docs/server_protocol_schema.json`.

Transport:
- TCP, one JSON object per line (`\n` delimited)
- Request shape: `{"cmd":"<name>", ...fields }`
- Response shape success: `{"ok":true,"cmd":"<name>", ...payload }`
- Response shape error: `{"ok":false,"cmd":"<name>","error":"<message>" }`
- Optional auth:
  - When server is started with `--server-token <secret>`, every request must include
    `token:"<secret>"` (for example `{"cmd":"status","token":"secret"}`).
  - Missing/invalid token is rejected with `{"ok":false,"cmd":"auth","error":"unauthorized"}`.

Core commands:
- `status`
  - Response payload:
    - `steps` (`uint64`)
    - `dt` (`float`)
    - `paused` (`bool`)
    - `faulted` (`bool`)
    - `fault_step` (`uint64`)
    - `fault_reason` (`string`)
    - `sph` (`bool`)
    - `server_fps` (`float`, simulation steps per second)
    - `performance_profile` (`string`)
    - `substep_target_dt` (`float`)
    - `substep_dt` (`float`)
    - `substeps` (`uint32`)
    - `max_substeps` (`uint32`)
    - `snapshot_publish_period_ms` (`uint32`)
    - `particles` (`uint32`)
    - `solver` (`string`)
    - `ekin`, `epot`, `eth`, `erad`, `etot` (`float`)
    - `drift_pct` (`float`)
    - `estimated` (`bool`)
- `get_snapshot`
  - Fields:
    - `max_points` (`uint32`, clamped to `[1,20000]`)
  - Response payload:
    - `has_snapshot` (`bool`)
    - `count` (`uint32`)
    - `particles`: array of `[x,y,z,mass,pressureNorm,temperature]`

Control commands:
- `pause`, `resume`, `toggle`
- `reset`
- `recover` (explicit fault recovery path; triggers server reset/reinit)
- `step` (`count:int`, clamped by server)
- `shutdown`

Runtime config commands:
- `set_dt` (`value:float`)
- `set_solver` (`value:string`)
- `set_integrator` (`value:string`)
- `set_particle_count` (`value:uint64`)
- `set_sph` (`value:bool`)
- `set_octree` (`theta:float`, `softening:float`)
- `set_sph_params` (`h`, `rest_density`, `gas_constant`, `viscosity`)
- `set_energy_measure` (`every_steps:uint32`, `sample_limit:uint32`)

I/O commands:
- `load` (`path:string`, `format:string=auto`) triggers reset on server
- `export` (`path?:string`, `format?:string`)

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
- The committed schema artifact in `docs/server_protocol_schema.json` is the source of truth for transport adapters outside the in-process client.



