# Helpers for linking protobuf::libprotobuf-lite consistently across platforms.
#
# Linux (pixi/conda): libprotobuf 3.x, static, no abseil in INTERFACE_LINK_LIBRARIES.
# macOS (Homebrew): libprotobuf 4+/7+, shared, INTERFACE_LINK_LIBRARIES lists abseil
# targets — find_package(absl) must run before linking protobuf::libprotobuf-lite.

function(ottd_find_protobuf_lite)
  set(protobuf_MODULE_COMPATIBLE ON)
  find_package(Protobuf CONFIG QUIET)
  if(NOT Protobuf_FOUND)
    find_package(Protobuf REQUIRED)
  endif()
  if(NOT TARGET protobuf::libprotobuf-lite)
    message(
      FATAL_ERROR "protobuf::libprotobuf-lite not found; install libprotobuf")
  endif()
  if(APPLE)
    find_package(absl CONFIG REQUIRED)
  endif()
endfunction()

function(ottd_target_link_protobuf_lite target visibility)
  target_link_libraries(${target} ${visibility} protobuf::libprotobuf-lite)
endfunction()