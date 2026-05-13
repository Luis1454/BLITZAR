/*
 * @file rust/blitzar-web-gateway/src/args.rs
 * @author Luis1454
 * @project BLITZAR
 * @brief Rust protocol and gateway components for BLITZAR runtime integration.
 */

use std::fmt::Write;

#[derive(Clone, Debug, PartialEq)]
pub struct GatewayOptions {
    pub listen_host: String,
    pub listen_port: u16,
    pub server_host: String,
    pub server_port: u16,
    pub server_token: String,
    pub snapshot_max_points: u32,
    pub snapshot_interval_ms: u64,
}

impl Default for GatewayOptions {
    fn default() -> Self {
        Self {
            listen_host: "127.0.0.1".to_string(),
            listen_port: 8080,
            server_host: "127.0.0.1".to_string(),
            server_port: 4545,
            server_token: String::new(),
            snapshot_max_points: 4096,
            snapshot_interval_ms: 250,
        }
    }
}

impl GatewayOptions {
    pub fn listen_addr(&self) -> String {
        format!("{}:{}", self.listen_host, self.listen_port)
    }
}

fn parse_u16(value: &str, name: &str) -> Result<u16, String> {
    value
        .parse::<u16>()
        .map_err(|_| format!("invalid {name}: {value}"))
}

fn parse_u32(value: &str, name: &str) -> Result<u32, String> {
    value
        .parse::<u32>()
        .map_err(|_| format!("invalid {name}: {value}"))
}

fn parse_u64(value: &str, name: &str) -> Result<u64, String> {
    value
        .parse::<u64>()
        .map_err(|_| format!("invalid {name}: {value}"))
}

pub fn parse_args<I>(args: I) -> Result<(GatewayOptions, bool), String>
where
    I: IntoIterator<Item = String>,
{
    let mut options = GatewayOptions::default();
    let mut iter = args.into_iter();
    let _program = iter.next();
    while let Some(arg) = iter.next() {
        if arg == "--help" || arg == "-h" {
            return Ok((options, true));
        }
        let value = iter
            .next()
            .ok_or_else(|| format!("missing value for {arg}"))?;
        match arg.as_str() {
            "--listen-host" => options.listen_host = value,
            "--listen-port" => options.listen_port = parse_u16(&value, "listen port")?,
            "--server-host" => options.server_host = value,
            "--server-port" => options.server_port = parse_u16(&value, "server port")?,
            "--server-token" => options.server_token = value,
            "--snapshot-max-points" => {
                options.snapshot_max_points = parse_u32(&value, "snapshot max points")?;
            }
            "--snapshot-interval-ms" => {
                options.snapshot_interval_ms = parse_u64(&value, "snapshot interval ms")?;
            }
            _ => return Err(format!("unknown argument: {arg}")),
        }
    }
    Ok((options, false))
}

pub fn help(binary_name: &str) -> String {
    let mut output = String::new();
    let _ = writeln!(&mut output, "Usage: {binary_name} [options]");
    let _ = writeln!(&mut output);
    let _ = writeln!(&mut output, "Options:");
    let _ = writeln!(
        &mut output,
        "  --listen-host <host>           Web gateway bind host"
    );
    let _ = writeln!(
        &mut output,
        "  --listen-port <port>           Web gateway bind port"
    );
    let _ = writeln!(
        &mut output,
        "  --server-host <host>           BLITZAR server host"
    );
    let _ = writeln!(
        &mut output,
        "  --server-port <port>           BLITZAR server port"
    );
    let _ = writeln!(
        &mut output,
        "  --server-token <token>         Optional backend auth token"
    );
    let _ = writeln!(
        &mut output,
        "  --snapshot-max-points <count>  Web snapshot cap"
    );
    let _ = writeln!(
        &mut output,
        "  --snapshot-interval-ms <ms>    WebSocket telemetry period"
    );
    let _ = writeln!(
        &mut output,
        "  --help                         Show this help"
    );
    output
}
