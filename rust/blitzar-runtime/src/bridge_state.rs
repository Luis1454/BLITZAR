pub const SNAPSHOT_MIN_POINTS: u32 = 1;
pub const SNAPSHOT_DEFAULT_POINTS: u32 = 4096;
pub const SNAPSHOT_MAX_POINTS: u32 = 20000;
pub const PENDING_COMMANDS_MAX: usize = 256;

pub struct PendingCommand {
    pub cmd: String,
    pub fields: String,
}

pub struct BridgeState {
    remote_mode: bool,
    connected: bool,
    server_launched: bool,
    remote_snapshot_cap: u32,
    pending_queue_drop_warned: bool,
    pending_commands: Vec<PendingCommand>,
}

impl BridgeState {
    pub fn new(remote_mode: bool) -> Self {
        Self {
            remote_mode,
            connected: false,
            server_launched: false,
            remote_snapshot_cap: SNAPSHOT_DEFAULT_POINTS,
            pending_queue_drop_warned: false,
            pending_commands: Vec::new(),
        }
    }

    pub fn set_remote_mode(&mut self, remote_mode: bool) {
        self.remote_mode = remote_mode;
    }

    pub fn remote_mode(&self) -> bool {
        self.remote_mode
    }

    pub fn set_connected(&mut self, connected: bool) {
        self.connected = connected;
    }

    pub fn connected(&self) -> bool {
        self.connected
    }

    pub fn set_server_launched(&mut self, server_launched: bool) {
        self.server_launched = server_launched;
    }

    pub fn server_launched(&self) -> bool {
        self.server_launched
    }

    pub fn set_remote_snapshot_cap(&mut self, requested: u32) -> u32 {
        self.remote_snapshot_cap = requested.clamp(SNAPSHOT_MIN_POINTS, SNAPSHOT_MAX_POINTS);
        self.remote_snapshot_cap
    }

    pub fn remote_snapshot_cap(&self) -> u32 {
        self.remote_snapshot_cap
    }

    pub fn clear_pending_commands(&mut self) {
        self.pending_commands.clear();
        self.pending_queue_drop_warned = false;
    }

    pub fn queue_pending_command(&mut self, cmd: String, fields: String) -> bool {
        if let Some(last) = self.pending_commands.last_mut() {
            if last.cmd == cmd {
                last.fields = fields;
                return false;
            }
        }
        let mut should_warn = false;
        if self.pending_commands.len() >= PENDING_COMMANDS_MAX {
            self.pending_commands.remove(0);
            if !self.pending_queue_drop_warned {
                self.pending_queue_drop_warned = true;
                should_warn = true;
            }
        }
        self.pending_commands.push(PendingCommand { cmd, fields });
        should_warn
    }

    pub fn pending_commands(&self) -> &[PendingCommand] {
        &self.pending_commands
    }

    pub fn erase_pending_prefix(&mut self, count: usize) {
        let clamped = count.min(self.pending_commands.len());
        if clamped == 0 {
            return;
        }
        self.pending_commands.drain(0..clamped);
        if self.pending_commands.is_empty() {
            self.pending_queue_drop_warned = false;
        }
    }

    pub fn link_state_label(&self) -> &'static str {
        if !self.remote_mode {
            "local"
        } else if self.connected {
            "connected"
        } else {
            "reconnecting"
        }
    }

    pub fn server_owner_label(&self) -> &'static str {
        if !self.remote_mode {
            "embedded"
        } else if self.server_launched {
            "managed"
        } else {
            "external"
        }
    }
}
