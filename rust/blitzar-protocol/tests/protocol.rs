use blitzar_protocol::codec;
use blitzar_protocol::framing;
use blitzar_protocol::schema;
use blitzar_protocol::v1::{
    CommandRequest, RenderParticle, ResponseEnvelope, SnapshotPayload, StatusPayload,
};
use blitzar_protocol::{LATEST_PROTOCOL_VERSION, ProtocolVersion};
use serde_json::Value;
use serde_json::json;
use std::collections::BTreeMap;
use std::path::PathBuf;

#[test]
fn tst_rust_prot_001_round_trip_command_request_with_escaped_token() {
    let mut extra_fields = BTreeMap::new();
    extra_fields.insert("value".to_string(), json!("octree_gpu"));
    let request = CommandRequest {
        cmd: "set_solver".to_string(),
        token: "secret\"token".to_string(),
        extra_fields,
    };

    let encoded = codec::encode_command_request(&request).unwrap();
    let decoded = codec::decode_command_request(&encoded).unwrap();

    assert_eq!(decoded, request);
    assert_eq!(LATEST_PROTOCOL_VERSION, ProtocolVersion::V1);
    assert_eq!(LATEST_PROTOCOL_VERSION.label(), "server-json-v1");
}

#[test]
fn tst_rust_prot_002_round_trip_status_payload() {
    let payload = StatusPayload {
        envelope: ResponseEnvelope {
            ok: true,
            cmd: "status".to_string(),
            error: String::new(),
        },
        steps: 42,
        dt: 0.02,
        paused: true,
        faulted: true,
        fault_step: 41,
        fault_reason: "bad\nstate".to_string(),
        sph_enabled: true,
        server_fps: 144.5,
        performance_profile: "interactive".to_string(),
        substep_target_dt: 0.0025,
        substep_dt: 0.00125,
        substeps: 8,
        max_substeps: 32,
        snapshot_publish_period_ms: 50,
        particle_count: 128,
        solver: "octree_gpu".to_string(),
        integrator: "euler".to_string(),
        kinetic_energy: 1.0,
        potential_energy: -2.0,
        thermal_energy: 3.0,
        radiated_energy: 4.0,
        total_energy: 5.0,
        energy_drift_pct: 0.125,
        energy_estimated: true,
        gpu_telemetry_enabled: true,
        gpu_telemetry_available: true,
        gpu_kernel_ms: 1.5,
        gpu_copy_ms: 0.75,
        gpu_vram_used_bytes: 64 * 1024 * 1024,
        gpu_vram_total_bytes: 256 * 1024 * 1024,
    };

    let encoded = codec::encode_status_payload(&payload).unwrap();
    let framed = framing::encode_json_line(&encoded);
    let decoded =
        codec::decode_status_payload(framing::decode_json_line(&framed).unwrap()).unwrap();

    assert_eq!(decoded, payload);
}

#[test]
fn tst_rust_prot_003_rejects_malformed_snapshot_payload() {
    let raw = "{\"ok\":true,\"cmd\":\"get_snapshot\",\"has_snapshot\":true,\"count\":1,\"particles\":[[1,2,3,4,5]]}";
    let error = codec::decode_snapshot_payload(raw).unwrap_err().to_string();
    assert!(error.contains("invalid length") || error.contains("expected"));
}

#[test]
fn tst_rust_prot_004_round_trip_snapshot_and_error_envelope() {
    let snapshot = SnapshotPayload {
        envelope: ResponseEnvelope {
            ok: true,
            cmd: "get_snapshot".to_string(),
            error: String::new(),
        },
        has_snapshot: true,
        count: 1,
        particles: vec![RenderParticle {
            x: 1.0,
            y: 2.0,
            z: 3.0,
            mass: 4.0,
            pressure_norm: 5.0,
            temperature: 6.0,
        }],
    };
    let encoded_snapshot = codec::encode_snapshot_payload(&snapshot).unwrap();
    let decoded_snapshot = codec::decode_snapshot_payload(&encoded_snapshot).unwrap();
    assert_eq!(decoded_snapshot, snapshot);

    let envelope = ResponseEnvelope {
        ok: false,
        cmd: "set_integrator".to_string(),
        error: "invalid integrator value".to_string(),
    };
    let encoded_envelope = codec::encode_response_envelope(&envelope).unwrap();
    let decoded_envelope = codec::decode_response_envelope(&encoded_envelope).unwrap();
    assert_eq!(decoded_envelope, envelope);
}

#[test]
fn tst_rust_prot_005_latest_schema_matches_committed_contract_artifact() {
    let schema_path = PathBuf::from(env!("CARGO_MANIFEST_DIR"))
        .join("..")
        .join("..")
        .join("docs")
        .join("server_protocol_schema.json");
    let committed: Value =
        serde_json::from_str(&std::fs::read_to_string(schema_path).unwrap()).unwrap();
    assert_eq!(committed, schema::latest_schema_value());
}

#[test]
fn tst_rust_prot_006_latest_schema_exposes_compatibility_and_deprecation_rules() {
    let schema = schema::latest_schema_value();
    assert_eq!(schema["schema"], LATEST_PROTOCOL_VERSION.label());
    assert_eq!(
        schema["deprecation"]["minimum_support_window"],
        "one release"
    );
    assert_eq!(schema["deprecation"]["removal_requires_new_schema"], true);
    assert!(schema["commands"].get("status").is_some());
    assert!(schema["commands"].get("get_snapshot").is_some());
    assert!(
        schema["commands"]
            .get("set_snapshot_publish_cadence")
            .is_some()
    );
    assert!(schema["commands"].get("set_gpu_telemetry").is_some());
    assert!(
        schema["compatibility"]["breaking_changes"]
            .as_array()
            .unwrap()
            .iter()
            .any(|value| value == "change auth behavior")
    );
}
