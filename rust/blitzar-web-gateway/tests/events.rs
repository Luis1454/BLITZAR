use blitzar_protocol::v1::{ResponseEnvelope, SnapshotPayload, StatusPayload};
use blitzar_web_gateway::events::{snapshot_event, status_event};

#[test]
fn tst_rust_web_005_event_helpers_emit_typed_status_and_snapshot_tags() {
    let status = status_event(StatusPayload {
        envelope: ResponseEnvelope {
            ok: true,
            cmd: "status".to_string(),
            error: String::new(),
        },
        steps: 11,
        ..StatusPayload::default()
    });
    let snapshot = snapshot_event(SnapshotPayload {
        envelope: ResponseEnvelope {
            ok: true,
            cmd: "get_snapshot".to_string(),
            error: String::new(),
        },
        has_snapshot: true,
        count: 0,
        particles: Vec::new(),
    });
    let status_json = serde_json::to_string(&status).expect("status");
    let snapshot_json = serde_json::to_string(&snapshot).expect("snapshot");
    assert!(status_json.contains("\"type\":\"status\""));
    assert!(snapshot_json.contains("\"type\":\"snapshot\""));
}
