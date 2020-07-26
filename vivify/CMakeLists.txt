#!/usr/bin/env bash
################################################################################################
#
# CMake input to build:
#   1. vivify static library
#   2. offline stack unwind demo binary
#
# Author: Andrew C., Morgan Stanley
#
################################################################################################

project(vivify C CXX)
check_include_file("dwarf.h" FOUND_DWARF_H)
if(NOT FOUND_DWARF_H)
  message("skipping vivify build. dwarf.h not found !!!")
  return()
endif()


find_package(PkgConfig REQUIRED)

pkg_check_modules(LIBUNWIND libunwind)

include_directories(include)
file(GLOB_RECURSE lib_vivify_headers lib/vivify/*.[hH])
file(GLOB_RECURSE lib_vivify_source lib/vivify/*.[cC])
if(LIBUNWIND_FOUND)
  link_directories(${LIBUNWIND_LIBRARY_DIRS})
  try_compile(LIBUNWIND_DEBUG_FRAME ${CMAKE_BINARY_DIR}/tmp
              ${CMAKE_CURRENT_SOURCE_DIR}/cmake/test/LibunwindDebugFrame.C
              CMAKE_FLAGS "-DINCLUDE_DIRECTORIES=${LIBUNWIND_INCLUDE_DIRS}"
                          "-DLINK_DIRECTORIES=${LIBUNWIND_LIBRARY_DIRS}"
              LINK_LIBRARIES unwind-x86_64)
  message(STATUS "LIBUNWIND_DEBUG_FRAME=${LIBUNWIND_DEBUG_FRAME}")
else()
  list(REMOVE_ITEM lib_vivify_headers ${CMAKE_CURRENT_SOURCE_DIR}/lib/vivify/StackUnwind.H)
  list(REMOVE_ITEM lib_vivify_source ${CMAKE_CURRENT_SOURCE_DIR}/lib/vivify/StackUnwind.C)
endif()
set(lib_vivify_files ${lib_vivify_headers} ${lib_vivify_source})
add_library(vivify STATIC ${lib_vivify_files})

if(LIBUNWIND_FOUND)
  if(NOT LIBUNWIND_DEBUG_FRAME)
    target_compile_definitions(vivify PRIVATE NO_LIBUNWIND_DEBUG_FRAME)
  endif()

  # begin workaround to embed libunwind
  # https://sourceware.org/bugzilla/show_bug.cgi?id=14243
  # https://sourceware.org/bugzilla/show_bug.cgi?id=15920
  add_definitions(-DPACKAGE)
  add_definitions(-DPACKAGE_VERSION)
  # end of workaround

  target_include_directories(vivify PRIVATE ${LIBUNWIND_INCLUDE_DIRS})
  target_link_libraries(vivify rt elf unwind-x86_64 unwind bfd)
  install(DIRECTORY "include/vivify" DESTINATION "include")
else()
  target_link_libraries(vivify rt bfd)
  install(DIRECTORY "include/vivify" DESTINATION "include" PATTERN "StackUnwind.H" EXCLUDE)
endif()
install(TARGETS vivify DESTINATION "lib")

if(LIBUNWIND_FOUND)
  add_executable(stackunwindDemo demo/StackUnwind.C)
  target_include_directories(stackunwindDemo PRIVATE ${LIBUNWIND_INCLUDE_DIRS})
  target_link_libraries(stackunwindDemo vivify)
  install(TARGETS stackunwindDemo DESTINATION "test")
endif()
