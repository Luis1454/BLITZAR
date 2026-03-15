use blitzar_web_gateway::api::{WebGatewayState, build_router};
use blitzar_web_gateway::args::{help, parse_args};
use blitzar_web_gateway::backend::{BackendClient, BackendConfig};
use std::process::ExitCode;
use tokio::net::TcpListener;
use tokio::time::Duration;

#[tokio::main(flavor = "current_thread")]
async fn main() -> ExitCode {
    let args = std::env::args().collect::<Vec<_>>();
    let binary_name = args
        .first()
        .cloned()
        .unwrap_or_else(|| "blitzar-web-gateway".to_string());
    let (options, show_help) = match parse_args(args) {
        Ok(result) => result,
        Err(error) => {
            eprintln!("[web-gateway] {error}");
            eprintln!("{}", help(&binary_name));
            return ExitCode::from(2);
        }
    };
    if show_help {
        print!("{}", help(&binary_name));
        return ExitCode::SUCCESS;
    }
    let backend = BackendClient::new(BackendConfig {
        host: options.server_host.clone(),
        port: options.server_port,
        token: options.server_token.clone(),
    });
    let state = WebGatewayState {
        backend,
        snapshot_max_points: options.snapshot_max_points,
        snapshot_interval: Duration::from_millis(options.snapshot_interval_ms),
    };
    let listener = match TcpListener::bind(options.listen_addr()).await {
        Ok(listener) => listener,
        Err(error) => {
            eprintln!("[web-gateway] failed to bind listener: {error}");
            return ExitCode::from(1);
        }
    };
    if let Err(error) = axum::serve(listener, build_router(state)).await {
        eprintln!("[web-gateway] server exited with error: {error}");
        return ExitCode::from(1);
    }
    ExitCode::SUCCESS
}
