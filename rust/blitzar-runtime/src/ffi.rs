use crate::bridge_state::BridgeState;
use crate::bridge_state::PendingCommand;
use std::slice;

#[repr(C)]
pub struct BlitzarRuntimeStringView {
    pub data: *const u8,
    pub len: usize,
}

#[repr(C)]
pub struct BlitzarRuntimePendingCommandView {
    pub cmd: BlitzarRuntimeStringView,
    pub fields: BlitzarRuntimeStringView,
}

fn string_view(value: &str) -> BlitzarRuntimeStringView {
    BlitzarRuntimeStringView {
        data: value.as_ptr(),
        len: value.len(),
    }
}

fn pending_command_view(value: &PendingCommand) -> BlitzarRuntimePendingCommandView {
    BlitzarRuntimePendingCommandView {
        cmd: string_view(value.cmd.as_str()),
        fields: string_view(value.fields.as_str()),
    }
}

fn state_ref<'a>(state: *const BridgeState) -> Option<&'a BridgeState> {
    unsafe { state.as_ref() }
}

fn state_mut<'a>(state: *mut BridgeState) -> Option<&'a mut BridgeState> {
    unsafe { state.as_mut() }
}

fn decode_string(data: *const u8, len: usize) -> Option<String> {
    if data.is_null() && len != 0 {
        return None;
    }
    let bytes = unsafe { slice::from_raw_parts(data, len) };
    std::str::from_utf8(bytes).ok().map(ToOwned::to_owned)
}

#[unsafe(no_mangle)]
pub extern "C" fn blitzar_runtime_bridge_create(remote_mode: bool) -> *mut BridgeState {
    Box::into_raw(Box::new(BridgeState::new(remote_mode)))
}

#[unsafe(no_mangle)]
pub extern "C" fn blitzar_runtime_bridge_destroy(state: *mut BridgeState) {
    if state.is_null() {
        return;
    }
    unsafe {
        drop(Box::from_raw(state));
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn blitzar_runtime_bridge_set_remote_mode(
    state: *mut BridgeState,
    remote_mode: bool,
) {
    if let Some(state) = state_mut(state) {
        state.set_remote_mode(remote_mode);
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn blitzar_runtime_bridge_is_remote_mode(state: *const BridgeState) -> bool {
    state_ref(state)
        .map(BridgeState::remote_mode)
        .unwrap_or(false)
}

#[unsafe(no_mangle)]
pub extern "C" fn blitzar_runtime_bridge_set_connected(state: *mut BridgeState, connected: bool) {
    if let Some(state) = state_mut(state) {
        state.set_connected(connected);
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn blitzar_runtime_bridge_is_connected(state: *const BridgeState) -> bool {
    state_ref(state)
        .map(BridgeState::connected)
        .unwrap_or(false)
}

#[unsafe(no_mangle)]
pub extern "C" fn blitzar_runtime_bridge_set_server_launched(
    state: *mut BridgeState,
    launched: bool,
) {
    if let Some(state) = state_mut(state) {
        state.set_server_launched(launched);
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn blitzar_runtime_bridge_is_server_launched(state: *const BridgeState) -> bool {
    state_ref(state)
        .map(BridgeState::server_launched)
        .unwrap_or(false)
}

#[unsafe(no_mangle)]
pub extern "C" fn blitzar_runtime_bridge_set_snapshot_cap(
    state: *mut BridgeState,
    requested: u32,
) -> u32 {
    state_mut(state)
        .map(|value| value.set_remote_snapshot_cap(requested))
        .unwrap_or(0)
}

#[unsafe(no_mangle)]
pub extern "C" fn blitzar_runtime_bridge_snapshot_cap(state: *const BridgeState) -> u32 {
    state_ref(state)
        .map(BridgeState::remote_snapshot_cap)
        .unwrap_or(0)
}

#[unsafe(no_mangle)]
pub extern "C" fn blitzar_runtime_bridge_clear_pending_commands(state: *mut BridgeState) {
    if let Some(state) = state_mut(state) {
        state.clear_pending_commands();
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn blitzar_runtime_bridge_queue_pending_command(
    state: *mut BridgeState,
    cmd_data: *const u8,
    cmd_len: usize,
    fields_data: *const u8,
    fields_len: usize,
) -> bool {
    let Some(state) = state_mut(state) else {
        return false;
    };
    let Some(cmd) = decode_string(cmd_data, cmd_len) else {
        return false;
    };
    let Some(fields) = decode_string(fields_data, fields_len) else {
        return false;
    };
    state.queue_pending_command(cmd, fields)
}

#[unsafe(no_mangle)]
pub extern "C" fn blitzar_runtime_bridge_pending_command_count(state: *const BridgeState) -> usize {
    state_ref(state)
        .map(|value| value.pending_commands().len())
        .unwrap_or(0)
}

#[unsafe(no_mangle)]
pub extern "C" fn blitzar_runtime_bridge_pending_command_view(
    state: *const BridgeState,
    index: usize,
    out_view: *mut BlitzarRuntimePendingCommandView,
) -> bool {
    let Some(state) = state_ref(state) else {
        return false;
    };
    let Some(command) = state.pending_commands().get(index) else {
        return false;
    };
    let Some(out_view) = (unsafe { out_view.as_mut() }) else {
        return false;
    };
    *out_view = pending_command_view(command);
    true
}

#[unsafe(no_mangle)]
pub extern "C" fn blitzar_runtime_bridge_erase_pending_prefix(
    state: *mut BridgeState,
    count: usize,
) {
    if let Some(state) = state_mut(state) {
        state.erase_pending_prefix(count);
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn blitzar_runtime_bridge_link_state_label(
    state: *const BridgeState,
) -> BlitzarRuntimeStringView {
    state_ref(state)
        .map(|value| string_view(value.link_state_label()))
        .unwrap_or(BlitzarRuntimeStringView {
            data: std::ptr::null(),
            len: 0,
        })
}

#[unsafe(no_mangle)]
pub extern "C" fn blitzar_runtime_bridge_server_owner_label(
    state: *const BridgeState,
) -> BlitzarRuntimeStringView {
    state_ref(state)
        .map(|value| string_view(value.server_owner_label()))
        .unwrap_or(BlitzarRuntimeStringView {
            data: std::ptr::null(),
            len: 0,
        })
}
