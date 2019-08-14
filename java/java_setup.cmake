#!/usr/bin/env bash
################################################################################################
#
# Xpedite script to compile Java files to create .jar files and .h files
#
# Author: Brooke Elizabeth Cantwell, Morgan Stanley
#
################################################################################################

function(java_setup)
    project(xpedite C CXX ASM)

    find_package(Java REQUIRED)
    find_package(JNI REQUIRED)
    include(UseJava)
    find_package(Java 1.8)

    set(CMAKE_ORIGINAL_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})
    set(CMAKE_CURRENT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/java/Xpedite)
    set(CMAKE_JAVA_INCLUDE_PATH ${CMAKE_ORIGINAL_SRC_DIR}/java/jar/javassist.jar ${JAVA_HOME}/lib/tools.jar)
    set(JAVA_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/com/xpedite)
    file(GLOB_RECURSE JAVA_SOURCE_FILES ${JAVA_SOURCE_DIR}/*.java)

    set(XPEDITE_INSTALL_PATH "${CMAKE_ORIGINAL_SRC_DIR}/install/java/jar")
    add_jar(Xpedite
        SOURCES ${JAVA_SOURCE_FILES}
        OUTPUT_DIR ${XPEDITE_INSTALL_PATH}
        MANIFEST ${CMAKE_CURRENT_SOURCE_DIR}/src/META-INF/MANIFEST.MF
    )

    set(XPEDITE_JNI_HEADER_PATH ${CMAKE_ORIGINAL_SRC_DIR}/install/jni)
    execute_process(
        COMMAND javah -d ${XPEDITE_JNI_HEADER_PATH} com.xpedite.Xpedite
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src
    )

    set(CMAKE_INSTALL_PREFIX "java")
    create_javadoc(xpedite-javadocs
        FILES ${JAVA_SOURCE_FILES}
        CLASSPATH Xpedite.jar ${CMAKE_JAVA_INCLUDE_PATH}
        DOCTITLE "Xpedite Java Instrumentation"
    )

    set(CMAKE_CURRENT_SOURCE_DIR ${CMAKE_ORIGINAL_SRC_DIR})
    include_directories(include ${XPEDITE_JNI_HEADER_PATH} ${JNI_INCLUDE_DIRS} ${XPEDITE_JNI_HEADER_PATH})
    file(GLOB JNI_FILES "lib/xpediteJni/*.C")
    add_library(XpediteJNI SHARED ${JNI_FILES})
    target_link_libraries (XpediteJNI xpedite-pic ${JAVA_JVM_LIBRARY})
    install(TARGETS XpediteJNI DESTINATION "lib")

    #create symlink for javassist jar file
    ADD_CUSTOM_TARGET(link_target ALL COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_ORIGINAL_SRC_DIR}/java/jar/javassist.jar ${XPEDITE_INSTALL_PATH}/javassist.jar)

endfunction()
