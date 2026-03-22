use crate::v1::ResponseEnvelope;
use serde::{Deserialize, Serialize};

#[derive(Clone, Debug, Default, Deserialize, PartialEq, Serialize)]
pub struct StatusPayload {
    #[serde(flatten)]
    pub envelope: ResponseEnvelope,
    #[serde(default)]
    pub steps: u64,
    #[serde(default)]
    pub dt: f32,
    #[serde(default)]
    pub paused: bool,
    #[serde(default)]
    pub faulted: bool,
    #[serde(default, rename = "fault_step")]
    pub fault_step: u64,
    #[serde(default, rename = "fault_reason")]
    pub fault_reason: String,
    #[serde(default, rename = "sph")]
    pub sph_enabled: bool,
    #[serde(default, rename = "server_fps")]
    pub server_fps: f32,
    #[serde(default, rename = "performance_profile")]
    pub performance_profile: String,
    #[serde(default, rename = "substep_target_dt")]
    pub substep_target_dt: f32,
    #[serde(default, rename = "substep_dt")]
    pub substep_dt: f32,
    #[serde(default)]
    pub substeps: u32,
    #[serde(default, rename = "max_substeps")]
    pub max_substeps: u32,
    #[serde(default, rename = "snapshot_publish_period_ms")]
    pub snapshot_publish_period_ms: u32,
    #[serde(default, rename = "particles")]
    pub particle_count: u32,
    #[serde(default)]
    pub solver: String,
    #[serde(default)]
    pub integrator: String,
    #[serde(default, rename = "ekin")]
    pub kinetic_energy: f32,
    #[serde(default, rename = "epot")]
    pub potential_energy: f32,
    #[serde(default, rename = "eth")]
    pub thermal_energy: f32,
    #[serde(default, rename = "erad")]
    pub radiated_energy: f32,
    #[serde(default, rename = "etot")]
    pub total_energy: f32,
    #[serde(default, rename = "drift_pct")]
    pub energy_drift_pct: f32,
    #[serde(default, rename = "estimated")]
    pub energy_estimated: bool,
    #[serde(default, rename = "gpu_telemetry_enabled")]
    pub gpu_telemetry_enabled: bool,
    #[serde(default, rename = "gpu_telemetry_available")]
    pub gpu_telemetry_available: bool,
    #[serde(default, rename = "gpu_kernel_ms")]
    pub gpu_kernel_ms: f32,
    #[serde(default, rename = "gpu_copy_ms")]
    pub gpu_copy_ms: f32,
    #[serde(default, rename = "gpu_vram_used_bytes")]
    pub gpu_vram_used_bytes: u64,
    #[serde(default, rename = "gpu_vram_total_bytes")]
    pub gpu_vram_total_bytes: u64,
}
