#!/usr/bin/env bash

CALLER_PATH=$(pwd)
SCRIPT_FULL_PATH=$(realpath $0)
ROOT_DIRECTORY_FULL_PATH=$(dirname $SCRIPT_FULL_PATH)

PROJECT_ROOT=$ROOT_DIRECTORY_FULL_PATH

cd ${PROJECT_ROOT}
THIS_SCRIPT=${PROJECT_ROOT}/manage.sh


if [[ "$1" == "copy-protos-from-openttd" ]]; then
    cd ${PROJECT_ROOT}../..
    cp src/3rdparty/extras/abi_rpc/proto/*.proto ${PROJECT_ROOT}/grpc_protos/
    echo "Protos copied from openttd-private"

elif [[ "$1" == "generate_proto_models" ]]; then
    uv run python -m controller.generate_proto_models

elif [[ "$1" == "run_company_value_goal_server" ]]; then
    uv run python -m controller.company_value_goal.server

elif [[ "$1" == "random-id" ]]; then
    echo $(openssl rand -base64 60 | tr -cd 'a-zA-Z0-9' | head -c $2)

elif [[ "$1" == "mypy" ]]; then
    uv run mypy -p controller -p grpc_gen

else
    echo "Unknown command: $1"
    exit 1
fi

exit
