/*
 * @file rust/blitzar-protocol/src/v1/envelope.rs
 * @author Luis1454
 * @project BLITZAR
 * @brief Rust protocol and gateway components for BLITZAR runtime integration.
 */

use serde::{Deserialize, Serialize};

#[derive(Clone, Debug, Default, Deserialize, PartialEq, Serialize)]
pub struct ResponseEnvelope {
    pub ok: bool,
    pub cmd: String,
    #[serde(default, skip_serializing_if = "String::is_empty")]
    pub error: String,
}
