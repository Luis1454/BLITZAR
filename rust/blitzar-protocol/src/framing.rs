/*
 * @file rust/blitzar-protocol/src/framing.rs
 * @author Luis1454
 * @project BLITZAR
 * @brief Rust protocol and gateway components for BLITZAR runtime integration.
 */

use crate::codec::ProtocolCodecError;

pub fn encode_json_line(payload: &str) -> String {
    let mut framed = String::with_capacity(payload.len() + 1);
    framed.push_str(payload.trim_end_matches('\n'));
    framed.push('\n');
    framed
}

pub fn decode_json_line(frame: &str) -> Result<&str, ProtocolCodecError> {
    frame
        .strip_suffix('\n')
        .or_else(|| frame.strip_suffix("\r\n"))
        .filter(|payload| !payload.is_empty())
        .ok_or(ProtocolCodecError::InvalidFrame("missing line terminator"))
}
