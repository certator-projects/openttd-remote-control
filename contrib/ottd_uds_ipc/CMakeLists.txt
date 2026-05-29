cmake_minimum_required(VERSION 3.16)

find_package(Boost REQUIRED)

add_library(ottd_uds_ipc INTERFACE)
target_include_directories(ottd_uds_ipc INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
                                                  ${Boost_INCLUDE_DIRS})
