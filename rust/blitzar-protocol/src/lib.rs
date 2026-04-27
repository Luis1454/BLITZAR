// File: rust/blitzar-protocol/src/lib.rs
// Purpose: Rust component implementation for BLITZAR runtime services.

pub mod codec;
pub mod framing;
pub mod schema;
pub mod v1;
pub mod version;

pub use version::{LATEST_PROTOCOL_VERSION, ProtocolVersion};
