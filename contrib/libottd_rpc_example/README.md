# OpenTTD RPC Example Plugin

This is an example plugin library that demonstrates how to implement the OpenTTD RPC interface.

## Overview

The example plugin showcases:
- Implementing the required plugin interface functions
- Making RPC calls to query game state
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

The plugin implements four required functions:

### `GetAPIVersion()`
Returns the plugin API version number.

### `RegisterHostOps(ops)`
Called by the host to provide host services (error strings, scoped memory manager, RPC handler, …). The plugin logs this call to stdout.

### `StartRPCServer()`
Called by the host to initialize the plugin. The plugin logs this call to stdout.

### `HandleRPCCalls()`
Called periodically by the host. Uses `HostOps::handle_rpc` to call into the game. Returns the number of RPC calls processed, or a negative value on error. The plugin:
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
├── plugin_interface.h       # C ABI interface definitions
├── plugin_interface.h       # Plugin ABI (includes HostOps)
└── proto/
    └── plugin_rpc.proto     # Protobuf definitions
```

## Implementation Details

### Memory Management

The plugin uses the scoped memory manager provided by the host for all RPC-related allocations. This ensures proper memory cleanup and prevents leaks.

### RPC Communication

1. Plugin creates a memory manager instance
2. Plugin serializes a protobuf request and allocates buffer for it
3. Plugin calls the RPC handler with the request
4. Host deserializes request, processes it, and returns serialized response
5. Plugin deserializes response and extracts data
6. Plugin destroys the memory manager, cleaning up all allocations

### State Tracking

The plugin maintains:
- Last known game mode to detect changes
- Timestamp of last idle message to output every 5 seconds

## License

This example is licensed with MIT License.
