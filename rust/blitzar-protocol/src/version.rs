/*
 * @file rust/blitzar-protocol/src/version.rs
 * @author Luis1454
 * @project BLITZAR
 * @brief Rust protocol and gateway components for BLITZAR runtime integration.
 */

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum ProtocolVersion {
    V1,
}

impl ProtocolVersion {
    #[must_use]
    pub const fn label(self) -> &'static str {
        match self {
            Self::V1 => "server-json-v1",
        }
    }
}

pub const LATEST_PROTOCOL_VERSION: ProtocolVersion = ProtocolVersion::V1;
