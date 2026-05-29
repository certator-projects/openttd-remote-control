cmake_minimum_required(VERSION 3.16)
project(
  libottd_rpc_example
  VERSION 1.0.0
  LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include FetchContent for spdlog include(FetchContent)

# Default Release: conda/pixi libprotobuf is built with NDEBUG; Debug plugin TUs
# need matching NDEBUG semantics or link will fail (protobuf 3.20+).
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

include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/OttdProtobufLite.cmake)
ottd_find_protobuf_lite()

find_package(spdlog REQUIRED)

# Generate protobuf LITE sources; link libprotobuf-lite into the plugin
# (self-contained)
set(PROTO_FILES ${CMAKE_CURRENT_SOURCE_DIR}/proto/console.proto
                ${CMAKE_CURRENT_SOURCE_DIR}/proto/admin.proto)

# Get protoc executable path
get_target_property(PROTOC_EXECUTABLE protobuf::protoc IMPORTED_LOCATION)
if(NOT PROTOC_EXECUTABLE)
  # Fallback to finding protoc in PATH
  find_program(PROTOC_EXECUTABLE protoc)
endif()

# Generate lite protobuf code
set(PROTO_SRCS)
set(PROTO_HDRS)
foreach(proto_file ${PROTO_FILES})
  get_filename_component(PROTO_NAME ${proto_file} NAME_WE)
  list(APPEND PROTO_SRCS "${CMAKE_CURRENT_BINARY_DIR}/${PROTO_NAME}.pb.cc")
  list(APPEND PROTO_HDRS "${CMAKE_CURRENT_BINARY_DIR}/${PROTO_NAME}.pb.h")
endforeach()

add_custom_command(
  OUTPUT ${PROTO_SRCS} ${PROTO_HDRS}
  COMMAND ${PROTOC_EXECUTABLE} ARGS --cpp_out=lite:${CMAKE_CURRENT_BINARY_DIR}
          -I ${CMAKE_CURRENT_SOURCE_DIR}/proto ${PROTO_FILES}
  DEPENDS ${PROTO_FILES} protobuf::protoc
  COMMENT "Generating lite protobuf C++ sources"
  VERBATIM)

add_library(ottd_rpc_example_proto STATIC ${PROTO_SRCS})
target_include_directories(
  ottd_rpc_example_proto PUBLIC ${CMAKE_CURRENT_BINARY_DIR}
                                ${Protobuf_INCLUDE_DIRS})
ottd_target_link_protobuf_lite(ottd_rpc_example_proto PUBLIC)

# Build the example plugin as a shared library
add_library(ottd_rpc_example SHARED example_plugin.cpp)

# Set the output name
set_target_properties(
  ottd_rpc_example
  PROPERTIES PREFIX "lib"
             OUTPUT_NAME "ottd_rpc_example"
             VERSION ${PROJECT_VERSION}
             SOVERSION 1)

target_include_directories(ottd_rpc_example PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(ottd_rpc_example PRIVATE ottd_rpc_example_proto
                                               spdlog::spdlog)
# Installation
include(GNUInstallDirs)

install(
  TARGETS ottd_rpc_example
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(FILES plugin_interface.h
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ottd_rpc_example)

install(FILES proto/admin.proto proto/console.proto
        DESTINATION ${CMAKE_INSTALL_DATADIR}/ottd_rpc_example/proto)

# Print configuration
message(STATUS "OpenTTD RPC Example Plugin Configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Protobuf version: ${Protobuf_VERSION}")
message(STATUS "  Install prefix: ${CMAKE_INSTALL_PREFIX}")
