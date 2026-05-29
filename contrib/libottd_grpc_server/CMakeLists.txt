cmake_minimum_required(VERSION 3.16)
project(
  libottd_grpc_server
  VERSION 1.0.0
  LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include FetchContent for spdlog include(FetchContent)

# Default Release: match NDEBUG semantics of packaged libprotobuf (see abi_rpc /
# Docker).
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE
      Release
      CACHE STRING "Build type (Debug or Release)" FORCE)
endif()

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Debug build configuration
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  # Add debug symbols and disable optimization
  add_compile_options(-g -O0)

  # Add useful debug flags
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(
      -Wall # Enable most warnings
      -Wextra # Enable extra warnings
      -fno-omit-frame-pointer # Keep frame pointers for better debugging
    )
  endif()

  message(STATUS "Building in Debug mode with debug symbols")
else()
  message(STATUS "Building in ${CMAKE_BUILD_TYPE} mode")
endif()

# gRPC brings Protobuf; require CONFIG exports and explicit libprotobuf-lite for
# --cpp_out=lite generated message code.
find_package(gRPC REQUIRED)
include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/OttdProtobufLite.cmake)
ottd_find_protobuf_lite()

find_package(spdlog REQUIRED)

# Generate protobuf and gRPC filesFetchContent_Declare
set(PROTO_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/proto/console.proto
    ${CMAKE_CURRENT_SOURCE_DIR}/proto/admin.proto
    ${CMAKE_CURRENT_SOURCE_DIR}/proto/script_company.proto
    ${CMAKE_CURRENT_SOURCE_DIR}/proto/script_generic.proto
    ${CMAKE_CURRENT_SOURCE_DIR}/proto/script_goal.proto
    ${CMAKE_CURRENT_SOURCE_DIR}/proto/script_map.proto)

# Get protoc and grpc_cpp_plugin executables
get_target_property(PROTOC_EXECUTABLE protobuf::protoc IMPORTED_LOCATION)
if(NOT PROTOC_EXECUTABLE)
  find_program(PROTOC_EXECUTABLE protoc)
endif()

find_program(GRPC_CPP_PLUGIN grpc_cpp_plugin)
if(NOT GRPC_CPP_PLUGIN)
  message(FATAL_ERROR "grpc_cpp_plugin not found")
endif()

# Set up generated files directory
set(PROTO_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
file(MAKE_DIRECTORY ${PROTO_OUT_DIR})

# Generate protobuf and gRPC sources
set(PROTO_SRCS)
set(PROTO_HDRS)
set(GRPC_SRCS)
set(GRPC_HDRS)

foreach(proto_file ${PROTO_FILES})
  get_filename_component(PROTO_NAME ${proto_file} NAME_WE)

  list(APPEND PROTO_SRCS "${PROTO_OUT_DIR}/${PROTO_NAME}.pb.cc")
  list(APPEND PROTO_HDRS "${PROTO_OUT_DIR}/${PROTO_NAME}.pb.h")

  # Only generate gRPC code for proto files that define services
  if(PROTO_NAME MATCHES "^(admin|console|script_.*)")
    if(NOT PROTO_NAME STREQUAL "plugin_rpc")
      list(APPEND GRPC_SRCS "${PROTO_OUT_DIR}/${PROTO_NAME}.grpc.pb.cc")
      list(APPEND GRPC_HDRS "${PROTO_OUT_DIR}/${PROTO_NAME}.grpc.pb.h")
    endif()
  endif()
endforeach()

# Generate protobuf code
add_custom_command(
  OUTPUT ${PROTO_SRCS} ${PROTO_HDRS}
  COMMAND ${PROTOC_EXECUTABLE} --cpp_out=lite:${PROTO_OUT_DIR} -I
          ${CMAKE_CURRENT_SOURCE_DIR}/proto ${PROTO_FILES}
  DEPENDS ${PROTO_FILES} protobuf::protoc
  COMMENT "Generating lite protobuf C++ sources"
  VERBATIM)

# Generate gRPC code for service definitions
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

# Build the gRPC server plugin as a shared library
add_library(ottd_grpc_server SHARED grpc_server_plugin.cpp
                                    grpc_server_services.cpp)

# Set the output name
set_target_properties(
  ottd_grpc_server
  PROPERTIES PREFIX "lib"
             OUTPUT_NAME "ottd_grpc_server"
             VERSION ${PROJECT_VERSION}
             SOVERSION 1)

target_include_directories(ottd_grpc_server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Lite messages: ottd_grpc_server_proto + libprotobuf-lite; gRPC stack is
# separate.
target_link_libraries(
  ottd_grpc_server
  PRIVATE ottd_grpc_server_proto gRPC::grpc++ absl::synchronization
          absl::core_headers spdlog::spdlog)
# Installation
include(GNUInstallDirs)

install(
  TARGETS ottd_grpc_server
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(FILES plugin_interface.h
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ottd_grpc_server)

install(
  DIRECTORY proto/
  DESTINATION ${CMAKE_INSTALL_DATADIR}/ottd_grpc_server/proto
  FILES_MATCHING
  PATTERN "*.proto")

# Print configuration
message(STATUS "OpenTTD gRPC Server Plugin Configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Protobuf version: ${Protobuf_VERSION}")
message(STATUS "  Install prefix: ${CMAKE_INSTALL_PREFIX}")
