#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SOCKET_PATH="${OTTD_UDS_SOCKET_PATH:-/run/openttd/rpc.sock}"

for _ in $(seq 1 120); do
  if [[ -S "${SOCKET_PATH}" ]]; then
    exec "${SCRIPT_DIR}/ottd_grpc_server" "$@"
  fi
  sleep 1
done

echo "Timed out waiting for UDS socket at ${SOCKET_PATH}" >&2
exit 1
