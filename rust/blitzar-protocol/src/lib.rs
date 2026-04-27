/*
 * @file rust/blitzar-protocol/src/lib.rs
 * @author Luis1454
 * @project BLITZAR
 * @brief Rust protocol and gateway components for BLITZAR runtime integration.
 */

pub mod codec;
pub mod framing;
pub mod schema;
pub mod v1;
pub mod version;

pub use version::{LATEST_PROTOCOL_VERSION, ProtocolVersion};
