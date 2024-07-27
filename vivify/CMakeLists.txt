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

cmake_minimum_required(VERSION 3.4.1)
project(vivify C CXX)
find_package(PkgConfig REQUIRED)
include(CheckSymbolExists)
include(CheckIncludeFile)

# vivify external dependencies
#   needs libunwind, libunwind-x86_64 to extract instruction pointers from stack frames
#   unwinding stack needs extraction of eh_frame section, which in turn requires:
#     1. libelf to extract eh_frame header
#     2. dwarf.h to decode eh_frame fields
#   needs libbfd to map instruction pointers to function, file and line no

check_include_file("dwarf.h" DWARF_H_FOUND)
if(NOT DWARF_H_FOUND)
  message(WARNING "skipping vivify build. dwarf.h not found!!!")
  return()
endif()

set(CMAKE_REQUIRED_DEFINITIONS -DPACKAGE -DPACKAGE_VERSION)
check_include_file("bfd.h" BFD_H_FOUND)
set(CMAKE_REQUIRED_DEFINITIONS)
if(NOT BFD_H_FOUND)
  message(WARNING "skipping vivify build. bfd.h not found!!!")
  return()
endif()

find_library(LIBELF_FOUND elf)
if(NOT LIBELF_FOUND)
  message(WARNING "skipping vivify build. libelf not found!!!")
  return()
endif()

set(CMAKE_REQUIRED_DEFINITIONS -DPACKAGE -DPACKAGE_VERSION)
check_symbol_exists(bfd_section_flags "bfd.h" HAVE_BFD_SECTION_FLAGS)
set(CMAKE_REQUIRED_DEFINITIONS)

include_directories(include)
file(GLOB_RECURSE lib_vivify_headers lib/vivify/*.[hH])
file(GLOB_RECURSE lib_vivify_source lib/vivify/*.[cC])

pkg_check_modules(LIBUNWIND libunwind)
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

add_library(vivify STATIC ${lib_vivify_headers} ${lib_vivify_source})
if(NOT HAVE_BFD_SECTION_FLAGS)
  target_compile_definitions(vivify PRIVATE NO_BFD_SECTION_FLAGS)
endif()

if(LIBUNWIND_FOUND)
  if(NOT LIBUNWIND_DEBUG_FRAME)
    target_compile_definitions(vivify PRIVATE NO_LIBUNWIND_DEBUG_FRAME)
  endif()

  target_include_directories(vivify PRIVATE ${LIBUNWIND_INCLUDE_DIRS})
  target_link_libraries(vivify rt elf unwind-x86_64 unwind bfd)
  install(DIRECTORY "include/vivify" DESTINATION "include")
else()
  target_link_libraries(vivify rt bfd)
  install(DIRECTORY "include/vivify" DESTINATION "include" PATTERN "StackUnwind.H" EXCLUDE)
endif()
install(TARGETS vivify DESTINATION "lib" COMPONENT libraries)

if(LIBUNWIND_FOUND)
  add_executable(stackunwindDemo demo/StackUnwind.C)
  target_include_directories(stackunwindDemo PRIVATE ${LIBUNWIND_INCLUDE_DIRS})
  target_link_libraries(stackunwindDemo vivify)
  install(TARGETS stackunwindDemo DESTINATION "test" COMPONENT testBinaries)
endif()
