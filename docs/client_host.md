# Client Module Host (`dlopen`/`LoadLibrary`)

This project now includes a client host that can load and switch client modules at runtime.

Targets:
- Host: `myAppClient`
- Sample module 1: `gravityClientModuleCli` (server control/status)
- Sample module 2: `gravityClientModuleEcho` (echo demo)
- Sample module 3: `gravityClientModuleGuiProxy` (launch/stop external GUI clients)
- Sample module 4: `gravityClientModuleQtInProc` (native in-process Qt client)

Note: these targets are enabled by default and are the preferred architecture.

## Build

```bash
cmake --build build --target myAppClient gravityClientModuleCli gravityClientModuleEcho gravityClientModuleGuiProxy gravityClientModuleQtInProc
```

## Run

Start server daemon first:

```bash
build/myAppServer --config simulation.ini --server-host 127.0.0.1 --server-port 4545
```

Start client host with a module:

```bash
build/myAppClient --config simulation.ini --module cli
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
client-host> modules
client-host> status
client-host> switch echo
client-host> hello from echo
client-host> switch cli
client-host> status
```

## GUI live switch through dynamic module

Load GUI proxy module:

```bash
build/myAppClient --config simulation.ini --module gui
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

## Native Qt in-process module

Start host directly with `qt`:

```bash
build/myAppClient --config simulation.ini --module qt
```

Runtime commands for `qt` module:

```text
status
set-endpoint 127.0.0.1 4545
set-autostart false
set-server-bin build/myAppServer.exe
restart
```

You can still hot-switch to another module:

```text
switch cli
switch echo
switch gui
switch qt
```
