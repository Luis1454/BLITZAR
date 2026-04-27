// File: rust/blitzar-protocol/src/framing.rs
// Purpose: Rust component implementation for BLITZAR runtime services.

use crate::codec::ProtocolCodecError;

/// Description: Executes the encode_json_line operation.
pub fn encode_json_line(payload: &str) -> String {
    let mut framed = String::with_capacity(payload.len() + 1);
    framed.push_str(payload.trim_end_matches('\n'));
    framed.push('\n');
    framed
}

/// Description: Executes the decode_json_line operation.
pub fn decode_json_line(frame: &str) -> Result<&str, ProtocolCodecError> {
    frame
        .strip_suffix('\n')
        .or_else(|| frame.strip_suffix("\r\n"))
        .filter(|payload| !payload.is_empty())
        .ok_or(ProtocolCodecError::InvalidFrame("missing line terminator"))
}
