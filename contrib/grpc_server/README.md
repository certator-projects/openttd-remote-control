# OpenTTD gRPC Server

Standalone executable that exposes a gRPC interface to OpenTTD. It runs **outside** the game process (Apache-2.0 / gRPC stack) and forwards calls to OpenTTD over a Unix domain socket via the [UDS bridge plugin](/contrib/libottd_uds_bridge) loaded in-process.

## Overview

OpenTTD’s runtime is strict GPLv2 and cannot link gRPC (Apache-2.0). The split is:

1. **[libottd_uds_bridge](/contrib/libottd_uds_bridge)** — ABI RPC plugin inside OpenTTD; listens on a local UDS and dispatches to host RPC handlers.
2. **`ottd_grpc_server`** (this directory) — separate process; accepts gRPC clients and translates each call into a framed UDS request ([`ipc_protocol.hpp`](/contrib/ottd_uds_ipc/ipc_protocol.hpp)).

Proto definitions live under [`proto/`](proto/). Each file defines one gRPC service; handlers are registered in [`grpc_server_services.cpp`](grpc_server_services.cpp).

## Configuration

### gRPC server (`ottd_grpc_server`)

| Variable | Description | Default |
|----------|-------------|---------|
| `OTTD_GRPC_ENABLED` | Enable gRPC server (`1` or `true`) | `0` (disabled) |
| `OTTD_GRPC_HOSTNAME` | Hostname/IP to bind | `0.0.0.0` |
| `OTTD_GRPC_PORT` | Port to bind | `50051` |
| `OTTD_UDS_SOCKET_PATH` | Path to the bridge UDS socket | `/run/openttd/rpc.sock` |

### OpenTTD (UDS bridge plugin)

| Variable | Description | Default |
|----------|-------------|---------|
| `OTTD_USE_RPC_PLUGIN` | Path to `libottd_uds_bridge` | (none — plugin disabled) |
| `OTTD_UDS_BRIDGE_ENABLED` | Enable UDS bridge (`1` or `true`) | `0` (disabled) |
| `OTTD_UDS_SOCKET_PATH` | Path to bind the UDS socket | `/run/openttd/rpc.sock` |

Both processes must use the **same** `OTTD_UDS_SOCKET_PATH` (and share the parent directory, e.g. via a Docker volume).

## Supported RPC Methods

All methods below are registered with the async gRPC server. Each forwards the request protobuf over UDS to the host RPC handler using the matching `RPCMethodID` from [`plugin_interface.h`](/src/3rdparty/extras/abi_rpc/plugin_interface.h).

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
| `openttd.ScriptGoal` | `New` | [`script_goal.proto`](proto/script_goal.proto) — see [Deferred goal creation](#deferred-goal-creation) |
| `openttd.ScriptGoal` | `Remove` | [`script_goal.proto`](proto/script_goal.proto) |
| `openttd.ScriptGoal` | `SetText` | [`script_goal.proto`](proto/script_goal.proto) |
| `openttd.ScriptGoal` | `SetCompleted` | [`script_goal.proto`](proto/script_goal.proto) |
| `openttd.ScriptGoal` | `IsCompleted` | [`script_goal.proto`](proto/script_goal.proto) |
| `openttd.ScriptGoal` | `Question` | [`script_goal.proto`](proto/script_goal.proto) |
| `openttd.ScriptGoal` | `QuestionClient` | [`script_goal.proto`](proto/script_goal.proto) |
| `openttd.ScriptGoal` | `CloseQuestion` | [`script_goal.proto`](proto/script_goal.proto) |
| `openttd.ScriptMap` | `GetMapSizeX` | [`script_map.proto`](proto/script_map.proto) |
| `openttd.ScriptMap` | `GetMapSizeY` | [`script_map.proto`](proto/script_map.proto) |
| `openttd.ScriptMap` | `GetTileIndex` | [`script_map.proto`](proto/script_map.proto) |

[`script_generic.proto`](proto/script_generic.proto) defines shared `GenericError` / `ErrorCode` types only; it is not exposed as a gRPC service.

Whether a call succeeds depends on game state and host-side RPC support (for example, console commands may be server-only). Failures are reported via gRPC error status and/or a `GenericError` payload from the host.

### Deferred goal creation

`ScriptGoal.New` is **asynchronous** in networked games: the game script suspends until the DoCommand completes, so the real `goal_id` is not available in the first host RPC response.

Flow:

1. gRPC `New` forwards to host `RPC_SCRIPTGOAL_NEW`.
2. When the goal is still pending, the host returns a `ScriptGoalNewAbiReply` with `deferred_result_id` (internal wire format in [`abi_internal.proto`](proto/abi_internal.proto)).
3. [`script_goal_new_call.h`](script_goal_new_call.h) polls `RPC_ABI_INTERNAL_POLL_DEFERRED_RESULT` from the completion-queue loop ([`pending_deferred_calls.cpp`](pending_deferred_calls.cpp)) until the deferred result is ready or a 30s deadline expires.
4. The gRPC client receives a single `NewGoalReply` with the final `goal_id`.

External clients do **not** call `PollDeferredResult` directly; polling is handled inside `ottd_grpc_server`.

## Building

From the repository root:

```bash
./manage.sh build-grpc_server-debug
```

Binary: `contrib/grpc_server/build-debug/ottd_grpc_server`

Protos are synced from [`src/3rdparty/extras/abi_rpc/proto/`](/src/3rdparty/extras/abi_rpc/proto/) via `update-related-protos` (run automatically by the build script).

## Testing

### Docker Compose (recommended)

[`docker-compose.yml`](/docker-compose.yml) runs OpenTTD with the UDS bridge, `grpc-server`, and optional `python-controller`:

```bash
docker compose up --build
```

gRPC is exposed on port `50051` on the `grpc-server` service.

### Manual test

1. **Start OpenTTD with the UDS bridge plugin:**

```bash
export OTTD_USE_RPC_PLUGIN=/path/to/libottd_uds_bridge.dylib   # or .so
export OTTD_UDS_BRIDGE_ENABLED=1
export OTTD_UDS_SOCKET_PATH=/tmp/openttd-rpc.sock
./openttd -D
```

2. **Start the gRPC server** (after the socket exists):

```bash
export OTTD_GRPC_ENABLED=1
export OTTD_GRPC_HOSTNAME=127.0.0.1
export OTTD_GRPC_PORT=50051
export OTTD_UDS_SOCKET_PATH=/tmp/openttd-rpc.sock
./contrib/grpc_server/build-debug/ottd_grpc_server
```

You should see:

```
[gRPC Server] Using UDS bridge at /tmp/openttd-rpc.sock
[gRPC Server] Listening on 127.0.0.1:50051
```

3. **Test with grpcurl** (from this directory):

The server is built with libprotobuf-lite, so server reflection is not available. Pass proto files explicitly:

```bash
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
grpcurl -plaintext \
  -import-path proto \
  -proto admin.proto \
  -d '{}' \
  localhost:50051 \
  openttd.Admin/GetCompanyList

grpcurl -plaintext \
  -import-path proto \
  -proto console.proto \
  -d '{"name": "difficulty.max_loan"}' \
  localhost:50051 \
  openttd.Console/Setting
```

## Architecture

```
External gRPC client
        |
        v
  ottd_grpc_server (this executable)
        |  GenericABIProxyHandler + async completion queue
        |  UDS framed RPC (OTRP protocol)
        v
  libottd_uds_bridge (OpenTTD plugin)
        |  HostOps::handle_rpc + ScopedMemoryManager (via HostOps::memory)
        v
  OpenTTD core (abi_rpc handlers)
```

The UDS bridge plugin must be built against [`plugin_interface.h`](plugin_interface.h) **API version 2**. Host services (memory manager, error strings, RPC handler) are passed through `RegisterHostOps`; plugins must not link against symbols from the OpenTTD binary.

### gRPC to host translation

Each registered method uses either `GenericABIProxyHandler` ([`call_base.h`](call_base.h)) or a dedicated async handler:

1. Accept the gRPC request on the async completion queue
2. Serialize the gRPC request protobuf
3. Send a UDS RPC frame (`method_id` + request bytes) to the bridge plugin
4. Parse the host response protobuf into the gRPC reply type
5. Map `GenericError` to gRPC status codes when present

`ScriptGoal.New` uses a dedicated handler ([`script_goal_new_call.h`](script_goal_new_call.h)) that may poll internal deferred results before finishing the client RPC.

The executable runs its own completion-queue loop in [`main.cpp`](main.cpp). On idle CQ timeouts it calls `PollPendingDeferredGrpcCalls()` so in-flight `New` requests can complete without blocking the game thread.

### Example: GetGameMode flow

```
External Client
     |
     | gRPC: GetGameMode()
     v
GenericABIProxyHandler (Admin::GetGameMode)
     |
     | UDS RPC (RPC_GET_MODE + serialized request)
     v
libottd_uds_bridge
     |
     | HostOps::handle_rpc (host RPCHandler)
     v
OpenTTD core
     |
     | GetGameModeReply over UDS
     v
gRPC GetGameModeReply → External Client
```

## Development Notes

### Adding new RPC methods

Once the corresponding abi_rpc method exists in OpenTTD:

1. **Add proto definition** under `proto/` (or extend an existing service)
2. **Add the RPC method ID** to [`plugin_interface.h`](/src/3rdparty/extras/abi_rpc/plugin_interface.h) and implement the host handler
3. **Rebuild** (`build-grpc_server-debug`) to regenerate gRPC stubs
4. **Register the handler** in [`grpc_server_services.cpp`](grpc_server_services.cpp) using `REGISTER_GRPC_HANDLER`, or a custom async handler if the call needs deferred polling (see `ScriptGoal.New`)
5. **Test** with grpcurl or a gRPC client

### Error handling

- UDS / bridge failures surface as gRPC `INTERNAL`
- Handler-level errors are returned as `GenericError` and mapped to gRPC status codes
- Log errors using spdlog (`[gRPC Server]` prefix)

## Troubleshooting

### gRPC server exits immediately

- Set `OTTD_GRPC_ENABLED=1`
- Ensure `OTTD_UDS_SOCKET_PATH` points to an existing socket (start OpenTTD + bridge first)

### Cannot connect to UDS bridge

- Confirm `OTTD_UDS_BRIDGE_ENABLED=1` and `OTTD_USE_RPC_PLUGIN` points at `libottd_uds_bridge`
- Both processes must share the same socket path (and filesystem namespace in Docker)

### gRPC server not listening

- Check that port `50051` (or `OTTD_GRPC_PORT`) is free
- Look for `[gRPC Server]` messages on stderr

### Method returns an error

- Confirm the method is listed in the table above and use the correct service name (e.g. `openttd.Admin/GetGameMode`)
- Pass proto files to grpcurl with `-import-path proto -proto <file>.proto` (reflection is not supported)
- Check OpenTTD logs for host-side RPC or `GenericError` details; many admin/console calls require a running dedicated server or specific game state

### `ScriptGoal.New` times out

- Ensure OpenTTD is running a game with an active game script and the game is advancing (deferred results resolve on a later tick)
- Rebuild `ottd_grpc_server` and `libottd_uds_bridge` against plugin API v2
- Confirm the UDS bridge plugin is loaded (`OTTD_USE_RPC_PLUGIN`, `OTTD_UDS_BRIDGE_ENABLED=1`)

## License

See [LICENSE](LICENSE). gRPC and related dependencies are Apache-2.0; this component is MIT-licensed separately from OpenTTD (GPLv2).
