/*
 * @file rust/blitzar-protocol/src/v1/command.rs
 * @author Luis1454
 * @project BLITZAR
 * @brief Rust protocol and gateway components for BLITZAR runtime integration.
 */

use serde::{Deserialize, Serialize};
use serde_json::Value;
use std::collections::BTreeMap;

#[derive(Clone, Debug, Default, Deserialize, PartialEq, Serialize)]
pub struct CommandRequest {
    pub cmd: String,
    #[serde(default, skip_serializing_if = "String::is_empty")]
    pub token: String,
    #[serde(flatten, default)]
    pub extra_fields: BTreeMap<String, Value>,
}
