# OpenTTD UDS IPC bridge plugin

Dynamic library loaded by OpenTTD that exposes the ABI-RPC interface over a Unix domain socket. Out-of-process services (for example [`ottd_grpc_server`](/contrib/grpc_server)) connect to the socket instead of linking into the GPLv2 game binary.

## Overview

| Component | Role |
|-----------|------|
| **OpenTTD + this plugin** | Runs in-process; accepts framed UDS RPC requests and dispatches them via host `RPCHandler` |
| **`ottd_grpc_server`** | Separate process; translates gRPC calls into UDS frames ([`ipc_protocol.hpp`](/contrib/ottd_uds_ipc/ipc_protocol.hpp)) |

## Configuration

| Variable | Description | Default |
|----------|-------------|---------|
| `OTTD_USE_RPC_PLUGIN` | Path to `libottd_uds_bridge` (`.dylib` / `.so`) | (none — plugin disabled) |
| `OTTD_UDS_BRIDGE_ENABLED` | Enable UDS listener (`1` or `true`) | `0` (disabled) |
| `OTTD_UDS_SOCKET_PATH` | Path to bind the UDS socket | `/run/openttd/rpc.sock` |

The socket path must match the gRPC server’s `OTTD_UDS_SOCKET_PATH`. Both processes need access to the parent directory.

## Plugin ABI (version 2)

The plugin implements the C ABI in [`plugin_interface.h`](plugin_interface.h) (`PLUGIN_API_VERSION` **2**):

| Export | Purpose |
|--------|---------|
| `GetAPIVersion()` | Must return `2` |
| `RegisterHostOps(ops)` | Host copies error strings, memory manager vtable, and `handle_rpc` into the plugin |
| `StartRPCServer()` | Binds the UDS socket when `OTTD_UDS_BRIDGE_ENABLED=1` |
| `HandleRPCCalls()` | Polls pending UDS connections; returns processed call count or negative on error |

**Plugins must not link against symbols from the OpenTTD binary.** All host interaction goes through `HostOps`:

- `get_last_error` — human-readable host error strings
- `memory` — scoped allocator (`create`, `allocate`, `deallocate`, `release_all`, `destroy`)
- `handle_rpc` — host RPC dispatcher (`RPCHandler_Handle` in OpenTTD)

## Building

From the repository root:

```bash
cd contrib/libottd_uds_bridge
mkdir -p build-debug && cd build-debug
cmake .. -DCMAKE_CXX_FLAGS=-w -DCMAKE_C_FLAGS=-w
cmake --build .
```

Library: `contrib/libottd_uds_bridge/build-debug/libottd_uds_bridge.dylib` (or `.so` on Linux).

Rebuild after any change to [`plugin_interface.h`](/src/3rdparty/extras/abi_rpc/plugin_interface.h); the copy in this directory is synced from the canonical ABI definition under `src/3rdparty/extras/abi_rpc/`.

## Request flow

```
External client (gRPC, Python, …)
        |
        v
  ottd_grpc_server
        |  UDS framed RPC (method_id + protobuf bytes)
        v
  libottd_uds_bridge::HandleRPCCalls()
        |  host_ops.handle_rpc(...)
        v
  OpenTTD abi_rpc handlers
```

## License

See [LICENSE](LICENSE). This plugin is MIT-licensed separately from OpenTTD (GPLv2).
