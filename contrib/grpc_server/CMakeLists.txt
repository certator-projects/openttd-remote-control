cmake_minimum_required(VERSION 3.16)
project(
  grpc_server
  VERSION 1.0.0
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE
      Release
      CACHE STRING "Build type (Debug or Release)" FORCE)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_compile_options(-g -O0)
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -fno-omit-frame-pointer)
  endif()
endif()

find_package(gRPC REQUIRED)
find_package(Boost REQUIRED)
include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/OttdProtobufLite.cmake)
ottd_find_protobuf_lite()
find_package(spdlog REQUIRED)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../ottd_uds_ipc
                 ${CMAKE_CURRENT_BINARY_DIR}/ottd_uds_ipc)

set(PROTO_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/proto/console.proto
    ${CMAKE_CURRENT_SOURCE_DIR}/proto/admin.proto
    ${CMAKE_CURRENT_SOURCE_DIR}/proto/script_company.proto
    ${CMAKE_CURRENT_SOURCE_DIR}/proto/script_generic.proto
    ${CMAKE_CURRENT_SOURCE_DIR}/proto/script_goal.proto
    ${CMAKE_CURRENT_SOURCE_DIR}/proto/script_map.proto)

get_target_property(PROTOC_EXECUTABLE protobuf::protoc IMPORTED_LOCATION)
if(NOT PROTOC_EXECUTABLE)
  find_program(PROTOC_EXECUTABLE protoc)
endif()

find_program(GRPC_CPP_PLUGIN grpc_cpp_plugin)
if(NOT GRPC_CPP_PLUGIN)
  message(FATAL_ERROR "grpc_cpp_plugin not found")
endif()

set(PROTO_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
file(MAKE_DIRECTORY ${PROTO_OUT_DIR})

set(PROTO_SRCS)
set(PROTO_HDRS)
set(GRPC_SRCS)
set(GRPC_HDRS)

foreach(proto_file ${PROTO_FILES})
  get_filename_component(PROTO_NAME ${proto_file} NAME_WE)

  list(APPEND PROTO_SRCS "${PROTO_OUT_DIR}/${PROTO_NAME}.pb.cc")
  list(APPEND PROTO_HDRS "${PROTO_OUT_DIR}/${PROTO_NAME}.pb.h")

  if(PROTO_NAME MATCHES "^(admin|console|script_.*)")
    list(APPEND GRPC_SRCS "${PROTO_OUT_DIR}/${PROTO_NAME}.grpc.pb.cc")
    list(APPEND GRPC_HDRS "${PROTO_OUT_DIR}/${PROTO_NAME}.grpc.pb.h")
  endif()
endforeach()

add_custom_command(
  OUTPUT ${PROTO_SRCS} ${PROTO_HDRS}
  COMMAND ${PROTOC_EXECUTABLE} --cpp_out=lite:${PROTO_OUT_DIR} -I
          ${CMAKE_CURRENT_SOURCE_DIR}/proto ${PROTO_FILES}
  DEPENDS ${PROTO_FILES} protobuf::protoc
  COMMENT "Generating lite protobuf C++ sources"
  VERBATIM)

add_custom_command(
  OUTPUT ${GRPC_SRCS} ${GRPC_HDRS}
  COMMAND
    ${PROTOC_EXECUTABLE} --grpc_out=${PROTO_OUT_DIR}
    --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN} -I
    ${CMAKE_CURRENT_SOURCE_DIR}/proto
    ${CMAKE_CURRENT_SOURCE_DIR}/proto/console.proto
    ${CMAKE_CURRENT_SOURCE_DIR}/proto/admin.proto
    ${CMAKE_CURRENT_SOURCE_DIR}/proto/script_company.proto
    ${CMAKE_CURRENT_SOURCE_DIR}/proto/script_generic.proto
    ${CMAKE_CURRENT_SOURCE_DIR}/proto/script_goal.proto
    ${CMAKE_CURRENT_SOURCE_DIR}/proto/script_map.proto
  DEPENDS ${PROTO_FILES} protobuf::protoc
  COMMENT "Generating gRPC C++ sources"
  VERBATIM)

add_library(ottd_grpc_server_proto STATIC ${PROTO_SRCS} ${GRPC_SRCS})
target_include_directories(ottd_grpc_server_proto
                           PUBLIC ${PROTO_OUT_DIR} ${Protobuf_INCLUDE_DIRS})
ottd_target_link_protobuf_lite(ottd_grpc_server_proto PUBLIC)

add_executable(ottd_grpc_server main.cpp grpc_server_services.cpp)

target_include_directories(ottd_grpc_server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(
  ottd_grpc_server
  PRIVATE ottd_grpc_server_proto ottd_uds_ipc gRPC::grpc++
          absl::synchronization absl::core_headers spdlog::spdlog)

include(GNUInstallDirs)

install(TARGETS ottd_grpc_server RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(
  DIRECTORY proto/
  DESTINATION ${CMAKE_INSTALL_DATADIR}/grpc_server/proto
  FILES_MATCHING
  PATTERN "*.proto")
