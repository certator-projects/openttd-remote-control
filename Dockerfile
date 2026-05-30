# OpenTTD dedicated server (GPLv2-compatible default stack).
# Builds libottd_rpc_example, libottd_uds_bridge and installs OpenTTD to /opt/openttd.
# Multi-stage: `builder` compiles; `runtime` keeps /opt/openttd, plugin, basesets, pixi env.
# Use with docker-compose.yml (UDS bridge in-process; gRPC in grpc-server container).

FROM ghcr.io/prefix-dev/pixi:latest AS base_set

RUN mkdir -p /baseset && \
    cd /baseset && \
    apt-get -qq update && \
    apt-get -yqq install --no-install-recommends curl unzip && \
    curl -L https://cdn.openttd.org/opengfx-releases/0.6.0/opengfx-0.6.0-all.zip -o opengfx-all.zip && \
    unzip opengfx-all.zip && \
    rm -f opengfx-all.zip && \
    apt-get clean && rm -rf /var/lib/apt/lists/*

FROM ghcr.io/prefix-dev/pixi:latest AS pixi_base

WORKDIR /build

COPY ./pixi.toml ./pixi.toml
COPY ./pixi.lock ./pixi.lock

RUN pixi workspace platform add linux-aarch64 && \
    pixi install

FROM pixi_base AS builder

ARG OTTD_REV_OVERRIDE

# Shared by contrib plugins (built before COPY . . for layer caching)
COPY cmake/OttdProtobufLite.cmake ./cmake/OttdProtobufLite.cmake

COPY contrib/libottd_rpc_example/ /build/contrib/libottd_rpc_example/
COPY contrib/ottd_uds_ipc/ /build/contrib/ottd_uds_ipc/
COPY contrib/libottd_uds_bridge/ /build/contrib/libottd_uds_bridge/

RUN --mount=type=cache,target=/root/.cache/ccache \
    mkdir -p contrib/libottd_rpc_example/build-debug && \
    cd contrib/libottd_rpc_example/build-debug && \
    pixi run bash -c ' \
        cmake .. \
         -DCMAKE_C_COMPILER_LAUNCHER="$(command -v ccache)" \
         -DCMAKE_CXX_COMPILER_LAUNCHER="$(command -v ccache)" \
         -DCMAKE_BUILD_TYPE=Release \
         -DCMAKE_CXX_FLAGS="-w" \
         -DCMAKE_C_FLAGS="-w" && \
        cmake --build . -- --jobs=$(nproc) \
    '

RUN --mount=type=cache,target=/root/.cache/ccache \
    mkdir -p contrib/libottd_uds_bridge/build-debug && \
    cd contrib/libottd_uds_bridge/build-debug && \
    pixi run bash -c ' \
        cmake .. \
         -DCMAKE_C_COMPILER_LAUNCHER="$(command -v ccache)" \
         -DCMAKE_CXX_COMPILER_LAUNCHER="$(command -v ccache)" \
         -DCMAKE_BUILD_TYPE=Release \
         -DCMAKE_CXX_FLAGS="-w" \
         -DCMAKE_C_FLAGS="-w" && \
        cmake --build . -- --jobs=$(nproc) \
    '

COPY . .

RUN --mount=type=cache,target=/root/.cache/ccache \
    mkdir -p build-host && \
    cd build-host && \
    pixi run bash -c " \
        cmake .. \
         -DCMAKE_C_COMPILER_LAUNCHER=\$(command -v ccache) \
         -DCMAKE_CXX_COMPILER_LAUNCHER=\$(command -v ccache) \
         -DCMAKE_BUILD_TYPE=Release \
         -DOTTD_REV_OVERRIDE=\"${OTTD_REV_OVERRIDE}\" \
         -DOPTION_TOOLS_ONLY=ON && \
        cmake --build . --target openttd_tools -- --jobs=\$(nproc) \
    "

RUN --mount=type=cache,target=/root/.cache/ccache \
    mkdir -p build && \
    cd build && \
    pixi run bash -c " \
        cmake .. \
         -DCMAKE_C_COMPILER_LAUNCHER=\$(command -v ccache) \
         -DCMAKE_CXX_COMPILER_LAUNCHER=\$(command -v ccache) \
         -DCMAKE_BUILD_TYPE=Release \
         -DOTTD_REV_OVERRIDE=\"${OTTD_REV_OVERRIDE}\" \
         -DOPTION_DEDICATED=ON \
         -DHOST_BINARY_DIR=/build/build-host \
         -DCMAKE_INSTALL_PREFIX=/opt/openttd && \
        cmake --build . --target openttd -- --jobs=\$(nproc) && \
        make install \
    "

# Stage runtime paths used by the final image (no /build source tree in runtime).
RUN mkdir -p /opt/openttd/lib && \
    cp -a contrib/libottd_uds_bridge/build-debug/libottd_uds_bridge.so* /opt/openttd/lib/

RUN mkdir -p /root/.openttd/baseset && \
    cp -r /build/build/baseset /root/.openttd/baseset && \
    mkdir -p /root/.local/share/openttd/baseset

ARG DUMMY_GS_TAR=dummydum-gs-08
RUN cd /tmp && cp -r /build/src/3rdparty/extras/dummy_game_script . && \
    tar -cf "${DUMMY_GS_TAR}.tar" dummy_game_script && \
    mkdir -p /root/.local/share/openttd/content_download/game && \
    cp "/tmp/${DUMMY_GS_TAR}.tar" /root/.local/share/openttd/content_download/game/ && \
    rm -rf /tmp/dummy_game_script "/tmp/${DUMMY_GS_TAR}.tar"

RUN mkdir -p /opt/openttd/data

# === runtime image (pixi.toml/lock + env only; no source or build tree) ===

FROM pixi_base AS runtime

COPY --from=builder /opt/openttd /opt/openttd
COPY --from=builder /root/.openttd/baseset /root/.openttd/baseset
COPY --from=builder /root/.local/share/openttd/content_download/game \
    /root/.local/share/openttd/content_download/game
COPY --from=base_set /baseset /root/.local/share/openttd/baseset

# pixi.toml and .pixi env live in /build (from pixi_base); pixi run must start there.
WORKDIR /build

EXPOSE 3979/tcp 3979/udp 3977/tcp

CMD ["/usr/local/bin/pixi", "run", "bash", "-c", \
    "cd /opt/openttd && /opt/openttd/games/openttd -D --standby --activate-dummy-game-script"]
