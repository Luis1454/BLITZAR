# Frontend Module Host (`dlopen`/`LoadLibrary`)

This project now includes a frontend host that can load and switch frontend modules at runtime.

Targets:
- Host: `myAppModuleHost`
- Sample module 1: `gravityFrontendModuleCli` (backend control/status)
- Sample module 2: `gravityFrontendModuleEcho` (echo demo)
- Sample module 3: `gravityFrontendModuleGuiProxy` (launch/stop external GUI frontends)
- Sample module 4: `gravityFrontendModuleQtInProc` (native in-process Qt frontend)

Note: these targets are enabled by default and are the preferred architecture.

## Build

```bash
cmake --build build --target myAppModuleHost gravityFrontendModuleCli gravityFrontendModuleEcho gravityFrontendModuleGuiProxy gravityFrontendModuleQtInProc
```

## Run

Start backend daemon first:

```bash
build/myAppBackend --config simulation.ini --backend-host 127.0.0.1 --backend-port 4545
```

Start module host with a module:

```bash
build/myAppModuleHost --config simulation.ini --module cli
```

On Linux/macOS use `.so` / `.dylib` names.

## Host commands

- `modules`: list built-in aliases and resolved paths
- `module`: prints current loaded module
- `reload`: hot-reload current module from its last specifier
- `switch <module_alias_or_path>`: unloads current module and loads a new one live
- `help`
- `quit`
- any other line is forwarded to current module

Built-in aliases:
- `cli`
- `gui`
- `echo`
- `qt`

## Example live switch

```
module-host> modules
module-host> status
module-host> switch echo
module-host> hello from echo
module-host> switch cli
module-host> status
```

## GUI live switch through dynamic module

Load GUI proxy module:

```bash
build/myAppModuleHost --config simulation.ini --module gui
```

Then:

```text
module-host> set-endpoint 127.0.0.1 4545
module-host> set-autostart false
module-host> launch path/to/your/frontend.exe
module-host> status
module-host> stop
module-host> launch path/to/another/frontend.exe
```

This lets you switch GUI frontends live while the backend process keeps running.

## Native Qt in-process module

Start host directly with `qt`:

```bash
build/myAppModuleHost --config simulation.ini --module qt
```

Runtime commands for `qt` module:

```text
status
set-endpoint 127.0.0.1 4545
set-autostart false
set-backend-bin build/myAppBackend.exe
restart
```

You can still hot-switch to another module:

```text
switch cli
switch echo
switch gui
switch qt
```
