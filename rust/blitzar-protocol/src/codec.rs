use crate::v1::{CommandRequest, ResponseEnvelope, SnapshotPayload, StatusPayload};
use serde::Serialize;
use serde::de::DeserializeOwned;

#[derive(Debug)]
pub enum ProtocolCodecError {
    Json(serde_json::Error),
    InvalidFrame(&'static str),
}

impl std::fmt::Display for ProtocolCodecError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            Self::Json(error) => write!(f, "{error}"),
            Self::InvalidFrame(message) => write!(f, "{message}"),
        }
    }
}

impl std::error::Error for ProtocolCodecError {}

impl From<serde_json::Error> for ProtocolCodecError {
    fn from(value: serde_json::Error) -> Self {
        Self::Json(value)
    }
}

fn encode<T>(value: &T) -> Result<String, ProtocolCodecError>
where
    T: Serialize,
{
    serde_json::to_string(value).map_err(ProtocolCodecError::from)
}

fn decode<T>(raw: &str) -> Result<T, ProtocolCodecError>
where
    T: DeserializeOwned,
{
    serde_json::from_str(raw).map_err(ProtocolCodecError::from)
}

pub fn encode_command_request(request: &CommandRequest) -> Result<String, ProtocolCodecError> {
    encode(request)
}

pub fn decode_command_request(raw: &str) -> Result<CommandRequest, ProtocolCodecError> {
    decode(raw)
}

pub fn encode_response_envelope(envelope: &ResponseEnvelope) -> Result<String, ProtocolCodecError> {
    encode(envelope)
}

pub fn decode_response_envelope(raw: &str) -> Result<ResponseEnvelope, ProtocolCodecError> {
    decode(raw)
}

pub fn encode_status_payload(payload: &StatusPayload) -> Result<String, ProtocolCodecError> {
    encode(payload)
}

pub fn decode_status_payload(raw: &str) -> Result<StatusPayload, ProtocolCodecError> {
    decode(raw)
}

pub fn encode_snapshot_payload(payload: &SnapshotPayload) -> Result<String, ProtocolCodecError> {
    if payload.count != payload.particles.len() as u32 {
        return Err(ProtocolCodecError::InvalidFrame("snapshot count mismatch"));
    }
    encode(payload)
}

pub fn decode_snapshot_payload(raw: &str) -> Result<SnapshotPayload, ProtocolCodecError> {
    let payload: SnapshotPayload = decode(raw)?;
    if payload.count != payload.particles.len() as u32 {
        return Err(ProtocolCodecError::InvalidFrame("snapshot count mismatch"));
    }
    Ok(payload)
}
