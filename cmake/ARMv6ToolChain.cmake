set(CMAKE_SYSTEM_NAME Linux)

# Possibly needed tweak
# set(CMAKE_SYSTEM_PROCESSOR armhf)

# If you have installed cross compiler to somewhere else, please specify that path.
set(COMPILER_ROOT /opt/cross-pi-gcc)

set(CMAKE_C_COMPILER ${COMPILER_ROOT}/bin/arm-linux-gnueabihf-gcc-8.3.0)
set(CMAKE_CXX_COMPILER ${COMPILER_ROOT}/bin/arm-linux-gnueabihf-g++)

# Below call is necessary to avoid non-RT problem.
set(CMAKE_LIBRARY_ARCHITECTURE arm-linux-gnueabihf)
set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE armhf)
set(CPACK_RPM_PACKAGE_ARCHITECTURE armhf)

set(RASPBERRY_ROOT_PATH ${CMAKE_CURRENT_LIST_DIR}/arm_raspberry)
set(RASPBERRY_KINETIC_PATH ${RASPBERRY_ROOT_PATH}/opt/ros/kinetic)

set(CMAKE_FIND_ROOT_PATH ${RASPBERRY_ROOT_PATH} ${CATKIN_DEVEL_PREFIX})

# Have to set this one to BOTH, to allow CMake to find rospack
# This set of variables controls whether the CMAKE_FIND_ROOT_PATH and CMAKE_SYSROOT are used for find_xxx() operations.
# set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM BOTH)
# set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
# set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
# set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
set(CMAKE_PREFIX_PATH ${RASPBERRY_KINETIC_PATH} ${RASPBERRY_ROOT_PATH}/usr)

unset(CMAKE_C_IMPLICIT_INCLUDE_DIRECTORIES)
unset(CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES)

set(CMAKE_INCLUDE_DIRECTORIES_BEFORE ON)
include_directories(
	${COMPILER_ROOT}/arm-linux-gnueabihf/libc/usr/include
	${COMPILER_ROOT}/arm-linux-gnueabihf/include
	${COMPILER_ROOT}/arm-linux-gnueabihf/include/c++/8.3.0
	${COMPILER_ROOT}/arm-linux-gnueabihf/include/c++/8.3.0/arm-linux-gnueabihf
	${COMPILER_ROOT}/lib/gcc/arm-linux-gnueabihf/8.3.0/include
	${COMPILER_ROOT}/lib/gcc/arm-linux-gnueabihf/8.3.0/include-fixed
	${CMAKE_SOURCE_DIR}/rootfs/usr/include/arm-linux-gnueabihf)
set(CMAKE_INCLUDE_DIRECTORIES_BEFORE OFF)

set(ZLIB_LIBRARY ${CMAKE_SOURCE_DIR}/rootfs/lib/arm-linux-gnueabihf/libz.so.1.2.11)
set(OPENSSL_CRYPTO_LIBRARY ${CMAKE_SOURCE_DIR}/rootfs/usr/lib/arm-linux-gnueabihf/libcrypto.so.1.1)
set(OPENSSL_SSL_LIBRARY ${CMAKE_SOURCE_DIR}/rootfs/usr/lib/arm-linux-gnueabihf/libssl.so.1.1)

set(CMAKE_CXX_COMPILER_WORKS 1)
set(CMAKE_C_FLAGS " ${CMAKE_C_FLAGS} -nostdinc --sysroot=${RASPBERRY_ROOT_PATH} -Wno-psabi " CACHE INTERNAL "" FORCE)
set(CMAKE_CXX_FLAGS " ${CMAKE_CXX_FLAGS} -nostdinc -nostdinc++ --sysroot=${RASPBERRY_ROOT_PATH} -Wno-psabi " CACHE INTERNAL "" FORCE)
set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} --sysroot=${RASPBERRY_ROOT_PATH} -ldl" CACHE INTERNAL "" FORCE)
set(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS} --sysroot=${RASPBERRY_ROOT_PATH} -ldl" CACHE INTERNAL "" FORCE)

set(LD_LIBRARY_PATH ${RASPBERRY_KINETIC_PATH}/lib)

execute_process(COMMAND wget -P ${CMAKE_SOURCE_DIR}/rootfs -q http://content.dpp.dev/zlib1g_1.2.11.dfsg-1_armhf.deb http://content.dpp.dev/zlib1g-dev_1.2.11.dfsg-1_armhf.deb http://content.dpp.dev/libssl1.1_1.1.1m-1_armhf.deb http://content.dpp.dev/libssl-dev_1.1.1m-1_armhf.deb https://content.dpp.dev/raspi-toolchain.tar.gz)

execute_process(
	COMMAND tar -xzf ${CMAKE_SOURCE_DIR}/rootfs/raspi-toolchain.tar.gz -C /opt
	COMMAND sudo dpkg-deb -x ${CMAKE_SOURCE_DIR}/rootfs/zlib1g-dev_1.2.11.dfsg-1_armhf.deb ${CMAKE_SOURCE_DIR}/rootfs
	COMMAND sudo dpkg-deb -x ${CMAKE_SOURCE_DIR}/rootfs/zlib1g_1.2.11.dfsg-1_armhf.deb ${CMAKE_SOURCE_DIR}/rootfs
	COMMAND sudo dpkg-deb -x ${CMAKE_SOURCE_DIR}/rootfs/libssl-dev_1.1.1m-1_armhf.deb ${CMAKE_SOURCE_DIR}/rootfs
	COMMAND sudo dpkg-deb -x ${CMAKE_SOURCE_DIR}/rootfs/libssl1.1_1.1.1m-1_armhf.deb ${CMAKE_SOURCE_DIR}/rootfs)
