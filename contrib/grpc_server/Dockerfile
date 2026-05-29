# Standalone gRPC server for OpenTTD (Apache-2.0 stack).
# Connects to the in-process UDS bridge plugin over a shared Unix domain socket.
# Use with docker-compose.yml (grpc-server service).

FROM ghcr.io/prefix-dev/pixi:latest AS pixi_base

WORKDIR /build

COPY ./contrib/grpc_server/pixi.toml ./pixi.toml
COPY ./contrib/grpc_server/pixi.lock ./pixi.lock

RUN pixi workspace platform add linux-aarch64 && \
    pixi install

FROM pixi_base AS runtime

COPY cmake/OttdProtobufLite.cmake ./cmake/OttdProtobufLite.cmake
COPY contrib/ottd_uds_ipc/ /build/contrib/ottd_uds_ipc/
COPY contrib/grpc_server/ /build/contrib/grpc_server/

RUN --mount=type=cache,target=/root/.cache/ccache \
    mkdir -p contrib/grpc_server/build && \
    cd contrib/grpc_server/build && \
    pixi run bash -c ' \
        cmake .. \
         -DCMAKE_C_COMPILER_LAUNCHER="$(command -v ccache)" \
         -DCMAKE_CXX_COMPILER_LAUNCHER="$(command -v ccache)" \
         -DCMAKE_BUILD_TYPE=Release \
         -DCMAKE_CXX_FLAGS="-w" \
         -DCMAKE_C_FLAGS="-w" && \
        cmake --build . -- --jobs=$(nproc) \
    ' && \
    chmod +x /build/contrib/grpc_server/wait_for_uds.sh

WORKDIR /build/contrib/grpc_server/build

ENV OTTD_GRPC_ENABLED=1 \
    OTTD_GRPC_HOSTNAME=0.0.0.0 \
    OTTD_GRPC_PORT=50051 \
    OTTD_UDS_SOCKET_PATH=/run/openttd/rpc.sock

EXPOSE 50051/tcp

CMD ["/usr/local/bin/pixi", "run", "bash", "-c", "/build/contrib/grpc_server/wait_for_uds.sh"]
