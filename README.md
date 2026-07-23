# OpenTTD Remote Control & External Scripting API

An experimental runtime integration layer that enables external remote control and multi-language scripting for OpenTTD dedicated servers. 

Instead of writing game logic internally in Squirrel, this project provides a stable API bridge (via gRPC and protobuf) allowing you to:
* **Control the server externally:** Automate admin tasks and server management.
* **Write scripts in Python (or other languages):** Interact with the game state out-of-process.
* **Keep the game core clean:** Uses a dynamically loaded runtime plugin to avoid modifying the OpenTTD binary.

*Example connecting to the server:*

```python
import asyncio
from grpclib.client import Channel
import grpc_gen.admin.openttd as admin

async def main() -> None:
    channel = Channel("127.0.0.1", 50051)
    stub = admin.AdminStub(channel)

    await stub.start_network_server(admin.StartNetworkServerRequest(
        server_name="My Server",
        scenario_path="example.scn",
    ))
    await stub.send_chat_message(admin.SendChatMessageRequest(
        message="Hello from Python!",
    ))
    channel.close()

asyncio.run(main())
```

## Fork notes

This fork is based on upstream [OpenTTD](https://github.com/OpenTTD/OpenTTD) [**15.3**](https://github.com/OpenTTD/OpenTTD/tree/15.3) (commit `14ec60f`), not `master`, so the dedicated server stays compatible with the official client.

Changes since that baseline:

* **ABI RPC layer** (`src/3rdparty/extras/abi_rpc/`) — proof-of-concept runtime plugin interface with protobuf-defined messages, enabling **external control via ABI** instead of writing game logic in Squirrel.
* **Runtime plugins** — `libottd_rpc_example` (minimal demo) and `libottd_uds_bridge` (Unix domain socket IPC into OpenTTD).
* **gRPC server** — standalone `ottd_grpc_server` executable (`contrib/grpc_server/`) that talks to the bridge over UDS; keeps Apache-2.0 gRPC out of the game process.
* **Python controller** — example external gRPC client (`contrib/python_controller/`).
* **Docker Compose** — OpenTTD + UDS bridge, `grpc-server`, and optional `python-controller` in one stack ([`docker-compose.yml`](/docker-compose.yml)).

This fork is **not intended for contribution** to [official OpenTTD](https://github.com/openttd/openttd): modifications were co-authored with an LLM (which violates upstream contribution policy), and the extra code would add maintenance burden upstream is not set up to carry.

Upstream `.github` CI/workflows were removed for the same reason — this is a standalone research fork with no plan to merge back.

ABI protobuf definitions, plugin interface headers, example plugin implementations, and Python controller scripts are licensed under MIT (see also [Licensing](#30-licensing) below for upstream components).

---

## Overview

This repository demonstrates:

* runtime loading of external `.so` plugins into the OpenTTD process — to keep OpenTTD core GPLv2-clean while experimenting with external controllers,
* communication through a stable ABI and protobuf-defined interfaces — so plugins do not depend on OpenTTD internal headers or data structures,
* optional gRPC-based control integration — for language-agnostic remote clients without linking gRPC into OpenTTD itself,
* separation between the OpenTTD source tree and external runtime modules.

The repository intentionally separates:

| Component               | License      | Note |
| ----------------------- | ------------ | ---- |
| OpenTTD                 | GPLv2        | |
| gRPC (`contrib/grpc_server`) | Apache-2.0   | separate process/container; MIT-licensed wrapper code |
| protobuf                | BSD-3-Clause | |
| UDS IPC + bridge plugin | MIT          | `contrib/ottd_uds_ipc`, `contrib/libottd_uds_bridge` |
| runtime plugin examples | MIT          | |

The default configuration is designed to remain GPLv2-compatible and does **not** link OpenTTD against Apache-2.0 gRPC components.

---

# Repository Layout

```text
.
├── docker-compose.yml              # OpenTTD + UDS bridge + grpc-server + python-controller
├── Dockerfile                      # OpenTTD image (GPLv2-compatible deps)
├── Dockerfile.cached               # Optimised rebuilds (BuildKit cache mounts + slimmer runtime)
├── contrib/
│   ├── content/scenario/           # example scenario files (.scn)
│   ├── grpc_server/                # standalone gRPC executable (+ pixi env for Docker build)
│   ├── libottd_uds_bridge/         # UDS IPC bridge plugin (loaded in OpenTTD)
│   ├── libottd_rpc_example/        # minimal runtime plugin demo
│   ├── ottd_uds_ipc/               # shared UDS framing protocol (header-only)
│   └── python_controller/          # Python gRPC client/controller examples
└── src/3rdparty/extras/
    └── abi_rpc/                    # ABI RPC layer integrated into OpenTTD
        └── proto/                  # canonical protobuf service definitions
```

Example scenarios live under `contrib/content/scenario/` (e.g. `example.scn`, `example-no-script.scn`).

---

## Included Plugins

### `libottd_rpc_example`

A minimal runtime-loaded plugin used by the default Docker configuration.

Purpose:

* demonstrates dynamic loading,
* demonstrates ABI boundaries,
* demonstrates protobuf communication,
* avoids Apache-2.0 runtime dependencies.

This plugin is loaded automatically in the **default Docker image** via `OTTD_USE_RPC_PLUGIN`. A plain `cmake` build does nothing until that variable is set.

---

### `libottd_uds_bridge`

GPLv2-compatible runtime plugin loaded into OpenTTD. It exposes the ABI RPC surface on a **Unix domain socket** so out-of-process tools can call into the game without linking Apache-2.0 libraries into the OpenTTD binary.

* configured with `OTTD_UDS_BRIDGE_ENABLED` and `OTTD_UDS_SOCKET_PATH`,
* uses Boost.Asio for local IPC ([`contrib/ottd_uds_ipc`](/contrib/ottd_uds_ipc)),
* see [contrib/libottd_uds_bridge/README.md](contrib/libottd_uds_bridge/README.md) (build via `manage.sh build-plugin-libottd_uds_bridge-debug`).

---

### `grpc_server` (`ottd_grpc_server`)

Standalone executable (not a plugin) that:

* runs the async gRPC server,
* forwards each RPC to OpenTTD through the UDS bridge,
* waits internally for deferred script RPC results where needed (for example `ScriptGoal.New`),
* is built with the Apache-2.0 gRPC stack in its **own** pixi environment ([`contrib/grpc_server/pixi.toml`](/contrib/grpc_server/pixi.toml)),
* has a dedicated Docker image ([`contrib/grpc_server/Dockerfile`](/contrib/grpc_server/Dockerfile)).

See [contrib/grpc_server/README.md](contrib/grpc_server/README.md).

---

### `python_controller`

Example **external** controller that connects to `ottd_grpc_server` over gRPC (not Squirrel or in-process ABI).

Purpose:

* demonstrates end-to-end remote control from Python,
* provides generated protobuf client stubs and runnable examples under `controller/`,
* runs as the `python-controller` service in [`docker-compose.yml`](/docker-compose.yml).

See [contrib/python_controller/README.md](contrib/python_controller/README.md) for local usage.

---

## Runtime Configuration

| Variable | Used by | Description |
| -------- | ------- | ----------- |
| `OTTD_USE_RPC_PLUGIN` | OpenTTD | Path to the bridge (or example) `.so` / `.dylib`. Unset = no plugin. |
| `OTTD_UDS_BRIDGE_ENABLED` | `libottd_uds_bridge` | Set to `1` or `true` to bind the UDS socket. |
| `OTTD_UDS_SOCKET_PATH` | bridge + `ottd_grpc_server` | Shared socket path (default `/run/openttd/rpc.sock`). |
| `OTTD_GRPC_ENABLED` | `ottd_grpc_server` | Set to `1` or `true` to start the gRPC server process. |
| `OTTD_GRPC_HOSTNAME` | `ottd_grpc_server`, `python_controller` | gRPC bind / connect host. |
| `OTTD_GRPC_PORT` | `ottd_grpc_server`, `python_controller` | gRPC port (default `50051`). |

[`docker-compose.yml`](/docker-compose.yml) sets these for the three services. For local runs: start OpenTTD with the UDS bridge plugin, then start `ottd_grpc_server` pointing at the same socket.

**Ports exposed by Compose:**

| Port | Protocol | Service |
| ---- | -------- | ------- |
| 3979 | TCP/UDP  | OpenTTD game server |
| 3977 | TCP      | OpenTTD admin interface |
| 50051 | TCP | `grpc-server` (gRPC) |

---

## CLI Flags

The dedicated server accepts two additional flags introduced by this fork:

| Flag | Purpose |
| ---- | ------- |
| `--standby` | Start the dedicated server idle, waiting for an RPC plugin to drive game startup (used by the Docker images). Requires a loaded RPC plugin. |
| `--activate-dummy-game-script` | If no game script is configured, activate the bundled dummy game script (`DUMM`) so script-related RPC calls have a minimal GS context. |

Both flags are passed by the default Docker `CMD`; omit them when running a stock dedicated server without the ABI layer.

Docker images bundle the dummy game script as `dummydum-gs-08.tar` under the OpenTTD content download path.

For reproducible **release** builds without a `.git` directory (for example Docker image builds), pass `OTTD_REV_OVERRIDE` to CMake. It is handled by [FindVersion.cmake](/cmake/scripts/FindVersion.cmake) and skips git-based version detection. Fields are semicolon-separated, in the same order as [`.ottdrev`](/.ottdrev) (tab-separated locally):

`version;isodate;modified;hash;istag;isstabletag`

Example (matches the pinned Docker Compose build):

```bash
cmake .. -DOTTD_REV_OVERRIDE=15.3;20260404;0;14ec60f248547d4d062a1160f0fc26d742319888;1;1
```

[`docker-compose.yml`](/docker-compose.yml) passes `OTTD_REV_OVERRIDE` as a build arg to the OpenTTD image (override via env when building). A checked-in `.ottdrev` is still supported when neither git nor `OTTD_REV_OVERRIDE` is available.

---

# Docker Compose Stack

The default [`docker-compose.yml`](/docker-compose.yml) builds and runs:

| Service | Image / build | Role |
| ------- | ------------- | ---- |
| `openttd` | [Dockerfile](/Dockerfile) or [Dockerfile.cached](/Dockerfile.cached) | Dedicated server + `libottd_uds_bridge` (GPLv2-compatible pixi deps) |
| `grpc-server` | [contrib/grpc_server/Dockerfile](/contrib/grpc_server/Dockerfile) | `ottd_grpc_server` (gRPC + Apache-2.0 stack, separate container) |
| `python-controller` | [contrib/python_controller/Dockerfile](/contrib/python_controller/Dockerfile) | Example gRPC client |

OpenTTD and `grpc-server` share a Docker volume for the UDS socket at `/run/openttd`.

Start the stack:

```bash
docker compose up --build
```

### Optimised cached build (development)

For faster **incremental** rebuilds while iterating on source, use [`Dockerfile.cached`](/Dockerfile.cached) instead of the default [`Dockerfile`](/Dockerfile). It requires [Docker BuildKit](https://docs.docker.com/build/buildkit/) (enabled by default in current Docker Desktop and recent `docker compose`).

```bash
OPENTTD_DOCKERFILE=Dockerfile.cached docker compose up --build
```

What it does differently:

* **BuildKit cache mounts** — CMake build directories for OpenTTD, host tools, and contrib plugins are restored and updated across builds via [`contrib/utils/cache_build_util.sh`](/contrib/utils/cache_build_util.sh), so changed translation units recompile instead of rebuilding from scratch.
* **ccache** — compiler object cache is reused within each build stage.
* **Slimmer runtime image** — both Dockerfiles use a multi-stage build: the final stage keeps only `/opt/openttd`, basesets, the UDS bridge plugin under `/opt/openttd/lib/`, and the pixi env (`pixi.toml` / `pixi.lock` in `/build` for `pixi run`).
* **Smaller build context** — [`Dockerfile.cached.dockerignore`](/Dockerfile.cached.dockerignore) sends only sources needed to compile (BuildKit picks it up automatically when `-f Dockerfile.cached` is used).

The first cached build is similar in time to the default Dockerfile; savings appear on subsequent `docker compose up --build` runs after local edits. [`docker-compose.yml`](/docker-compose.yml) selects the Dockerfile via `OPENTTD_DOCKERFILE` (default `Dockerfile`); plugin path defaults to `/opt/openttd/lib/libottd_uds_bridge.so` for both variants.

The OpenTTD image does **not** link gRPC. Apache-2.0 dependencies exist only in the `grpc-server` container.

## Licensing notice (gRPC path)

You are responsible for evaluating license compatibility for your deployment (GPLv2 OpenTTD process + separately distributed gRPC binary/container). This repository does not assert GPLv2/Apache-2.0 compatibility for any combined distribution model.

---

# ABI Design

The runtime plugin ABI is intentionally designed to remain independent from OpenTTD internals — external controllers can drive admin, console, and script-level actions without compiling against OpenTTD.

Design goals:

* stable C ABI boundary (`plugin_interface.h`, currently **API version 2**),
* host services supplied via `RegisterHostOps` (`get_last_error`, scoped memory vtable, `handle_rpc`) — plugins must not link OpenTTD symbols,
* protobuf-defined messages (under `src/3rdparty/extras/abi_rpc/proto/` and mirrored in each plugin),
* opaque handles,
* no OpenTTD internal structure exposure,
* independently buildable plugins,
* runtime discovery/loading via `OTTD_USE_RPC_PLUGIN`.

Example simplified flow:

```text
External controller (e.g. python_controller)
    ↕ gRPC
ottd_grpc_server (contrib/grpc_server)
    ↕ UDS (OTRP framing, contrib/ottd_uds_ipc)
libottd_uds_bridge plugin in OpenTTD process
    ↕ ABI + protobuf
OpenTTD core handlers (abi_rpc)
```

---

# Process vs. plugin separation

| Piece | Runs where | Why |
| ----- | ---------- | --- |
| OpenTTD + UDS bridge | Game container / process | GPLv2 runtime; Boost-only IPC in-process |
| `ottd_grpc_server` | Separate binary / container | Apache-2.0 gRPC stack never loaded into OpenTTD |
| `libottd_rpc_example` | Optional plugin | Minimal ABI demo without UDS or gRPC |

---

# Distribution notes

Redistribution obligations depend on which artifacts you ship (OpenTTD binary, bridge plugin, gRPC server binary, images). Perform your own compliance review before publishing combined images or installers.

---

# Status

Experimental research project.

No warranty is provided regarding:

* production suitability,
* ABI stability,
* legal interpretation of combined runtime deployments.

# OpenTTD (original Readme)

## Table of contents

- 1.0) [About](#10-about)
    - 1.1) [Downloading OpenTTD](#11-downloading-openttd)
    - 1.2) [OpenTTD gameplay manual](#12-openttd-gameplay-manual)
    - 1.3) [Supported platforms](#13-supported-platforms)
    - 1.4) [Installing and running OpenTTD](#14-installing-and-running-openttd)
    - 1.5) [Add-on content / mods](#15-add-on-content--mods)
    - 1.6) [OpenTTD directories](#16-openttd-directories)
    - 1.7) [Compiling OpenTTD](#17-compiling-openttd)
- 2.0) [Contact and community](#20-contact-and-community)
    - 2.1) [Multiplayer games](#21-multiplayer-games)
    - 2.2) [Contributing to OpenTTD](#22-contributing-to-openttd)
    - 2.3) [Reporting bugs](#23-reporting-bugs)
    - 2.4) [Translating](#24-translating)
- 3.0) [Licensing](#30-licensing)
- 4.0) [Credits](#40-credits)

## 1.0) About

OpenTTD is a transport simulation game based upon the popular game Transport Tycoon Deluxe, written by Chris Sawyer.
It attempts to mimic the original game as closely as possible while extending it with new features.

OpenTTD is licensed under the GNU General Public License version 2.0, but includes some 3rd party software under different licenses.
See the section ["Licensing"](#30-licensing) below for details.

## 1.1) Downloading OpenTTD

OpenTTD can be downloaded from the [official OpenTTD website](https://www.openttd.org/).

Both 'stable' and 'nightly' versions are available for download:

- most people should choose the 'stable' version, as this has been more extensively tested
- the 'nightly' version includes the latest changes and features, but may sometimes be less reliable

OpenTTD is also available for free on [Steam](https://store.steampowered.com/app/1536610/OpenTTD/), [GOG.com](https://www.gog.com/game/openttd), and the [Microsoft Store](https://www.microsoft.com/p/openttd-official/9ncjg5rvrr1c). On some platforms OpenTTD will be available via your OS package manager or a similar service.

## 1.2) OpenTTD gameplay manual

OpenTTD has a [community-maintained wiki](https://wiki.openttd.org/), including a gameplay manual and tips.

## 1.3) Supported platforms

OpenTTD has been ported to several platforms and operating systems.

The currently supported platforms are:

- Linux (SDL (OpenGL and non-OpenGL))
- macOS (universal) (Cocoa)
- Windows (Win32 GDI / OpenGL)

Other platforms may also work (in particular various BSD systems), but we don't actively test or maintain these.

### 1.3.1) Legacy support

Platforms, languages and compilers change.
We'll keep support going on old platforms as long as someone is interested in supporting them, except where it means the project can't move forward to keep up with language and compiler features.

We guarantee that every revision of OpenTTD will be able to load savegames from every older revision (excepting where the savegame is corrupt).
Please report a bug if you find a save that doesn't load.

## 1.4) Installing and running OpenTTD

OpenTTD is usually straightforward to install, but for more help the wiki [includes an installation guide](https://wiki.openttd.org/en/Manual/Installation).

OpenTTD needs some additional graphics and sound files to run.

For some platforms these will be downloaded during the installation process if required.

For some platforms, you will need to refer to [the installation guide](https://wiki.openttd.org/en/Manual/Installation).

### 1.4.1) Free graphics and sound files

The free data files, split into OpenGFX for graphics, OpenSFX for sounds and
OpenMSX for music can be found at:

- [OpenGFX](https://www.openttd.org/downloads/opengfx-releases/latest)
- [OpenSFX](https://www.openttd.org/downloads/opensfx-releases/latest)
- [OpenMSX](https://www.openttd.org/downloads/openmsx-releases/latest)

Please follow the readme of these packages about the installation procedure.
The Windows installer can optionally download and install these packages.

### 1.4.2) Original Transport Tycoon Deluxe graphics and sound files

If you want to play with the original Transport Tycoon Deluxe data files you have to copy the data files from the CD-ROM into the baseset/ directory.
It does not matter whether you copy them from the DOS or Windows version of Transport Tycoon Deluxe.
The Windows install can optionally copy these files.

You need to copy the following files:
- sample.cat
- trg1r.grf or TRG1.GRF
- trgcr.grf or TRGC.GRF
- trghr.grf or TRGH.GRF
- trgir.grf or TRGI.GRF
- trgtr.grf or TRGT.GRF

### 1.4.3) Original Transport Tycoon Deluxe music

If you want the Transport Tycoon Deluxe music, copy the appropriate files from the original game into the baseset folder.
- TTD for Windows: All files in the gm/ folder (gm_tt00.gm up to gm_tt21.gm)
- TTD for DOS: The GM.CAT file
- Transport Tycoon Original: The GM.CAT file, but rename it to GM-TTO.CAT

## 1.5) Add-on content / mods

OpenTTD features multiple types of add-on content, which modify gameplay in different ways.

Most types of add-on content can be downloaded within OpenTTD via the 'Check Online Content' button in the main menu.

Add-on content can also be installed manually, but that's more complicated; the [OpenTTD wiki](https://wiki.openttd.org/) may offer help with that, or the [OpenTTD directory structure guide](./docs/directory_structure.md).

### 1.5.1) Social Integration

OpenTTD has the ability to load plugins to integrate with Social Platforms like Steam, Discord, etc.

To enable such integration, the plugin for the specific platform has to be downloaded and stored in the `social_integration` folder.

See [OpenTTD's website](https://www.openttd.org), under Downloads, for what plugins are available.

### 1.6) OpenTTD directories

OpenTTD uses its own directory structure to store game data, add-on content etc.

For more information, see the [directory structure guide](./docs/directory_structure.md).

### 1.7) Compiling OpenTTD

If you want to compile OpenTTD from source, instructions can be found in [COMPILING.md](./COMPILING.md).

## 2.0) Contact and Community

'Official' channels

- [OpenTTD website](https://www.openttd.org)
- [OpenTTD official Discord](https://discord.gg/openttd)
- IRC chat using #openttd on irc.oftc.net [more info about our irc channel](https://wiki.openttd.org/en/Development/IRC%20channel)
- [OpenTTD on Github](https://github.com/OpenTTD/) for code repositories and for reporting issues
- [forum.openttd.org](https://forum.openttd.org/) - the primary community forum site for discussing OpenTTD and related games
- [OpenTTD wiki](https://wiki.openttd.org/) community-maintained wiki, including topics like gameplay guide, detailed explanation of some game mechanics, how to use add-on content (mods) and much more

'Unofficial' channels

- the OpenTTD wiki has a [page listing OpenTTD communities](https://wiki.openttd.org/en/Community/Community) including some in languages other than English


### 2.1) Multiplayer games

You can play OpenTTD with others, either cooperatively or competitively.

See the [multiplayer documentation](./docs/multiplayer.md) for more details.

### 2.2) Contributing to OpenTTD

We welcome contributors to OpenTTD.  More information for contributors can be found in [CONTRIBUTING.md](./CONTRIBUTING.md)

### 2.3) Reporting bugs

Good bug reports are very helpful.  We have a [guide to reporting bugs](./CONTRIBUTING.md#bug-reports) to help with this.

Desyncs in multiplayer are complex to debug and report (some software development skils are required).
Instructions can be found in [debugging and reporting desyncs](./docs/debugging_desyncs.md).

### 2.4) Translating

OpenTTD is translated into many languages.  Translations are added and updated via the [online translation tool](https://translator.openttd.org).

## 3.0) Licensing

OpenTTD is licensed under the GNU General Public License version 2.0.
For the complete license text, see the file '[COPYING.md](./COPYING.md)'.
This license applies to all files in this distribution, except as noted below.

The squirrel implementation in `src/3rdparty/squirrel` is licensed under the Zlib license.
See `src/3rdparty/squirrel/COPYRIGHT` for the complete license text.

The md5 implementation in `src/3rdparty/md5` is licensed under the Zlib license.
See the comments in the source files in `src/3rdparty/md5` for the complete license text.

The fmt implementation in `src/3rdparty/fmt` is licensed under the MIT license.
See `src/3rdparty/fmt/LICENSE.rst` for the complete license text.

The nlohmann json implementation in `src/3rdparty/nlohmann` is licensed under the MIT license.
See `src/3rdparty/nlohmann/LICENSE.MIT` for the complete license text.

The OpenGL API in `src/3rdparty/opengl` is licensed under the MIT license.
See `src/3rdparty/opengl/khrplatform.h` for the complete license text.

The catch2 implementation in `src/3rdparty/catch2` is licensed under the Boost Software License, Version 1.0.
See `src/3rdparty/catch2/LICENSE.txt` for the complete license text.

The icu scriptrun implementation in `src/3rdparty/icu` is licensed under the Unicode license.
See `src/3rdparty/icu/LICENSE` for the complete license text.

The monocypher implementation in `src/3rdparty/monocypher` is licensed under the 2-clause BSD and CC-0 license.
See `src/3rdparty/monocypher/LICENSE.md` for the complete license text.

The OpenTTD Social Integration API in `src/3rdparty/openttd_social_integration_api` is licensed under the MIT license.
See `src/3rdparty/openttd_social_integration_api/LICENSE` for the complete license text.

The atomic datatype support detection in `cmake/3rdparty/llvm/CheckAtomic.cmake` is licensed under the Apache 2.0 license.
See `cmake/3rdparty/llvm/LICENSE.txt` for the complete license text.

ABI protobuf protocol in `src/3rdparty/extras/abi_rpc/proto` is licenced under the MIT license. See `src/3rdparty/extras/abi_rpc/proto/LICENSE` for the complete license text.

ABI plugin interface header in `src/3rdparty/extras/abi_rpc/plugin_interface.h` is licenced under the MIT license. Complete license text is located in the header of the file.

ABI plugin scoped memory manager interface in `src/3rdparty/extras/abi_rpc/scoped_memory_manager.h` is licenced under the MIT license. Complete license text is located in the header of the file.

ABI plugin example in `contrib/libottd_rpc_example` is licenced under the MIT license. See `contrib/libottd_rpc_example/LICENSE` for the complete license text.

UDS IPC headers in `contrib/ottd_uds_ipc`, the bridge plugin in `contrib/libottd_uds_bridge`, and the gRPC server in `contrib/grpc_server` are licenced under the MIT license. See each directory's `LICENSE` file.

gRPC python controller service example in `contrib/python_controller` is licenced under the MIT license. See `contrib/python_controller/LICENSE` for the complete license text.

## 4.0) Credits

See [CREDITS.md](./CREDITS.md)
