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
