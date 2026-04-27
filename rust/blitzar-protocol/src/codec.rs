// File: rust/blitzar-protocol/src/codec.rs
// Purpose: Rust component implementation for BLITZAR runtime services.

use crate::v1::{CommandRequest, ResponseEnvelope, SnapshotPayload, StatusPayload};
use serde::Serialize;
use serde::de::DeserializeOwned;

#[derive(Debug)]
/// Description: Defines the ProtocolCodecError enum contract.
pub enum ProtocolCodecError {
    Json(serde_json::Error),
    InvalidFrame(&'static str),
}

/// Description: Implements behavior for the associated Rust type.
impl std::fmt::Display for ProtocolCodecError {
    /// Description: Executes the fmt operation.
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            Self::Json(error) => write!(f, "{error}"),
            Self::InvalidFrame(message) => write!(f, "{message}"),
        }
    }
}

/// Description: Implements behavior for the associated Rust type.
impl std::error::Error for ProtocolCodecError {}

/// Description: Implements behavior for the associated Rust type.
impl From<serde_json::Error> for ProtocolCodecError {
    /// Description: Executes the from operation.
    fn from(value: serde_json::Error) -> Self {
        Self::Json(value)
    }
}

/// Description: Executes the encode operation.
fn encode<T>(value: &T) -> Result<String, ProtocolCodecError>
where
    T: Serialize,
{
    serde_json::to_string(value).map_err(ProtocolCodecError::from)
}

/// Description: Executes the decode operation.
fn decode<T>(raw: &str) -> Result<T, ProtocolCodecError>
where
    T: DeserializeOwned,
{
    serde_json::from_str(raw).map_err(ProtocolCodecError::from)
}

/// Description: Executes the encode_command_request operation.
pub fn encode_command_request(request: &CommandRequest) -> Result<String, ProtocolCodecError> {
    encode(request)
}

/// Description: Executes the decode_command_request operation.
pub fn decode_command_request(raw: &str) -> Result<CommandRequest, ProtocolCodecError> {
    decode(raw)
}

/// Description: Executes the encode_response_envelope operation.
pub fn encode_response_envelope(envelope: &ResponseEnvelope) -> Result<String, ProtocolCodecError> {
    encode(envelope)
}

/// Description: Executes the decode_response_envelope operation.
pub fn decode_response_envelope(raw: &str) -> Result<ResponseEnvelope, ProtocolCodecError> {
    decode(raw)
}

/// Description: Executes the encode_status_payload operation.
pub fn encode_status_payload(payload: &StatusPayload) -> Result<String, ProtocolCodecError> {
    encode(payload)
}

/// Description: Executes the decode_status_payload operation.
pub fn decode_status_payload(raw: &str) -> Result<StatusPayload, ProtocolCodecError> {
    decode(raw)
}

/// Description: Executes the encode_snapshot_payload operation.
pub fn encode_snapshot_payload(payload: &SnapshotPayload) -> Result<String, ProtocolCodecError> {
    if payload.count != payload.particles.len() as u32 {
        return Err(ProtocolCodecError::InvalidFrame("snapshot count mismatch"));
    }
    encode(payload)
}

/// Description: Executes the decode_snapshot_payload operation.
pub fn decode_snapshot_payload(raw: &str) -> Result<SnapshotPayload, ProtocolCodecError> {
    let payload: SnapshotPayload = decode(raw)?;
    if payload.count != payload.particles.len() as u32 {
        return Err(ProtocolCodecError::InvalidFrame("snapshot count mismatch"));
    }
    Ok(payload)
}
