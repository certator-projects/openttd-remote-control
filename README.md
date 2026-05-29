# OpenTTD gRPC Runtime Integration Demo

Experimental runtime integration layer for OpenTTD using a dynamically loaded plugin and protobuf-based ABI communication.

## Fork notes

This fork is based on upstream OpenTTD **15.3** (commit `14ec60f`), not `master`, so the dedicated server stays compatible with the official client.

Changes since that baseline:

* **ABI RPC layer** (`src/3rdparty/extras/abi_rpc/`) — proof-of-concept runtime plugin interface with protobuf-defined messages, enabling **external control via ABI** instead of writing game logic in Squirrel.
* **Runtime plugins** — `libottd_rpc_example` (default path) and optional `libottd_grpc_server`.
* **Python controller** — example external client for the gRPC plugin path (`contrib/python_controller/`).
* **Docker compose stacks** — default (GPLv2-compatible) and optional gplv3 (gRPC) configurations.

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
| gRPC                    | Apache-2.0   | used by libottd_grpc_server, which is under MIT license itself |
| protobuf                | BSD-3-Clause | |
| runtime plugin examples | MIT          | |

The default configuration is designed to remain GPLv2-compatible and does **not** link OpenTTD against Apache-2.0 gRPC components.

---

# Repository Layout

```text
.
├── docker-compose.yml              # default Docker build (libottd_rpc_example)
├── docker-compose.gplv3.yml        # GPLv3 Docker build (libottd_grpc_server + python_controller)
├── contrib/
│   ├── content/scenario/           # example scenario files (.scn)
│   ├── libottd_grpc_server/        # optional gRPC runtime plugin
│   ├── libottd_rpc_example/        # default minimal runtime plugin
│   └── python_controller/          # Python gRPC client/controller examples
├── src/3rdparty/extras/
│   └── abi_rpc/                    # ABI RPC layer integrated into OpenTTD
│       └── proto/                  # canonical protobuf service definitions
└── gplv3_version/                  # alternate Dockerfile for GPLv3-compatible build
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

### `libottd_grpc_server`

Optional experimental plugin using:

* gRPC,
* protobuf,
* Apache-2.0 licensed runtime libraries (gRPC and abseil).

This plugin is intentionally isolated from OpenTTD internals and communicates only through the repository-defined ABI/protobuf interfaces — so it can be built and distributed separately from OpenTTD sources.

The gRPC plugin:

* does not use OpenTTD source code,
* does not include OpenTTD internal headers,
* does not depend on OpenTTD internal data structures.

However, because the plugin is dynamically loaded into the OpenTTD process, users must independently evaluate licensing compatibility for their intended distribution model.

This plugin is **NOT** enabled in the default configuration.

---

### `python_controller`

Example **external** controller that talks to OpenTTD through the gRPC plugin (not through Squirrel or in-process ABI directly).

Purpose:

* demonstrates end-to-end remote control from Python,
* provides generated protobuf client stubs and runnable examples under `controller/`,
* runs as a separate container/service in `docker-compose.gplv3.yml`.

See [contrib/python_controller/README.md](contrib/python_controller/README.md) for local usage.

---

## Runtime Configuration

| Variable | Used by | Description |
| -------- | ------- | ----------- |
| `OTTD_USE_RPC_PLUGIN` | OpenTTD (`abi_rpc` plugin loader) | Path to the `.so` / `.dylib` plugin to load at startup. Unset = no plugin loaded. |
| `OTTD_GRPC_ENABLED` | `libottd_grpc_server` | Set to `1` or `true` to start the in-process gRPC server inside the plugin. |
| `OTTD_GRPC_HOSTNAME` | `libottd_grpc_server`, `python_controller` | Hostname/IP to bind (plugin) or connect to (controller). |
| `OTTD_GRPC_PORT` | `libottd_grpc_server`, `python_controller` | gRPC port (default `50051`). |

Default Docker compose files set these for you. For manual runs, export `OTTD_USE_RPC_PLUGIN` before starting the dedicated server; gRPC variables apply only when using `libottd_grpc_server`.

**Ports exposed by the compose stacks:**

| Port | Protocol | Service |
| ---- | -------- | ------- |
| 3979 | TCP/UDP  | OpenTTD game server |
| 3977 | TCP      | OpenTTD admin interface |
| 50051 | TCP/UDP | gRPC (gplv3 configuration only) |

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

Example (matches the pinned Docker build):

```bash
cmake .. -DOTTD_REV_OVERRIDE=15.3;20260404;0;14ec60f248547d4d062a1160f0fc26d742319888;1;1
```

[Dockerfile](/Dockerfile) and [gplv3_version/Dockerfile](/gplv3_version/Dockerfile) pass this flag on configure. A checked-in `.ottdrev` is still supported when neither git nor `OTTD_REV_OVERRIDE` is available.

---

# Default Setup (GPLv2-Compatible)

The default setup builds:

* OpenTTD dedicated server,
* `libottd_rpc_example` runtime plugin,
* protobuf ABI layer.

No Apache-2.0 gRPC runtime components are linked into the OpenTTD process in this configuration.

Start the default setup:

```bash
docker-compose up
```

This configuration is the recommended default for:

* development,
* ABI experimentation,
* protocol testing,
* runtime loading demonstrations.

---

# Optional gRPC Runtime Configuration

An additional `docker-compose.gplv3.yml` compose file is provided for advanced users working with environments where all linked/runtime-loaded components are compatible with GPLv3 licensing terms.

The optional configuration enables:

* runtime loading of the gRPC plugin,
* in-process gRPC communication,
* protobuf RPC integration,
* a companion `python-controller` service that connects over gRPC.

Start the optional configuration:

```bash
COMPOSE_FILE=docker-compose.gplv3.yml docker-compose up
```

The gplv3 stack mounts `contrib/content/scenario/example-no-script.scn` by default (no embedded game script — control is external via gRPC).

## Important Licensing Notice

The optional gRPC configuration introduces Apache-2.0 licensed runtime dependencies into the dynamically loaded plugin environment.

This configuration:

* is provided for experimental and research purposes,
* is intended only for environments where all involved components are GPLv3-compatible,
* is not intended for redistribution together with GPLv2-only OpenTTD builds.

Users are responsible for independently verifying:

* license compatibility,
* redistribution rights,
* compliance obligations for their specific build and deployment model.

No claim of GPLv2/Apache-2.0 compatibility is made by this repository.

---

# ABI Design

The runtime plugin ABI is intentionally designed to remain independent from OpenTTD internals — external controllers can drive admin, console, and script-level actions without compiling against OpenTTD.

Design goals:

* stable C ABI boundary,
* protobuf-defined messages (under `src/3rdparty/extras/abi_rpc/proto/` and mirrored in each plugin),
* opaque handles,
* no OpenTTD internal structure exposure,
* independently buildable plugins,
* runtime discovery/loading via `OTTD_USE_RPC_PLUGIN`.

Example simplified flow:

```text
External controller (e.g. python_controller)
    ↕
gRPC (optional, via libottd_grpc_server)
    ↕
OpenTTD process
    ↕
runtime ABI
    ↕
protobuf transport
    ↕
plugin implementation
```

---

# Why Two Configurations Exist

This repository intentionally separates:

| Configuration | Purpose | Why |
| ------------- | ------- | --- |
| default | GPLv2-compatible runtime loading demo | Keeps the common path free of Apache-2.0 gRPC/Abseil while proving the ABI plugin model. |
| gplv3 | optional experimental gRPC integration | Lets external clients (Python, etc.) control the server remotely; only for deployments where GPLv3-compatible combination of all runtime components is acceptable. |

This mirrors common open-source practices where optional components may introduce additional licensing requirements depending on enabled build/runtime combinations.

---

# Distribution Notes

The default configuration may be redistributed under the terms of the included licenses.

The optional gRPC runtime configuration may introduce additional licensing obligations depending on:

* linked libraries,
* runtime composition,
* distribution model,
* applicable GPL interpretation.

Users distributing modified images or binaries should perform their own legal/compliance review.

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

gRPC ABI plugin example in `contrib/libottd_grpc_server` is licenced under the MIT license. See `contrib/libottd_grpc_server/LICENSE` for the complete license text.

gRPC python controller service example in `contrib/python_controller` is licenced under the MIT license. See `contrib/python_controller/LICENSE` for the complete license text.

## 4.0) Credits

See [CREDITS.md](./CREDITS.md)
