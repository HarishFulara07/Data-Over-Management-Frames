#
# Copyright (C) The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

cmake_minimum_required(VERSION 3.4.1)

# Distribution Folder
# the distribution folder contains all the 3rd party libraries
# either precompiled or sources.
# the header files and include directories are also located here
set(distribution_DIR ${CMAKE_SOURCE_DIR}/../../../../distribution)

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

#include_directories(
#    ${distribution_DIR}/nl/include/${ANDROID_ABI}
#)

# build application's shared lib
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++14")

# JNI-HANDLER
add_library(
    jni-handler SHARED jni-handler.cpp
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

