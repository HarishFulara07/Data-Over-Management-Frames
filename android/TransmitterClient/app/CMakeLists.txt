# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

# Distribution Folder
# the distribution folder contains all the 3rd party libraries
# either precompiled or sources.
# the header files and include directories are also located here
set(distribution_DIR ${CMAKE_SOURCE_DIR}/../distribution)


# LOG
find_library(
    log-lib
    log
)

# LIBNL
#add_library(
#    nl-3 STATIC IMPORTED
#)
#set_target_properties(
#    nl-3 PROPERTIES
#    IMPORTED_LOCATION ${distribution_DIR}/nl/lib/${ANDROID_ABI}/libnl-3.a
#)

# LIBNL-GENL
#add_library(
#    nl-genl-3 STATIC IMPORTED
#)
#set_target_properties(
#    nl-genl-3 PROPERTIES
#    IMPORTED_LOCATION ${distribution_DIR}/nl/lib/${ANDROID_ABI}/libnl-genl-3.a
#)

# build application's shared lib
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++14")

# JNI-HANDLER
add_library(
    jni-handler SHARED src/main/cpp/jni-handler.cpp
)
#
target_link_libraries(
    jni-handler
#    driver
    android
    log
)

# DRIVER
#add_library(
#    driver SHARED driver.cpp
#)
#
#target_link_libraries(
#    driver
#    nl-callbacks
#    utility
#    android
#    log
#)

# NL-CALLBACKS
#add_library(
#    nl-callbacks SHARED nl-callbacks.cpp
#)
#
#target_link_libraries(
#    nl-callbacks
#    nl-3
#    nl-genl-3
#    android
#    log
#)

# UTILITY
#add_library(
#    utility SHARED utility.cpp
#)
#
#target_link_libraries(
#    utility
#    nl-callbacks
#    nl-3
#    nl-genl-3
#    android
#    log
#)

#
target_include_directories(jni-handler PRIVATE)