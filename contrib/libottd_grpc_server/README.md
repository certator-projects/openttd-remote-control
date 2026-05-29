# OpenTTD gRPC Server Plugin

This plugin provides a gRPC interface to OpenTTD by acting as a bridge between external gRPC clients and OpenTTD's C-ABI RPC interface.

## Overview

The plugin implements the abi_rpc plugin interface and translates gRPC calls into abi_rpc calls that OpenTTD's core can handle. This allows external applications to control and query OpenTTD via gRPC without modifying the core game code.

Proto definitions live under [`proto/`](proto/). Each file defines one gRPC service; handlers are registered in [`grpc_server_services.cpp`](grpc_server_services.cpp).

## Configuration

The plugin is configured via environment variables:

| Variable | Description | Default |
|----------|-------------|---------|
| `OTTD_USE_RPC_PLUGIN` | Path to the plugin library | (none - plugin disabled) |
| `OTTD_GRPC_ENABLED` | Enable gRPC server (1 or true) | `0` (disabled) |
| `OTTD_GRPC_HOSTNAME` | Hostname to bind the server to | `0.0.0.0` |
| `OTTD_GRPC_PORT` | Port to bind the server to | `50051` |

## Supported RPC Methods

All methods below are registered with the async gRPC server. Each forwards the request protobuf to the host RPC handler via the matching `RPCMethodID` from [`plugin_interface.h`](/src/3rdparty/extras/abi_rpc/plugin_interface.h).

| Service | Method | Proto file |
|---------|--------|------------|
| `openttd.Admin` | `GetGameMode` | [`admin.proto`](proto/admin.proto) |
| `openttd.Admin` | `StartNetworkServer` | [`admin.proto`](proto/admin.proto) |
| `openttd.Admin` | `GetNetworkClients` | [`admin.proto`](proto/admin.proto) |
| `openttd.Admin` | `SendChatMessage` | [`admin.proto`](proto/admin.proto) |
| `openttd.Admin` | `SendPrivateChatMessage` | [`admin.proto`](proto/admin.proto) |
| `openttd.Admin` | `PollLastChatMessages` | [`admin.proto`](proto/admin.proto) |
| `openttd.Admin` | `GetCompanyList` | [`admin.proto`](proto/admin.proto) |
| `openttd.Console` | `ResetCompany` | [`console.proto`](proto/console.proto) |
| `openttd.Console` | `SettingNewgame` | [`console.proto`](proto/console.proto) |
| `openttd.Console` | `Setting` | [`console.proto`](proto/console.proto) |
| `openttd.Console` | `Restart` | [`console.proto`](proto/console.proto) |
| `openttd.ScriptCompany` | `GetQuarterlyCompanyValue` | [`script_company.proto`](proto/script_company.proto) |
| `openttd.ScriptGoal` | `New` | [`script_goal.proto`](proto/script_goal.proto) |
| `openttd.ScriptMap` | `GetMapSizeX` | [`script_map.proto`](proto/script_map.proto) |
| `openttd.ScriptMap` | `GetMapSizeY` | [`script_map.proto`](proto/script_map.proto) |
| `openttd.ScriptMap` | `GetTileIndex` | [`script_map.proto`](proto/script_map.proto) |
| `openttd.ScriptGeneric` | `GetLastIntResult` | [`script_generic.proto`](proto/script_generic.proto) |

Whether a call succeeds depends on game state and host-side RPC support (for example, console commands may be server-only). Failures are reported via gRPC error status and/or a `GenericError` payload from the host.

## Testing

### Manual Test

1. **Set environment variables:**

```bash
export OTTD_USE_RPC_PLUGIN=/path/to/libottd_grpc_server.dylib/so
export OTTD_GRPC_ENABLED=1
export OTTD_GRPC_HOSTNAME=127.0.0.1
export OTTD_GRPC_PORT=50051
```

2. **Run OpenTTD:**

```bash
./openttd
```

You should see a log message indicating the gRPC server has started:
```
[gRPC Plugin] gRPC server started and listening on 127.0.0.1:50051
```

3. **Test with grpcurl** (in another terminal, from this directory):

The plugin is built with libprotobuf-lite, so server reflection is not available. Pass the proto files explicitly:

```bash
# Query current game mode
grpcurl -plaintext \
  -import-path proto \
  -proto admin.proto \
  -d '{}' \
  localhost:50051 \
  openttd.Admin/GetGameMode
```

Expected output for GetGameMode:
```json
{
  "currentMode": "NEW_GAME_MENU",
  "isDedicated": false,
  "serverName": ""
}
```

Field names in JSON follow proto3 JSON mapping (`current_mode` → `currentMode`, `is_dedicated` → `isDedicated`, `server_name` → `serverName`).

Additional examples:

```bash
# List companies (when in a game)
grpcurl -plaintext \
  -import-path proto \
  -proto admin.proto \
  -d '{}' \
  localhost:50051 \
  openttd.Admin/GetCompanyList

# Read a setting
grpcurl -plaintext \
  -import-path proto \
  -proto console.proto \
  -d '{"name": "difficulty.max_loan"}' \
  localhost:50051 \
  openttd.Console/Setting
```

## Architecture

### Plugin Interface

The plugin implements the standard abi_rpc plugin interface:

- `GetAPIVersion()` - Returns plugin API version
- `PluginGetLastError()` - Provides error descriptions
- `RegisterHostOps()` - Receives host operation table (`HostOps`: errors, memory, …)
- `StartRPCServer()` - Starts the async gRPC server when enabled via environment variables
- `HandleRPCCalls()` - Non-blocking drain of the gRPC completion queue; returns the number of events processed this tick

### gRPC to abi_rpc Translation

Each registered method uses `GenericABIProxyHandler` ([`call_base.h`](call_base.h)):

1. Accept the gRPC request on the async completion queue
2. Create a scoped memory manager for the call
3. Serialize the gRPC request protobuf and call the host RPC handler with the appropriate `RPCMethodID`
4. Parse the host response protobuf into the gRPC reply type
5. Map `GenericError` to gRPC status codes when present
6. Clean up the scoped memory manager

### Example: GetGameMode Flow

```
External Client
     |
     | gRPC call: GetGameMode()
     v
GenericABIProxyHandler (Admin::GetGameMode)
     |
     | 1. Create ScopedMemoryManager
     | 2. Serialize GetGameModeRequest
     v
host RPC handler (RPC_GET_MODE)
     |
     | OpenTTD core processes request
     v
GetGameModeReply
     |
     | 3. Parse response
     | 4. Finish async gRPC call
     v
GetGameModeReply → External Client
```

## Development Notes

### Adding New RPC Methods

To add a new gRPC method once the corresponding abi_rpc method is available:

1. **Add proto definition** in the appropriate file under `proto/` (or create a new proto + service if needed)
2. **Add the RPC method ID** to [`plugin_interface.h`](/src/3rdparty/extras/abi_rpc/plugin_interface.h) and implement the host handler
3. **Rebuild** to generate new gRPC stubs
4. **Register the handler** in [`grpc_server_services.cpp`](grpc_server_services.cpp) using `REGISTER_GRPC_HANDLER`
5. **Test** using grpcurl or a gRPC client

### Memory Management

All memory used when calling the RPC handler must be allocated via the `ScopedMemoryManager` provided by the host. This ensures proper cleanup and prevents memory leaks across the plugin boundary.

### Error Handling

- Host RPC failures use `g_host_ops.get_last_error()` for detailed descriptions
- Handler-level errors are returned as `GenericError` and mapped to gRPC status codes
- Plugin-internal failures return gRPC `INTERNAL`
- Log errors using spdlog

## Troubleshooting

### Plugin not loading

- Check that `OTTD_USE_RPC_PLUGIN` points to the correct library path
- Verify the plugin was built for the correct platform
- Check OpenTTD logs for plugin loading errors

### gRPC server not starting

- Ensure `OTTD_GRPC_ENABLED=1` is set
- Check that the port is not already in use
- Look for error messages in OpenTTD console output

### Method returns an error

- Confirm the method is listed in the table above and you are calling the correct service name (for example `openttd.Admin/GetGameMode`)
- Pass proto files to grpcurl with `-import-path proto -proto <file>.proto` (reflection is not supported)
- Check OpenTTD logs for host-side RPC or `GenericError` details; many console/admin calls require a running dedicated server or specific game state
