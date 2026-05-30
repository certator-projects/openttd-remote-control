set(GENERATED_BINARY_DIR ${CMAKE_BINARY_DIR}/generated)
set(GENERATED_ABI_RPC_DIR ${GENERATED_BINARY_DIR}/abi_rpc)

# ------------------------------------------------------------------
# Use installed Protobuf
# ------------------------------------------------------------------

include(${CMAKE_SOURCE_DIR}/cmake/OttdProtobufLite.cmake)
ottd_find_protobuf_lite()
message(STATUS "Using protobuf ${Protobuf_VERSION} for abi_rpc")

find_program(PROTOBUF_PROTOC protoc)

# ------------------------------------------------------------------
# Protobuf code generation
# ------------------------------------------------------------------
set(PROTO_DIR ${CMAKE_CURRENT_SOURCE_DIR}/proto)

# List all proto files
set(PROTO_FILES
    ${PROTO_DIR}/console.proto
    ${PROTO_DIR}/admin.proto
    ${PROTO_DIR}/script_map.proto
    ${PROTO_DIR}/script_goal.proto
    ${PROTO_DIR}/script_company.proto
    ${PROTO_DIR}/script_generic.proto
    ${PROTO_DIR}/abi_internal.proto)

# Generate output file lists
set(PROTO_SRCS)
set(PROTO_HDRS)

foreach(proto_file ${PROTO_FILES})
  get_filename_component(PROTO_NAME ${proto_file} NAME_WE)
  list(APPEND PROTO_SRCS "${GENERATED_ABI_RPC_DIR}/${PROTO_NAME}.pb.cc")
  list(APPEND PROTO_HDRS "${GENERATED_ABI_RPC_DIR}/${PROTO_NAME}.pb.h")
endforeach()

message(STATUS "ABI RPC PROTOBUF_PROTOC = ${PROTOBUF_PROTOC}")
message(STATUS "ABI RPC PROTO_SRCS = ${PROTO_SRCS}")
message(STATUS "ABI RPC PROTO_HDRS = ${PROTO_HDRS}")

# Generate all proto files
add_custom_command(
  OUTPUT ${PROTO_SRCS} ${PROTO_HDRS}
  COMMAND ${CMAKE_COMMAND} -E cmake_echo_color --cyan
          "=== Generating protobuf files for abi_rpc ==="
  COMMAND ${CMAKE_COMMAND} -E make_directory ${GENERATED_ABI_RPC_DIR}
  COMMAND ${PROTOBUF_PROTOC} ARGS --cpp_out=lite:"${GENERATED_ABI_RPC_DIR}" -I
          "${PROTO_DIR}" ${PROTO_FILES}
  COMMAND ${CMAKE_COMMAND} -E cmake_echo_color --cyan
          "=== Protobuf generation complete for abi_rpc ==="
  DEPENDS ${PROTO_FILES}
  COMMENT "Generating protobuf files for abi_rpc")

add_library(openttd_abi_rpc_proto ${PROTO_SRCS} ${PROTO_HDRS})

ottd_target_link_protobuf_lite(openttd_abi_rpc_proto PUBLIC)

target_include_directories(openttd_abi_rpc_proto PUBLIC ${GENERATED_BINARY_DIR})

# ------------------------------------------------------------------
# ABI RPC implementation
# ------------------------------------------------------------------

add_files(
  scoped_memory_manager.cpp
  plugin_loader.cpp
  rpc_handler.cpp
  host_errors.cpp
  deferred_result.cpp
  chat_notifications.cpp
  handlers/script_company.cpp
  handlers/script_goal.cpp
  handlers/script_map.cpp
  handlers/abi_internal.cpp
  handlers/rpc_console.cpp
  handlers/admin.cpp)
