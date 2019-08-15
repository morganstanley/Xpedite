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

execute_process(
  COMMAND gradle build
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

get_filename_component(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR} DIRECTORY)
set(XPEDITE_JNI_HEADER_PATH ${SRC_DIR}/install/jni)
include_directories(include ${XPEDITE_JNI_HEADER_PATH} ${JNI_INCLUDE_DIRS})
file(GLOB JNI_FILES "lib/*.C")
add_library(XpediteJNI SHARED ${JNI_FILES})
target_link_libraries (XpediteJNI xpedite-pic ${JAVA_JVM_LIBRARY})
install(TARGETS XpediteJNI DESTINATION "lib")

#create symlink for javassist jar file
set(XPEDITE_INSTALL_PATH ${CMAKE_CURRENT_SOURCE_DIR}/"build/libs")
ADD_CUSTOM_TARGET(link_target ALL COMMAND ${CMAKE_COMMAND} -E create_symlink "${CMAKE_CURRENT_SOURCE_DIR}/jar/javassist.jar" ${XPEDITE_INSTALL_PATH}/javassist.jar)
