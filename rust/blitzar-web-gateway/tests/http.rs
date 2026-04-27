/*
 * @file rust/blitzar-web-gateway/tests/http.rs
 * @author Luis1454
 * @project BLITZAR
 * @brief Rust protocol and gateway components for BLITZAR runtime integration.
 */

use axum::body::Body;
use axum::http::{Method, Request, StatusCode};
use blitzar_web_gateway::api::{WebGatewayState, build_router};
use blitzar_web_gateway::backend::{BackendClient, BackendConfig};
use http_body_util::BodyExt;
use tokio::io::{AsyncBufReadExt, AsyncWriteExt, BufReader};
use tokio::net::TcpListener;
use tokio::runtime::Builder;
use tokio::time::Duration;
use tower::ServiceExt;

async fn spawn_mock_backend(response: &'static str) -> u16 {
    let listener = TcpListener::bind("127.0.0.1:0").await.expect("bind");
    let port = listener.local_addr().expect("local addr").port();
    tokio::spawn(async move {
        let (stream, _) = listener.accept().await.expect("accept");
        let mut reader = BufReader::new(stream);
        let mut line = String::new();
        reader.read_line(&mut line).await.expect("read");
        let mut stream = reader.into_inner();
        stream.write_all(response.as_bytes()).await.expect("write");
        stream.write_all(b"\n").await.expect("newline");
    });
    port
}

#[test]
fn tst_rust_web_003_http_status_route_proxies_backend_status_payload() {
    Builder::new_current_thread()
        .enable_all()
        .build()
        .expect("runtime")
        .block_on(async {
            let port = spawn_mock_backend(
                r#"{"ok":true,"cmd":"status","steps":7,"dt":0.1,"paused":false,"faulted":false}"#,
            )
            .await;
            let state = WebGatewayState {
                backend: BackendClient::new(BackendConfig {
                    host: "127.0.0.1".to_string(),
                    port,
                    token: String::new(),
                }),
                snapshot_max_points: 1024,
                snapshot_interval: Duration::from_millis(250),
            };
            let response = build_router(state)
                .oneshot(
                    Request::builder()
                        .method(Method::GET)
                        .uri("/api/v1/status")
                        .body(Body::empty())
                        .expect("request"),
                )
                .await
                .expect("response");
            assert_eq!(response.status(), StatusCode::OK);
            let body = response.into_body().collect().await.expect("body");
            let text = String::from_utf8(body.to_bytes().to_vec()).expect("utf8");
            assert!(text.contains("\"steps\":7"));
            assert!(text.contains("\"cmd\":\"status\""));
        });
}

#[test]
fn tst_rust_web_004_http_command_route_preserves_backend_error_envelope() {
    Builder::new_current_thread()
        .enable_all()
        .build()
        .expect("runtime")
        .block_on(async {
            let port =
                spawn_mock_backend(r#"{"ok":false,"cmd":"set_solver","error":"invalid solver"}"#)
                    .await;
            let state = WebGatewayState {
                backend: BackendClient::new(BackendConfig {
                    host: "127.0.0.1".to_string(),
                    port,
                    token: String::new(),
                }),
                snapshot_max_points: 1024,
                snapshot_interval: Duration::from_millis(250),
            };
            let response = build_router(state)
                .oneshot(
                    Request::builder()
                        .method(Method::POST)
                        .uri("/api/v1/command")
                        .header("content-type", "application/json")
                        .body(Body::from(r#"{"cmd":"set_solver","value":"bad"}"#))
                        .expect("request"),
                )
                .await
                .expect("response");
            assert_eq!(response.status(), StatusCode::OK);
            let body = response.into_body().collect().await.expect("body");
            let text = String::from_utf8(body.to_bytes().to_vec()).expect("utf8");
            assert!(text.contains("\"ok\":false"));
            assert!(text.contains("\"invalid solver\""));
        });
}
