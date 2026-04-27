// File: rust/blitzar-web-gateway/src/api.rs
// Purpose: Rust component implementation for BLITZAR runtime services.

use crate::backend::BackendClient;
use crate::error::GatewayError;
use crate::ws::websocket;
use axum::extract::{Query, State};
use axum::routing::{get, post};
use axum::{Json, Router};
use blitzar_protocol::schema;
use blitzar_protocol::v1::CommandRequest;
use serde::Deserialize;
use serde::Serialize;
use serde_json::Value;
use std::collections::BTreeMap;
use std::sync::Arc;
use tokio::time::Duration;

#[derive(Clone)]
/// Description: Defines the WebGatewayState struct contract.
pub struct WebGatewayState {
    pub backend: BackendClient,
    pub snapshot_max_points: u32,
    pub snapshot_interval: Duration,
}

#[derive(Serialize)]
/// Description: Defines the HealthPayload struct contract.
struct HealthPayload {
    ok: bool,
    service: &'static str,
    protocol: &'static str,
}

#[derive(Deserialize)]
/// Description: Defines the SnapshotQuery struct contract.
pub struct SnapshotQuery {
    pub max_points: Option<u32>,
}

#[derive(Deserialize)]
/// Description: Defines the LoadRequest struct contract.
pub struct LoadRequest {
    pub path: String,
    pub format: Option<String>,
}

#[derive(Deserialize)]
/// Description: Defines the ExportRequest struct contract.
pub struct ExportRequest {
    pub path: Option<String>,
    pub format: Option<String>,
}

/// Description: Executes the build_router operation.
pub fn build_router(state: WebGatewayState) -> Router {
    Router::new()
        .route("/healthz", get(health))
        .route("/api/v1/schema", get(schema_doc))
        .route("/api/v1/status", get(get_status))
        .route("/api/v1/snapshot", get(get_snapshot))
        .route("/api/v1/command", post(post_command))
        .route("/api/v1/load", post(post_load))
        .route("/api/v1/export", post(post_export))
        .route("/ws", get(websocket))
        .with_state(Arc::new(state))
}

/// Description: Executes the health operation.
async fn health() -> Json<HealthPayload> {
    Json(HealthPayload {
        ok: true,
        service: "blitzar-web-gateway",
        protocol: "server-json-v1",
    })
}

/// Description: Executes the schema_doc operation.
async fn schema_doc() -> Json<Value> {
    Json(schema::latest_schema_value())
}

/// Description: Executes the get_status operation.
async fn get_status(
    State(state): State<Arc<WebGatewayState>>,
) -> Result<Json<blitzar_protocol::v1::StatusPayload>, GatewayError> {
    state.backend.get_status().await.map(Json)
}

/// Description: Executes the get_snapshot operation.
async fn get_snapshot(
    State(state): State<Arc<WebGatewayState>>,
    Query(query): Query<SnapshotQuery>,
) -> Result<Json<blitzar_protocol::v1::SnapshotPayload>, GatewayError> {
    let max_points = query.max_points.unwrap_or(state.snapshot_max_points);
    state.backend.get_snapshot(max_points).await.map(Json)
}

/// Description: Executes the post_command operation.
async fn post_command(
    State(state): State<Arc<WebGatewayState>>,
    Json(request): Json<CommandRequest>,
) -> Result<Json<Value>, GatewayError> {
    state.backend.send_command(&request).await.map(Json)
}

/// Description: Executes the post_load operation.
async fn post_load(
    State(state): State<Arc<WebGatewayState>>,
    Json(request): Json<LoadRequest>,
) -> Result<Json<Value>, GatewayError> {
    let mut fields = BTreeMap::new();
    fields.insert("path".to_string(), Value::from(request.path));
    fields.insert(
        "format".to_string(),
        Value::from(request.format.unwrap_or_else(|| "auto".to_string())),
    );
    state
        .backend
        .send_named_command("load", fields)
        .await
        .map(Json)
}

/// Description: Executes the post_export operation.
async fn post_export(
    State(state): State<Arc<WebGatewayState>>,
    Json(request): Json<ExportRequest>,
) -> Result<Json<Value>, GatewayError> {
    let mut fields = BTreeMap::new();
    if let Some(path) = request.path {
        fields.insert("path".to_string(), Value::from(path));
    }
    if let Some(format) = request.format {
        fields.insert("format".to_string(), Value::from(format));
    }
    state
        .backend
        .send_named_command("export", fields)
        .await
        .map(Json)
}
