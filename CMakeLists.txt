#!/usr/bin/env bash
################################################################################################
#
# Xpedite script to build
#   1. static libraries for linking runtime with application
#   2. Xpedite Demo binary
#   3. Xpedite Kernel module (optional depencdency for using H/W pmc events)
#   4. Xpedite unit test binaries
#
# Author: Manikandan Dhamodharan, Morgan Stanley
#
################################################################################################

cmake_minimum_required(VERSION 3.4.1)
project(xpedite C CXX ASM)
if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 5.2)
  message(FATAL_ERROR "Xpedite requires GCC 5.2 or later to build.")
endif()

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)  # ensure standard is supported
set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -g")
set(CMAKE_EXE_LINKER_FLAGS "-no-pie" CACHE INTERNAL "")

file(GLOB_RECURSE lib_headers lib/xpedite/*.[hH])
file(GLOB_RECURSE lib_source lib/xpedite/*.[cC])
file(GLOB_RECURSE asm_source lib/xpedite/*.S)
set(lib_files ${lib_headers} ${lib_source} ${asm_source})
include_directories(include)

add_library(xpedite STATIC ${lib_files})
target_link_libraries(xpedite pthread rt dl)
install(TARGETS xpedite DESTINATION "lib" COMPONENT libraries)
install(DIRECTORY "include/xpedite" DESTINATION "include" COMPONENT headers)

add_library(xpedite-pie STATIC ${lib_files})
target_link_libraries(xpedite-pie pthread rt dl)
target_compile_definitions(xpedite-pie PRIVATE XPEDITE_PIE=1)
install(TARGETS xpedite-pie DESTINATION "lib" COMPONENT libraries)

add_library(xpedite-stub SHARED lib/stub/Stub.C)
install(TARGETS xpedite-stub DESTINATION "lib" COMPONENT libraries)

file(GLOB_RECURSE bin_headers bin/*.H)
file(GLOB_RECURSE bin_source bin/*.C)
set(bin_files ${bin_headers} ${bin_source})
add_executable(xpediteSamplesLoader ${bin_files})
target_link_libraries(xpediteSamplesLoader xpedite)
install(TARGETS xpediteSamplesLoader DESTINATION "bin" COMPONENT binaries)

add_library(xpedite-pic SHARED ${lib_files})
set_property(TARGET xpedite-pic PROPERTY POSITION_INDEPENDENT_CODE ON)
target_link_libraries(xpedite-pic pthread rt dl)
target_compile_definitions(xpedite-pic PRIVATE XPEDITE_PIE=1)
install(TARGETS xpedite-pic DESTINATION "lib" COMPONENT libraries)

######################### Python bindings #############################

find_package(pybind11 REQUIRED)
pybind11_add_module(xpediteBindings lib/xpedite/pybind/Bindings.cpp)
install(TARGETS xpediteBindings DESTINATION "lib" COMPONENT libraries)

######################### Kernel module #############################

Set(DRIVER_FILE xpedite.ko)
Set(KERNEL_DIR "/lib/modules/${CMAKE_SYSTEM_VERSION}/build")
Set(MODULE_SOURCE_DIR  ${PROJECT_SOURCE_DIR}/ko)

Set(KBUILD_CMD ${CMAKE_MAKE_PROGRAM}
                -C ${KERNEL_DIR}
                M=${CMAKE_BINARY_DIR}/ko
                src=${MODULE_SOURCE_DIR}
                modules)

Set(KBUILD_INSTALL_CMD ${CMAKE_MAKE_PROGRAM}
                -C ${KERNEL_DIR}
                M=${CMAKE_BINARY_DIR}/ko
                #INSTALL_MOD_PATH=${CMAKE_INSTALL_PREFIX}
                modules_install)

Add_Custom_Command(OUTPUT ${DRIVER_FILE}
                   COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/ko
                   COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_BINARY_DIR}/ko/Makefile
                   COMMAND ${KBUILD_CMD}
                   DEPENDS ${MODULE_SOURCE_DIR} ko VERBATIM)

Add_Custom_Target (driver DEPENDS ${DRIVER_FILE})

Add_Custom_Target (driver_install
                   COMMAND ${KBUILD_INSTALL_CMD}
                   DEPENDS driver)


######################### demo #############################
file(GLOB_RECURSE demo_headers demo/*.H)
set(demo_files ${demo_headers} demo/Demo.C)
add_executable(xpediteDemo ${demo_files})
target_link_libraries(xpediteDemo xpedite)
install(TARGETS xpediteDemo DESTINATION "bin" COMPONENT binaries)

######################### targets #############################
add_executable(allocatorApp test/targets/AllocatorApp.C)
SET(ALLOCATOR_LINK_FLAGS "-Wl,-wrap,_Znwm,-wrap,_Znam,-wrap,malloc,-wrap,calloc,-wrap,realloc,-wrap,posix_memalign,-wrap,valloc,-wrap,free,-wrap,mmap,-wrap,munmap")
set_property(TARGET allocatorApp APPEND_STRING PROPERTY LINK_FLAGS " ${ALLOCATOR_LINK_FLAGS}")
target_link_libraries(allocatorApp xpedite)
install(TARGETS allocatorApp DESTINATION "test" COMPONENT testBinaries)

add_executable(embeddedApp test/targets/EmbeddedApp.C)
target_link_libraries(embeddedApp xpedite)
install(TARGETS embeddedApp DESTINATION "test" COMPONENT testBinaries)

add_executable(customRecorderApp test/targets/CustomRecorderApp.C)
target_link_libraries(customRecorderApp xpedite)
install(TARGETS customRecorderApp DESTINATION "test" COMPONENT testBinaries)

add_executable(multiThreadedApp test/targets/MultiThreadedApp.C)
target_link_libraries(multiThreadedApp xpedite)
install(TARGETS multiThreadedApp DESTINATION "test" COMPONENT testBinaries)

add_executable(dataTxnApp test/targets/DataTxnApp.C)
target_link_libraries(dataTxnApp xpedite)
install(TARGETS dataTxnApp DESTINATION "test" COMPONENT testBinaries)

add_library(pic SHARED test/targets/PicLib.C)
target_compile_definitions(pic PRIVATE XPEDITE_PIC=1)
install(TARGETS pic DESTINATION "test" COMPONENT testBinaries)

add_executable(picApp test/targets/PicApp.C)
target_link_libraries(picApp xpedite pic)
install(TARGETS picApp DESTINATION "test" COMPONENT testBinaries)

add_executable(slowFixDecoderApp test/targets/SlowFixDecoder.C)
target_link_libraries(slowFixDecoderApp xpedite)
install(TARGETS slowFixDecoderApp DESTINATION "test" COMPONENT testBinaries)

######################### test #############################

enable_testing()

# GTest
find_package(GTest)
if(GTEST_FOUND)
  include_directories(${GTEST_INCLUDE_DIRS})
  file(GLOB_RECURSE test_headers test/gtest/*.H)
  file(GLOB_RECURSE test_source test/gtest/*.C)
  set(test_files ${test_headers} ${test_source})
  add_executable(testXpedite ${test_files})
  target_link_libraries(testXpedite ${GTEST_BOTH_LIBRARIES} xpedite)
  install(TARGETS testXpedite DESTINATION "test" COMPONENT testBinaries)
  add_test(NAME testXpedite
         COMMAND testXpedite)
endif()

if(BUILD_JAVA)
message("Starting Xpedite Java Setup")
add_subdirectory(jni)
endif()

if(BUILD_VIVIFY)
message("Starting Vivify Library Setup")
add_subdirectory(vivify)
endif()

######################### package #############################
set(CPACK_GENERATOR "TGZ;DEB;RPM")
set(CPACK_ARCHIVE_COMPONENT_INSTALL ON)
set(CPACK_DEB_COMPONENT_INSTALL ON)
set(CPACK_RPM_COMPONENT_INSTALL ON)
set(CPACK_COMPONENTS_ALL_IN_ONE_PACKAGE ON)
set(CPACK_COMPONENTS_ALL headers libraries binaries)

set(CPACK_PACKAGE_NAME "Xpedite")
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_RPM_PACKAGE_RELEASE "1.0.0")
set(CPACK_PACKAGE_VENDOR "Xpedite")
set(CPACK_PACKAGE_CONTACT "Manikandan Dhamodharan")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://www.xpedite.dev")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Non-sampling profiler for low-latency/real time systems")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_SOURCE_DIR}/docs/pkgDesc.txt")

include(CPack)
