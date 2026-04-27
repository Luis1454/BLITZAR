/*
 * @file rust/blitzar-runtime/tests/bridge_state.rs
 * @author Luis1454
 * @project BLITZAR
 * @brief Rust protocol and gateway components for BLITZAR runtime integration.
 */

use blitzar_runtime::bridge_state::BridgeState;

#[test]
fn tst_rust_runt_001_labels_follow_remote_link_state() {
    let mut state = BridgeState::new();
    assert_eq!(state.link_state_label(), "reconnecting");
    assert_eq!(state.server_owner_label(), "external");

    state.set_connected(true);
    state.set_server_launched(true);
    assert_eq!(state.link_state_label(), "connected");
    assert_eq!(state.server_owner_label(), "managed");
}

#[test]
fn tst_rust_runt_002_snapshot_cap_is_clamped_to_protocol_bounds() {
    let mut state = BridgeState::new();
    assert_eq!(state.set_remote_snapshot_cap(0), 1);
    assert_eq!(state.set_remote_snapshot_cap(999999), 20000);
}

#[test]
fn tst_rust_runt_003_queue_replaces_trailing_duplicate_command() {
    let mut state = BridgeState::new();
    assert!(!state.queue_pending_command("set_dt".to_string(), "\"value\":0.1".to_string()));
    assert!(!state.queue_pending_command("set_dt".to_string(), "\"value\":0.2".to_string()));
    assert_eq!(state.pending_commands().len(), 1);
    assert_eq!(state.pending_commands()[0].fields, "\"value\":0.2");
}

#[test]
fn tst_rust_runt_004_queue_drops_oldest_once_when_capacity_is_exceeded() {
    let mut state = BridgeState::new();
    for index in 0..256 {
        assert!(!state.queue_pending_command(format!("cmd{index}"), String::new()));
    }
    assert!(state.queue_pending_command("overflow".to_string(), String::new()));
    assert!(!state.queue_pending_command("overflow2".to_string(), String::new()));
    assert_eq!(state.pending_commands().len(), 256);
    assert_eq!(state.pending_commands()[0].cmd, "cmd2");
}
