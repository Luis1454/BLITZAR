use serde::{Deserialize, Serialize};

#[derive(Clone, Debug, Default, Deserialize, PartialEq, Serialize)]
pub struct ResponseEnvelope {
    pub ok: bool,
    pub cmd: String,
    #[serde(default, skip_serializing_if = "String::is_empty")]
    pub error: String,
}
