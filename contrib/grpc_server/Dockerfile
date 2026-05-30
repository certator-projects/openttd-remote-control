# Standalone gRPC server for OpenTTD (Apache-2.0 stack).
# Connects to the in-process UDS bridge plugin over a shared Unix domain socket.
# Multi-stage: `builder` compiles; `runtime` keeps /opt/ottd_grpc_server and pixi env.
# Use with docker-compose.yml (grpc-server service).

FROM ghcr.io/prefix-dev/pixi:latest AS pixi_base

WORKDIR /build

COPY pixi.toml ./pixi.toml
COPY pixi.lock ./pixi.lock

RUN pixi workspace platform add linux-aarch64 && \
    pixi install

FROM pixi_base AS builder

COPY --from=root-context cmake/OttdProtobufLite.cmake ./cmake/OttdProtobufLite.cmake
COPY --from=root-context contrib/ottd_uds_ipc/ /build/contrib/ottd_uds_ipc/
COPY . /build/contrib/grpc_server/

# Keep pixi workspace at /build only (from pixi_base), not nested under grpc_server.
RUN rm -f /build/contrib/grpc_server/pixi.toml /build/contrib/grpc_server/pixi.lock && \
    rm -rf /build/contrib/grpc_server/.pixi

RUN mkdir -p contrib/grpc_server/build && \
    cd contrib/grpc_server/build && \
    pixi run bash -c ' \
        cmake .. \
         -DCMAKE_BUILD_TYPE=Release \
         -DCMAKE_CXX_FLAGS="-w" \
         -DCMAKE_C_FLAGS="-w" \
         -DCMAKE_INSTALL_PREFIX=/opt/ottd_grpc_server \
         -DCMAKE_INSTALL_RPATH=/build/.pixi/envs/default/lib \
         -DCMAKE_BUILD_WITH_INSTALL_RPATH=ON \
         -DCMAKE_INSTALL_RPATH_USE_LINK_PATH=OFF && \
        cmake --build . -- --jobs=$(nproc) && \
        make install \
    '

RUN install -m 755 /build/contrib/grpc_server/wait_for_uds.sh /opt/ottd_grpc_server/bin/

# === runtime image (pixi.toml/lock + env only; no source or build tree) ===

FROM pixi_base AS runtime

COPY --from=builder /opt/ottd_grpc_server /opt/ottd_grpc_server

# pixi.toml and .pixi env live in /build (from pixi_base); pixi run must start there.
WORKDIR /build

ENV OTTD_GRPC_ENABLED=1 \
    OTTD_GRPC_HOSTNAME=0.0.0.0 \
    OTTD_GRPC_PORT=50051 \
    OTTD_UDS_SOCKET_PATH=/run/openttd/rpc.sock

EXPOSE 50051/tcp

CMD ["/usr/local/bin/pixi", "run", "bash", "-c", "/opt/ottd_grpc_server/bin/wait_for_uds.sh"]
