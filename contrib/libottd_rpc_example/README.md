# OpenTTD RPC Example Plugin

This is an example plugin library that demonstrates how to implement the OpenTTD RPC interface.

## Overview

The example plugin showcases:
- Implementing the required plugin interface functions (API version **2**)
- Using `HostOps` for all host interaction (no linking against OpenTTD symbols)
- Making RPC calls to query game state via `HostOps::handle_rpc`
- Tracking game mode changes
- Outputting periodic status messages

## Requirements

- CMake 3.16 or later
- Protocol Buffers (protobuf) compiler and C++ library
- C++17 compatible compiler

## Building

```bash
mkdir build
cd build
cmake ..
make
```

From the repository root you can also use:

```bash
./manage.sh build-libottd_rpc_example
```

## Installation

To install to the user's local library path:

```bash
# From the build directory
cmake --install . --prefix ~/.local
```

Or for system-wide installation (requires sudo):

```bash
sudo cmake --install .
```

## Usage

Load the plugin in OpenTTD:

```bash
export OTTD_USE_RPC_PLUGIN=/path/to/libottd_rpc_example.dylib
./openttd
```

The plugin implements four required exports:

### `GetAPIVersion()`

Returns `PLUGIN_API_VERSION` (**2**). Must match the host’s expected API version or the plugin is rejected at load time.

### `RegisterHostOps(ops)`

Called once during plugin load. The host provides:

- `get_last_error` — resolve host error codes to strings
- `memory` — scoped memory manager vtable
- `handle_rpc` — call into OpenTTD RPC handlers

The plugin copies the table and must not depend on host symbols beyond these function pointers.

### `StartRPCServer()`

Called by the host to initialize the plugin. The example plugin logs this call and resets its idle timer.

### `HandleRPCCalls()`

Called periodically from the game loop. Uses `HostOps::handle_rpc` to invoke host RPCs. Returns the number of RPC calls processed, or a **negative** value on error (the host logs negative returns).

The example plugin:
- Makes a `GetMode` RPC call to query the current game mode
- Outputs a log message when the game mode changes
- Outputs "plugin idle" every 5 seconds

## Behavior

When integrated with OpenTTD, the plugin will output:

```
[ExamplePlugin] RegisterHostOps called
[ExamplePlugin] StartRPCServer called
[ExamplePlugin] Game mode changed: NEW_GAME_MENU
[ExamplePlugin] Plugin idle
[ExamplePlugin] Game mode changed: SINGLE_GAME
[ExamplePlugin] Plugin idle
...
```

## File Structure

```
libottd_rpc_example/
├── CMakeLists.txt           # Build configuration
├── README.md                # This file
├── example_plugin.cpp       # Plugin implementation
├── plugin_interface.h       # C ABI (synced from src/3rdparty/extras/abi_rpc/)
└── proto/
    ├── admin.proto          # Example RPC messages
    └── abi_internal.proto   # Internal wire types (reference copy)
```

## Implementation Details

### Memory Management

The plugin uses `HostOps::memory` for all RPC-related allocations. This ensures proper cleanup and avoids linking against host `ScopedMemoryManager_*` symbols.

### RPC Communication

1. Plugin creates a memory manager via `host_ops.memory.create`
2. Plugin serializes a protobuf request into a buffer allocated with `host_ops.memory.allocate`
3. Plugin calls `host_ops.handle_rpc(...)` with the serialized request
4. Host deserializes, processes, and returns serialized response (and optional `GenericError`)
5. Plugin deserializes the response
6. Plugin destroys the memory manager with `host_ops.memory.destroy`

### State Tracking

The plugin maintains:
- Last known game mode to detect changes
- Timestamp of last idle message to output every 5 seconds

## License

This example is licensed with MIT License.
