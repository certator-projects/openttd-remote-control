cmake_minimum_required(VERSION 3.16)
project(
  libottd_uds_bridge
  VERSION 1.0.0
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE
      Release
      CACHE STRING "Build type (Debug or Release)" FORCE)
endif()

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_compile_options(-g -O0)
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -fno-omit-frame-pointer)
  endif()
endif()

find_package(Boost REQUIRED)
find_package(spdlog REQUIRED)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../ottd_uds_ipc
                 ${CMAKE_CURRENT_BINARY_DIR}/ottd_uds_ipc)

add_library(ottd_uds_bridge SHARED uds_bridge_plugin.cpp uds_bridge_server.cpp)

set_target_properties(
  ottd_uds_bridge
  PROPERTIES PREFIX "lib"
             OUTPUT_NAME "ottd_uds_bridge"
             VERSION ${PROJECT_VERSION}
             SOVERSION 1)

target_include_directories(ottd_uds_bridge PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ottd_uds_bridge PRIVATE ottd_uds_ipc spdlog::spdlog)

include(GNUInstallDirs)

install(
  TARGETS ottd_uds_bridge
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(FILES plugin_interface.h
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ottd_uds_bridge)
