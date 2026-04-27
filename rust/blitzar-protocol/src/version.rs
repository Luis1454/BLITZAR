// File: rust/blitzar-protocol/src/version.rs
// Purpose: Rust component implementation for BLITZAR runtime services.

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
/// Description: Defines the ProtocolVersion enum contract.
pub enum ProtocolVersion {
    V1,
}

/// Description: Implements behavior for the associated Rust type.
impl ProtocolVersion {
    #[must_use]
    pub const fn label(self) -> &'static str {
        match self {
            Self::V1 => "server-json-v1",
        }
    }
}

pub const LATEST_PROTOCOL_VERSION: ProtocolVersion = ProtocolVersion::V1;
