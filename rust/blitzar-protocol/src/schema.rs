// File: rust/blitzar-protocol/src/schema.rs
// Purpose: Rust component implementation for BLITZAR runtime services.

use crate::LATEST_PROTOCOL_VERSION;
use serde_json::Value;
use serde_json::json;

#[must_use]
pub fn latest_schema_value() -> Value {
    json!({
        "schema": LATEST_PROTOCOL_VERSION.label(),
        "transport": {
            "kind": "tcp-json-lines",
            "frame_delimiter": "\n"
        },
        "request_envelope": {
            "required": ["cmd"],
            "optional": ["token"],
            "extra_fields": "command-specific"
        },
        "response_envelope": {
            "success_required": ["ok", "cmd"],
            "error_required": ["ok", "cmd", "error"]
        },
        "compatibility": {
            "compatible_additions": [
                "new optional response fields",
                "new additive commands",
                "new optional command fields"
            ],
            "breaking_changes": [
                "rename or remove command identifiers",
                "rename or remove mandatory fields",
                "change auth behavior",
                "change particle tuple layout"
            ]
        },
        "deprecation": {
            "announce_in": "docs/server-protocol.md",
            "minimum_support_window": "one release",
            "removal_requires_new_schema": true
        },
        "commands": {
            "status": { "request": [], "response": "status_payload" },
            "get_snapshot": { "request": ["max_points"], "response": "snapshot_payload" },
            "pause": { "request": [], "response": "response_envelope" },
            "resume": { "request": [], "response": "response_envelope" },
            "toggle": { "request": [], "response": "response_envelope" },
            "reset": { "request": [], "response": "response_envelope" },
            "recover": { "request": [], "response": "response_envelope" },
            "step": { "request": ["count"], "response": "response_envelope" },
            "set_dt": { "request": ["value"], "response": "response_envelope" },
            "set_solver": { "request": ["value"], "response": "response_envelope" },
            "set_integrator": { "request": ["value"], "response": "response_envelope" },
            "set_performance_profile": { "request": ["value"], "response": "response_envelope" },
            "set_particle_count": { "request": ["value"], "response": "response_envelope" },
            "set_sph": { "request": ["value"], "response": "response_envelope" },
            "set_octree": { "request": ["theta", "softening"], "response": "response_envelope" },
            "set_sph_params": { "request": ["h", "rest_density", "gas_constant", "viscosity"], "response": "response_envelope" },
            "set_substeps": { "request": ["target_dt", "max"], "response": "response_envelope" },
            "set_energy_measure": { "request": ["every_steps", "sample_limit"], "response": "response_envelope" },
            "set_gpu_telemetry": { "request": ["value"], "response": "response_envelope" },
            "set_snapshot_publish_cadence": { "request": ["period_ms"], "response": "response_envelope" },
            "load": { "request": ["path", "format"], "response": "response_envelope" },
            "load_checkpoint": { "request": ["path"], "response": "response_envelope" },
            "export": { "request": ["path", "format"], "response": "response_envelope" },
            "save_checkpoint": { "request": ["path"], "response": "response_envelope" },
            "shutdown": { "request": [], "response": "response_envelope" }
        },
        "payloads": {
            "status_payload": {
                "required": [
                    "steps", "dt", "paused", "faulted", "fault_step", "fault_reason", "sph",
                    "server_fps", "performance_profile", "substep_target_dt", "substep_dt",
                    "substeps", "max_substeps", "snapshot_publish_period_ms", "particles",
                    "solver", "integrator", "ekin", "epot", "eth", "erad", "etot",
                    "drift_pct", "estimated"
                ],
                "optional": [
                    "gpu_telemetry_enabled", "gpu_telemetry_available", "gpu_kernel_ms",
                    "gpu_copy_ms", "gpu_vram_used_bytes", "gpu_vram_total_bytes",
                    "export_queue_depth", "export_active", "export_completed_count",
                    "export_failed_count", "export_last_state", "export_last_path",
                    "export_last_message"
                ]
            },
            "snapshot_payload": {
                "required": ["has_snapshot", "count", "particles"],
                "particle_tuple": ["x", "y", "z", "mass", "pressureNorm", "temperature"]
            }
        }
    })
}

#[must_use]
pub fn latest_schema_pretty_json() -> String {
    serde_json::to_string_pretty(&latest_schema_value())
        .expect("protocol schema serialization must succeed")
}
