// File: rust/blitzar-web-gateway/src/ws.rs
// Purpose: Rust component implementation for BLITZAR runtime services.

use crate::api::WebGatewayState;
use crate::error::GatewayError;
use crate::events::{command_event, error_event, snapshot_event, status_event};
use axum::extract::State;
use axum::extract::ws::{Message, WebSocket, WebSocketUpgrade};
use axum::response::IntoResponse;
use blitzar_protocol::v1::CommandRequest;
use futures_util::StreamExt;
use serde::Serialize;
use std::sync::Arc;
use tokio::time::interval;

/// Description: Executes the websocket operation.
pub async fn websocket(
    ws: WebSocketUpgrade,
    State(state): State<Arc<WebGatewayState>>,
) -> impl IntoResponse {
    ws.on_upgrade(move |socket| serve_websocket(socket, state))
}

/// Description: Executes the serve_websocket operation.
async fn serve_websocket(mut socket: WebSocket, state: Arc<WebGatewayState>) {
    if send_telemetry(&mut socket, state.as_ref()).await.is_err() {
        return;
    }
    let mut ticker = interval(state.snapshot_interval);
    loop {
        tokio::select! {
            message = socket.next() => {
                match message {
                    Some(Ok(Message::Text(text))) => {
                        if handle_command_message(&mut socket, state.as_ref(), &text).await.is_err() {
                            break;
                        }
                    }
                    Some(Ok(Message::Close(_))) | None => break,
                    Some(Ok(_)) => {}
                    Some(Err(_)) => break,
                }
            }
            _ = ticker.tick() => {
                if send_telemetry(&mut socket, state.as_ref()).await.is_err() {
                    break;
                }
            }
        }
    }
}

/// Description: Executes the handle_command_message operation.
async fn handle_command_message(
    socket: &mut WebSocket,
    state: &WebGatewayState,
    text: &str,
) -> Result<(), GatewayError> {
    let request = serde_json::from_str::<CommandRequest>(text)
        .map_err(|error| GatewayError::bad_request(error.to_string()))?;
    let response = state.backend.send_command(&request).await?;
    send_json(socket, &command_event(response)).await
}

/// Description: Executes the send_telemetry operation.
async fn send_telemetry(
    socket: &mut WebSocket,
    state: &WebGatewayState,
) -> Result<(), GatewayError> {
    match state.backend.get_status().await {
        Ok(payload) => send_json(socket, &status_event(payload)).await?,
        Err(error) => send_json(socket, &error_event(error.message())).await?,
    }
    match state.backend.get_snapshot(state.snapshot_max_points).await {
        Ok(payload) => send_json(socket, &snapshot_event(payload)).await,
        Err(error) => send_json(socket, &error_event(error.message())).await,
    }
}

/// Description: Executes the send_json operation.
async fn send_json<T>(socket: &mut WebSocket, value: &T) -> Result<(), GatewayError>
where
    T: Serialize,
{
    let payload =
        serde_json::to_string(value).map_err(|error| GatewayError::internal(error.to_string()))?;
    socket
        .send(Message::Text(payload.into()))
        .await
        .map_err(|error| GatewayError::backend(error.to_string()))
}
