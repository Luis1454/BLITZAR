// File: rust/blitzar-web-gateway/tests/args.rs
// Purpose: Rust component implementation for BLITZAR runtime services.

use blitzar_web_gateway::args::{GatewayOptions, parse_args};

#[test]
fn tst_rust_web_001_parse_args_supports_gateway_and_backend_flags() {
    let args = vec![
        "blitzar-web-gateway".to_string(),
        "--listen-port".to_string(),
        "9090".to_string(),
        "--server-host".to_string(),
        "10.0.0.5".to_string(),
        "--snapshot-max-points".to_string(),
        "2048".to_string(),
    ];
    let (options, show_help) = parse_args(args).expect("args should parse");
    assert!(!show_help);
    assert_eq!(
        options,
        GatewayOptions {
            listen_port: 9090,
            server_host: "10.0.0.5".to_string(),
            snapshot_max_points: 2048,
            ..GatewayOptions::default()
        }
    );
}

#[test]
fn tst_rust_web_002_parse_args_rejects_unknown_flag() {
    let args = vec![
        "blitzar-web-gateway".to_string(),
        "--mystery".to_string(),
        "value".to_string(),
    ];
    let error = parse_args(args).expect_err("unknown flags must fail");
    assert!(error.contains("unknown argument"));
}
