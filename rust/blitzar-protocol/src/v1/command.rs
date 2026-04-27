// File: rust/blitzar-protocol/src/v1/command.rs
// Purpose: Rust component implementation for BLITZAR runtime services.

use serde::{Deserialize, Serialize};
use serde_json::Value;
use std::collections::BTreeMap;

#[derive(Clone, Debug, Default, Deserialize, PartialEq, Serialize)]
/// Description: Defines the CommandRequest struct contract.
pub struct CommandRequest {
    pub cmd: String,
    #[serde(default, skip_serializing_if = "String::is_empty")]
    pub token: String,
    #[serde(flatten, default)]
    pub extra_fields: BTreeMap<String, Value>,
}
