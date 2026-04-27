// File: rust/blitzar-protocol/src/v1/snapshot.rs
// Purpose: Rust component implementation for BLITZAR runtime services.

use crate::v1::ResponseEnvelope;
use serde::{Deserialize, Serialize};

#[derive(Clone, Debug, PartialEq)]
/// Description: Defines the RenderParticle struct contract.
pub struct RenderParticle {
    pub x: f32,
    pub y: f32,
    pub z: f32,
    pub mass: f32,
    pub pressure_norm: f32,
    pub temperature: f32,
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
/// Description: Defines the RenderParticleTuple struct contract.
struct RenderParticleTuple(f32, f32, f32, f32, f32, f32);

/// Description: Implements behavior for the associated Rust type.
impl From<RenderParticle> for RenderParticleTuple {
    /// Description: Executes the from operation.
    fn from(value: RenderParticle) -> Self {
        Self(
            value.x,
            value.y,
            value.z,
            value.mass,
            value.pressure_norm,
            value.temperature,
        )
    }
}

/// Description: Implements behavior for the associated Rust type.
impl TryFrom<RenderParticleTuple> for RenderParticle {
    type Error = &'static str;

    /// Description: Executes the try_from operation.
    fn try_from(value: RenderParticleTuple) -> Result<Self, Self::Error> {
        Ok(Self {
            x: value.0,
            y: value.1,
            z: value.2,
            mass: value.3,
            pressure_norm: value.4,
            temperature: value.5,
        })
    }
}

/// Description: Implements behavior for the associated Rust type.
impl Serialize for RenderParticle {
    /// Description: Executes the serialize operation.
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: serde::Serializer,
    {
        RenderParticleTuple::from(self.clone()).serialize(serializer)
    }
}

impl<'de> Deserialize<'de> for RenderParticle {
    /// Description: Executes the deserialize operation.
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: serde::Deserializer<'de>,
    {
        RenderParticleTuple::deserialize(deserializer)?
            .try_into()
            .map_err(serde::de::Error::custom)
    }
}

#[derive(Clone, Debug, Default, Deserialize, PartialEq, Serialize)]
/// Description: Defines the SnapshotPayload struct contract.
pub struct SnapshotPayload {
    #[serde(flatten)]
    pub envelope: ResponseEnvelope,
    #[serde(default, rename = "has_snapshot")]
    pub has_snapshot: bool,
    #[serde(default)]
    pub count: u32,
    #[serde(default)]
    pub particles: Vec<RenderParticle>,
}
