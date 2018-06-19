#!/bin/bash

# @author: Gursimran Singh
# @github: https://github.com/gursimransinghhanspal
# 
# @reference:	1. https://gist.github.com/klynch/8192710
# 				2. https://gist.github.com/nddrylliog/4688209
#				3. https://github.com/RoyGuanyu/build-scripts-of-ffmpeg-x264-for-android-ndk
# 				4. https://developer.android.com/ndk/guides/standalone_toolchain.html
#
# This script cross compiles a library (with makefile and src) for use with 
# the Android platform and its Native Development Kit
# Note: the latest stable release at the time of writing this script is ndk-r17 (17.1.4828580)
#
# The script uses the `standalone toolchains` created using `make_standalone_toolchain.py` in the ndk-bundle.
# For ease use `make_ndk_toolchains.py` provided in this repo to easily create toolchains for all architectures 
# for an api.
#
# Run:
# Call this script from within a directory with a valid configure script.


# the directory containing toolchains for each architecture
export TOOLCHAIN_HOME=/
# the directory where the library shall be installed (placed after compilation)
export INSTALL_DIR=/


if [ -d "${TOOLCHAIN_HOME}/arm" ]
then
	./build_armeabi_v7a.sh
fi

if [ -d "${TOOLCHAIN_HOME}/arm64" ]
then
	# ./build_arm64_v8a.sh
fi

if [ -d "${TOOLCHAIN_HOME}/x86" ]
then
	# ./build_x86.sh
fi

if [ -d "${TOOLCHAIN_HOME}/x86_64" ]
then
	# ./build_x86_64.sh
fi
