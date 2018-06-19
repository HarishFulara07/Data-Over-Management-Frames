#!/bin/bash


# ----- ABI Specific Configuration -----

# the target abi name (the name of toolchain dir, the name of install dir)
export TARGET_ABI=x86_64
# the name of the compilation target (also known as `host`)
export TARGET_HOST=x86_64-linux-android
# the path to toolchain root
export TOOLCHAIN_ROOT=${TOOLCHAIN_HOME}/${TARGET_ABI}
# the path to toolchain binaries
export TOOLCHAIN_BIN_PATH=${TOOLCHAIN_ROOT}/bin
# the toolchain sysroot
export SYSROOT=${TOOLCHAIN_ROOT}/sysroot

# add toolchain binaries to system path (precautionary)
export PATH=${PATH}:${TOOLCHAIN_BIN_PATH}
# -----


# ----- Setup Environment -----
# this block contains all the environment variables that may affect the configure script
# this block should essentially remain the same in all configuration scripts

# * = the env. variable influences libnl compilation (according to `./configure --help`)
# others are extra, couldn't hurt

# toolchain binaries
export CPP=${TOOLCHAIN_BIN_PATH}/${TARGET_HOST}-cpp 				# *, C preprocessor
export AR=${TOOLCHAIN_BIN_PATH}/${TARGET_HOST}-ar
export AS=${TOOLCHAIN_BIN_PATH}/${TARGET_HOST}-as
export NM=${TOOLCHAIN_BIN_PATH}/${TARGET_HOST}-nm
export CC=${TOOLCHAIN_BIN_PATH}/${TARGET_HOST}-gcc  				# *, C compiler command
export CXX=${TOOLCHAIN_BIN_PATH}/${TARGET_HOST}-g++
export LD=${TOOLCHAIN_BIN_PATH}/${TARGET_HOST}-ld
export RANLIB=${TOOLCHAIN_BIN_PATH}/${TARGET_HOST}-ranlib
export STRIP=${TOOLCHAIN_BIN_PATH}/${TARGET_HOST}-strip
export OBJDUMP=${TOOLCHAIN_BIN_PATH}/${TARGET_HOST}-objdump
export OBJCOPY=${TOOLCHAIN_BIN_PATH}/${TARGET_HOST}-objcopy
export ADDR2LINE=${TOOLCHAIN_BIN_PATH}/${TARGET_HOST}-addr2line
export READELF=${TOOLCHAIN_BIN_PATH}/${TARGET_HOST}-readelf
export SIZE=${TOOLCHAIN_BIN_PATH}/${TARGET_HOST}-size
export STRINGS=${TOOLCHAIN_BIN_PATH}/${TARGET_HOST}-strings
export ELFEDIT=${TOOLCHAIN_BIN_PATH}/${TARGET_HOST}-elfedit
export GCOV=${TOOLCHAIN_BIN_PATH}/${TARGET_HOST}-gcov
export GDB=${TOOLCHAIN_BIN_PATH}/${TARGET_HOST}-gdb
export GPROF=${TOOLCHAIN_BIN_PATH}/${TARGET_HOST}-gprof

# *, directories to add to pkg-config's search path
# Make sure that compiler doesn't mix up [.pc] files for `host` and `target`
export PKG_CONFIG_PATH=${TOOLCHAIN_ROOT}/lib/pkgconfig

# *, C compiler flags
export CFLAGS="${CFLAGS} --sysroot=${SYSROOT} -I${SYSROOT}/usr/include -I${TOOLCHAIN_ROOT}/include"
export CFLAGS="${CFLAGS} -fpic -fPIC -fpie -fPIE"
export CFLAGS="${CFLAGS} -march=x86-64 -msse4.2 -mpopcnt -m64 -mtune=intel"

# *, 
export LDFLAGS="${LDFLAGS} -L${SYSROOT}/usr/lib -L${TOOLCHAIN_ROOT}/lib"
export LDFLAGS="${LDFLAGS} -pie"

# *,
export LIBS="${LIBS}"

# *,
export CPPFLAGS="${CPPFLAGS} ${CFLAGS}"

# *,  path to pkg-config utility
export PKG_CONFIG="${PKG_CONFIG}"

# *, path overriding pkg-config's built-in search path
export PKG_CONFIG_LIBDIR="${PKG_CONFIG_LIBDIR}"

# *, C compiler flags for CHECK, overriding pkg-config
export CHECK_CFLAGS="${CHECK_CFLAGS}"

# *, linker flags for CHECK, overriding pkg-config
export CHECK_LIBS="${CHECK_LIBS}"
# -----


# ----- Other Configuration -----

# the installation directory
export PREFIX=${INSTALL_DIR}/${TARGET_ABI}
# -----


# clean
make clean

# run the configure script
#	... --enable-shared			[.so] files
#	... --enable-static			[.a] files
#	... --with-sysroot=			sysroot path
#	... --prefix=				install path
#	... --host=					target host
#	... --disable-cli			cli utilities not required for our purposes
#	... --disable-pthreads		arm doesn't support pthread
#	... --disable-debug			to reduce size in release build

./configure \
	--enable-shared \
	--enable-static \
	--with-sysroot=${SYSROOT} \
	--prefix=${PREFIX} \
	--host=${TARGET_HOST} \
	--disable-cli \
	# --disable-pthreads \
	# --disable-debug \
	# "$0"

# build
make
# install
make install