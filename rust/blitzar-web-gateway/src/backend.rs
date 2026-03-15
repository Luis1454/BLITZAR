use crate::error::GatewayError;
use blitzar_protocol::codec;
use blitzar_protocol::v1::{CommandRequest, ResponseEnvelope, SnapshotPayload, StatusPayload};
use serde_json::Value;
use std::collections::BTreeMap;
use tokio::io::{AsyncBufReadExt, AsyncWriteExt, BufReader};
use tokio::net::TcpStream;

#[derive(Clone, Debug, PartialEq)]
pub struct BackendConfig {
    pub host: String,
    pub port: u16,
    pub token: String,
}

impl BackendConfig {
    pub fn socket_addr(&self) -> String {
        format!("{}:{}", self.host, self.port)
    }
}

#[derive(Clone, Debug)]
pub struct BackendClient {
    config: BackendConfig,
}

impl BackendClient {
    pub fn new(config: BackendConfig) -> Self {
        Self { config }
    }

    pub async fn send_command(&self, request: &CommandRequest) -> Result<Value, GatewayError> {
        let mut request = request.clone();
        if !self.config.token.is_empty() {
            request.token = self.config.token.clone();
        }
        let raw = codec::encode_command_request(&request)
            .map_err(|error| GatewayError::internal(error.to_string()))?;
        let mut stream = TcpStream::connect(self.config.socket_addr())
            .await
            .map_err(|error| GatewayError::backend(error.to_string()))?;
        stream
            .write_all(raw.as_bytes())
            .await
            .map_err(|error| GatewayError::backend(error.to_string()))?;
        stream
            .write_all(b"\n")
            .await
            .map_err(|error| GatewayError::backend(error.to_string()))?;
        let mut reader = BufReader::new(stream);
        let mut line = String::new();
        reader
            .read_line(&mut line)
            .await
            .map_err(|error| GatewayError::backend(error.to_string()))?;
        if line.trim().is_empty() {
            return Err(GatewayError::backend("backend returned empty response"));
        }
        serde_json::from_str::<Value>(line.trim())
            .map_err(|error| GatewayError::backend(error.to_string()))
    }

    pub async fn get_status(&self) -> Result<StatusPayload, GatewayError> {
        let response = self.send_named_command("status", BTreeMap::new()).await?;
        self.parse_typed_response::<StatusPayload>(response)
    }

    pub async fn get_snapshot(&self, max_points: u32) -> Result<SnapshotPayload, GatewayError> {
        let mut fields = BTreeMap::new();
        fields.insert("max_points".to_string(), Value::from(max_points));
        let response = self.send_named_command("get_snapshot", fields).await?;
        self.parse_typed_response::<SnapshotPayload>(response)
    }

    pub async fn send_named_command(
        &self,
        command: &str,
        extra_fields: BTreeMap<String, Value>,
    ) -> Result<Value, GatewayError> {
        self.send_command(&CommandRequest {
            cmd: command.to_string(),
            token: String::new(),
            extra_fields,
        })
        .await
    }

    fn parse_typed_response<T>(&self, value: Value) -> Result<T, GatewayError>
    where
        T: serde::de::DeserializeOwned,
    {
        let envelope = serde_json::from_value::<ResponseEnvelope>(value.clone())
            .map_err(|error| GatewayError::backend(error.to_string()))?;
        if !envelope.ok {
            return Err(GatewayError::backend(envelope.error));
        }
        serde_json::from_value::<T>(value).map_err(|error| GatewayError::backend(error.to_string()))
    }
}
