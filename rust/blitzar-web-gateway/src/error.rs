// File: rust/blitzar-web-gateway/src/error.rs
// Purpose: Rust component implementation for BLITZAR runtime services.

use axum::Json;
use axum::http::StatusCode;
use axum::response::{IntoResponse, Response};
use serde::Serialize;

#[derive(Debug)]
/// Description: Defines the GatewayError enum contract.
pub enum GatewayError {
    Backend(String),
    BadRequest(String),
    Internal(String),
}

#[derive(Serialize)]
/// Description: Defines the ErrorBody struct contract.
struct ErrorBody {
    ok: bool,
    error: String,
}

/// Description: Implements behavior for the associated Rust type.
impl GatewayError {
    /// Description: Executes the backend operation.
    pub fn backend(message: impl Into<String>) -> Self {
        Self::Backend(message.into())
    }

    /// Description: Executes the bad_request operation.
    pub fn bad_request(message: impl Into<String>) -> Self {
        Self::BadRequest(message.into())
    }

    /// Description: Executes the internal operation.
    pub fn internal(message: impl Into<String>) -> Self {
        Self::Internal(message.into())
    }

    /// Description: Executes the message operation.
    pub fn message(&self) -> &str {
        match self {
            Self::Backend(message) | Self::BadRequest(message) | Self::Internal(message) => {
                message.as_str()
            }
        }
    }
}

/// Description: Implements behavior for the associated Rust type.
impl IntoResponse for GatewayError {
    /// Description: Executes the into_response operation.
    fn into_response(self) -> Response {
        let status = match self {
            Self::BadRequest(_) => StatusCode::BAD_REQUEST,
            Self::Backend(_) => StatusCode::BAD_GATEWAY,
            Self::Internal(_) => StatusCode::INTERNAL_SERVER_ERROR,
        };
        let body = ErrorBody {
            ok: false,
            error: self.message().to_string(),
        };
        (status, Json(body)).into_response()
    }
}
