# Client Module Host (`dlopen`/`LoadLibrary`)

`blitzar-client` is the dynamic client host used for client-side module development and controlled runtime composition.

Targets:
- Host: `blitzar-client`
- Module: `gravityClientModuleCli` (server control/status)
- Module: `gravityClientModuleEcho` (diagnostic echo module)
- Module: `gravityClientModuleGuiProxy` (launch/stop external GUI clients)
- Module: `gravityClientModuleQtInProc` (Qt client module using the server service)

Profile posture:
- `dev`: preferred profile for the module host; startup load plus live `reload` and `switch` are available.
- `prod`: client host is non-default and non-evidence unless explicitly reproduced under `prod` constraints; if built, only manifest-verified startup load is allowed and live `reload` / `switch` are disabled.

Module loading is not best-effort anymore. Before `LoadLibrary` / `dlopen`, the host now verifies:
- a sidecar `<module>.manifest`,
- an allowlisted `module_id`,
- matching `api_version`,
- matching product metadata,
- and the module `sha256` digest.

## Build

```bash
cmake --build build --target blitzar-client gravityClientModuleCli gravityClientModuleEcho gravityClientModuleGuiProxy gravityClientModuleQtInProc
```

Each built module emits a sidecar manifest next to the dynamic library:

```text
gravityClientModuleCli.dll
gravityClientModuleCli.dll.manifest
```

## Run

Start server daemon first:

```bash
build/blitzar-server --config simulation.ini --server-host 127.0.0.1 --server-port 4545
```

Start client host with a module:

```bash
build/blitzar-client --config simulation.ini --module cli
```

On Linux/macOS use `.so` / `.dylib` names.

If `--module` is an alias (`cli`, `echo`, `gui`, `qt`), the host enforces that the manifest `module_id` matches the alias. If `--module` is an explicit path, the host still enforces the built-in allowlist and manifest/hash checks.

## Host commands

- `modules`: list built-in aliases and resolved paths
- `module`: prints current loaded module
- `help`
- `quit`
- any other line is forwarded to current module

`dev`-only commands:
- `reload`: re-load current module from its last specifier after manifest/hash verification
- `switch <module_alias_or_path>`: unload current module and load another allowlisted module live

`prod` behavior:
- startup load remains manifest-verified
- `reload` is disabled
- `switch` is disabled

Built-in aliases:
- `cli`
- `gui`
- `echo`
- `qt`

## Example `dev` live switch

```
client-host> modules
client-host> status
client-host> switch echo
client-host> hello from echo
client-host> switch cli
client-host> status
```

## GUI live switch through dynamic module (`dev`)

Load GUI proxy module:

```bash
build/blitzar-client --config simulation.ini --module gui
```

Then:

```text
client-host> set-endpoint 127.0.0.1 4545
client-host> set-autostart false
client-host> launch path/to/your/client.exe
client-host> status
client-host> stop
client-host> launch path/to/another/client.exe
```

This lets you switch GUI clients live while the server process keeps running.

## Qt client module

Start host directly with `qt`:

```bash
build/blitzar-client --config simulation.ini --module qt
```

When launching the Qt module from a desktop shortcut or any non-interactive process, pass
`--wait-for-module` so the host stays alive until the Qt window exits:

```bash
build/blitzar-client --config simulation.ini --module qt --wait-for-module
```

For release users on Windows, use the desktop GUI installer executable from the GitHub Release:

```text
blitzar-<tag>-windows-desktop-installer.exe
```

The installer creates per-user Start Menu and desktop shortcuts and does not require administrator rights.

On Windows, the Qt module build now runs `windeployqt` automatically when that tool is available. That copies the Qt DLLs and `platforms/qwindows.dll` next to `blitzar-client.exe`, which removes the common clean-machine launch failure.

Manual fallback if needed:

```bash
make deploy-qt BUILD_DIR=build-dev
```

Runtime commands for `qt` module:

```text
status
set-endpoint 127.0.0.1 4545
set-autostart false
set-server-bin build/blitzar-server.exe
restart
wait
```

The Qt module runs inside `blitzar-client`, but it no longer embeds a backend path. All runtime control and snapshots flow through the server service connector.

The Qt frontend now exposes a modular desktop-style workspace:
- top menu bar with `File`, `Edit`, `View`, `Simulation`, `Window`, `Help`
- left control dock with `Run`, `Scene`, `Physics`, `Render`
- dockable `Telemetry` and `Validation` panes

Workspace presets can be managed from `Window > Workspace`:
- `Save Workspace...`
- `Load Workspace...`
- `Delete Workspace...`
- `Restore Default Workspace`

Saved presets are stored next to the active config under `workspace_layouts/qt/`.

In `dev`, you can still hot-switch to another module:

```text
switch cli
switch echo
switch gui
switch qt
```

In `prod`, the same host prints a policy error instead of reloading:

```text
[client-host] reload is disabled in prod profile
[client-host] switch is disabled in prod profile
```
