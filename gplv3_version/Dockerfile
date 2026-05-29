# OpenTTD dedicated server for the optional GPLv3 / gRPC stack.
# Same as the root Dockerfile, plus libottd_grpc_server and gplv3_version pixi deps.
# Use with docker-compose.gplv3.yml (libottd_grpc_server + python_controller).

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

COPY ./gplv3_version/pixi.toml ./pixi.toml
COPY ./gplv3_version/pixi.lock ./pixi.lock

RUN pixi workspace platform add linux-aarch64 && \
    pixi install

FROM pixi_base AS runtime

COPY cmake/OttdProtobufLite.cmake ./cmake/OttdProtobufLite.cmake

COPY contrib/libottd_rpc_example/ /build/contrib/libottd_rpc_example/

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

COPY contrib/libottd_grpc_server/ /build/contrib/libottd_grpc_server/

RUN --mount=type=cache,target=/root/.cache/ccache \
    mkdir -p contrib/libottd_grpc_server/build-debug && \
    cd contrib/libottd_grpc_server/build-debug && \
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

# Root pixi.toml is copied by COPY . .; restore the gplv3 workspace for OpenTTD + gRPC deps.
COPY ./gplv3_version/pixi.toml ./pixi.toml
COPY ./gplv3_version/pixi.lock ./pixi.lock

RUN --mount=type=cache,target=/root/.cache/ccache \
    mkdir -p build-host && \
    cd build-host && \
    pixi run bash -c ' \
        cmake .. \
         -DCMAKE_C_COMPILER_LAUNCHER="$(command -v ccache)" \
         -DCMAKE_CXX_COMPILER_LAUNCHER="$(command -v ccache)" \
         -DCMAKE_BUILD_TYPE=Release \
         -DOTTD_REV_OVERRIDE="15.3;20260404;0;14ec60f248547d4d062a1160f0fc26d742319888;1;1" \
         -DOPTION_TOOLS_ONLY=ON && \
        cmake --build . --target openttd_tools -- --jobs=$(nproc) \
    '

RUN mkdir -p build && \
    cd build && \
    pixi run bash -c ' \
        cmake .. \
         -DCMAKE_C_COMPILER_LAUNCHER="$(command -v ccache)" \
         -DCMAKE_CXX_COMPILER_LAUNCHER="$(command -v ccache)" \
         -DCMAKE_BUILD_TYPE=Release \
         -DOTTD_REV_OVERRIDE="15.3;20260404;0;14ec60f248547d4d062a1160f0fc26d742319888;1;1" \
         -DOPTION_DEDICATED=ON \
         -DHOST_BINARY_DIR=/build/build-host \
         -DCMAKE_INSTALL_PREFIX=/opt/openttd && \
        cmake --build . --target openttd -- --jobs=$(nproc) && \
        make install \
    '

RUN mkdir -p /root/.openttd/baseset && \
    cp -r /build/build/baseset /root/.openttd/baseset && \
    mkdir -p /root/.local/share/openttd/baseset

COPY --from=base_set /baseset /root/.local/share/openttd/baseset

ARG DUMMY_GS_TAR=dummydum-gs-08
COPY ./src/3rdparty/extras/dummy_game_script /tmp/dummy_game_script
RUN cd /tmp && tar -cf "${DUMMY_GS_TAR}.tar" dummy_game_script && \
    mkdir -p /root/.local/share/openttd/content_download/game && \
    cp "/tmp/${DUMMY_GS_TAR}.tar" /root/.local/share/openttd/content_download/game/

RUN mkdir -p /opt/openttd/data

WORKDIR /build

EXPOSE 3979/tcp 3979/udp 3977/tcp 50051/tcp 50051/udp

CMD ["/usr/local/bin/pixi", "run", "bash", "-c", \
    "cd /opt/openttd && /opt/openttd/games/openttd -D --standby --activate-dummy-game-script"]
