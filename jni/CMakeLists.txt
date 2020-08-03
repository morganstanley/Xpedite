#!/usr/bin/env bash
################################################################################################
#
# Xpedite script to compile Java files to create .jar files and .h files
#
# Author: Brooke Elizabeth Cantwell, Morgan Stanley
#
################################################################################################

project(xpedite C CXX ASM)
find_package(Java REQUIRED)
find_package(JNI REQUIRED)
include(UseJava)
find_package(Java 1.8)

find_program(GRADLE gradle)
if(NOT GRADLE)
  message(WARNING "skipping java support. gradle not found !!!")
  return()
endif()

set(ENV{XPEDITE_JARPATH} ${CMAKE_CURRENT_SOURCE_DIR}/jar)
execute_process(
  COMMAND gradle build
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  RESULT_VARIABLE GRADLE_BUILD_STATUS
)

if(NOT GRADLE_BUILD_STATUS EQUAL 0)
  message(FATAL_ERROR "gradle build failed to generate jni headers!!!")
  return()
endif()

get_filename_component(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR} DIRECTORY)
set(XPEDITE_JNI_HEADER_PATH ${SRC_DIR}/install/jni)
include_directories(include ${XPEDITE_JNI_HEADER_PATH} ${JNI_INCLUDE_DIRS})
file(GLOB JNI_FILES "lib/*.C")
add_library(XpediteJNI SHARED ${JNI_FILES})
target_link_libraries (XpediteJNI xpedite-pic ${JAVA_JVM_LIBRARY})
install(TARGETS XpediteJNI DESTINATION "lib")
