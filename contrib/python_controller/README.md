# OpenTTD Python Controller (gRPC)

Python client/controller for an **OpenTTD gRPC server**. This repo vendors the `.proto` files from OpenTTD, generates typed Python stubs with **betterproto**, and provides a few runnable scripts under `controller/` that exercise Admin/Console/Script\* RPCs.

## Prerequisites

- **Python**: 3.14+ (this repo uses `uv`)
- **An OpenTTD gRPC server** reachable at **`127.0.0.1:50051`** (currently hardcoded in `controller/grpc_services.py`)
- **`protoc`** on your PATH (needed only when regenerating `grpc_gen/`)

## Quickstart

Install deps:

```bash
uv sync
```

Smoke-test the connection (calls `Admin.SayHello`):

```bash
uv run python -m controller.grpc_client_test
```

Run the larger end-to-end example:

```bash
uv run python -m controller.openttd_test
```

## Common tasks

Generate Python models/stubs from `grpc_protos/*.proto`:

```bash
./manage.sh generate_proto_models
```

Type-check:

```bash
./manage.sh mypy
```

Install and run pre-commit locally:

```bash
uv run pre-commit install
uv run pre-commit run -a
```

## Project layout

- **`controller/`**: hand-written code and runnable scripts
  - `grpc_services.py`: Dishka provider that creates a gRPC channel and provides typed stubs
  - `grpc_client_test.py`: minimal “hello” request against the Admin service
  - `openttd_test.py`: more complete flow (start/stop server, query map/date/companies, goals, console reset)
  - `generate_proto_models.py`: codegen driver invoked by `./manage.sh generate_proto_models`
- **`grpc_protos/`**: `.proto` sources (mirrors OpenTTD’s gRPC proto set)
- **`grpc_gen/`**: generated betterproto output (committed to the repo)

## Configuration notes

- **Server address**: `controller/grpc_services.py` currently uses `Channel(host="127.0.0.1", port=50051)`.
- **Scenario path**: `controller/openttd_test.py` starts a server with `scenario_path="sc_hilly1.scn"`; make sure the OpenTTD server can resolve that scenario (or adjust the script).

## Updating protos from an OpenTTD checkout (optional)

If you have a sibling repo at `../openttd-private`, you can copy the upstream protos into this repo:

```bash
./manage.sh copy-protos-from-openttd
./manage.sh generate_proto_models
```

## Troubleshooting

- **Connection errors**: confirm your OpenTTD gRPC server is running and listening on `127.0.0.1:50051`.
- **`protoc: command not found`**: install Protocol Buffers compiler and ensure `protoc` is on PATH.
- **Codegen mismatch**: after changing any file in `grpc_protos/`, re-run `./manage.sh generate_proto_models`.
