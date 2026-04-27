// File: rust/blitzar-web-gateway/src/events.rs
// Purpose: Rust component implementation for BLITZAR runtime services.

use blitzar_protocol::v1::{SnapshotPayload, StatusPayload};
use serde::Serialize;
use serde_json::Value;

#[derive(Debug, Serialize)]
/// Description: Defines the WebEvent struct contract.
pub struct WebEvent<'a, T> {
    #[serde(rename = "type")]
    pub kind: &'a str,
    pub payload: T,
}

#[derive(Debug, Serialize)]
/// Description: Defines the WebErrorEvent struct contract.
pub struct WebErrorEvent<'a> {
    #[serde(rename = "type")]
    pub kind: &'a str,
    pub message: &'a str,
}

/// Description: Executes the status_event operation.
pub fn status_event(payload: StatusPayload) -> WebEvent<'static, StatusPayload> {
    WebEvent {
        kind: "status",
        payload,
    }
}

/// Description: Executes the snapshot_event operation.
pub fn snapshot_event(payload: SnapshotPayload) -> WebEvent<'static, SnapshotPayload> {
    WebEvent {
        kind: "snapshot",
        payload,
    }
}

/// Description: Executes the command_event operation.
pub fn command_event(payload: Value) -> WebEvent<'static, Value> {
    WebEvent {
        kind: "command_response",
        payload,
    }
}

/// Description: Executes the error_event operation.
pub fn error_event(message: &str) -> WebErrorEvent<'_> {
    WebErrorEvent {
        kind: "error",
        message,
    }
}
