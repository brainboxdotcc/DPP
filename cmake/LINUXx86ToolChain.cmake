SET(CMAKE_SYSTEM_NAME Linux)
# Possibly needed tweak
#SET(CMAKE_SYSTEM_PROCESSOR i386)

SET(CMAKE_C_COMPILER gcc-8)
SET(CMAKE_CXX_COMPILER g++-8)

# Below call is necessary to avoid non-RT problem.
SET(CMAKE_LIBRARY_ARCHITECTURE i386-linux-gnu)
SET(CPACK_DEBIAN_PACKAGE_ARCHITECTURE i386)
SET(CPACK_RPM_PACKAGE_ARCHITECTURE i686)

#If you have installed cross compiler to somewhere else, please specify that path.
SET(COMPILER_ROOT /usr/bin) 

INCLUDE_DIRECTORIES(
	/usr/include/i386-linux-gnu)

SET(ZLIB_LIBRARY /lib/i386-linux-gnu/libz.so.1.2.11)
SET(OPENSSL_CRYPTO_LIBRARY /usr/lib/i386-linux-gnu/libcrypto.so.1.1)
SET(OPENSSL_SSL_LIBRARY /usr/lib/i386-linux-gnu/libssl.so.1.1)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32 " CACHE INTERNAL "" FORCE)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32 " CACHE INTERNAL "" FORCE)

EXECUTE_PROCESS(COMMAND sudo dpkg --add-architecture i386)
EXECUTE_PROCESS(COMMAND sudo apt update)
EXECUTE_PROCESS(COMMAND sudo apt install -y g++-8 gcc-8-multilib glibc-*:i386 libc6-dev-i386 g++-8-multilib zlib1g-dev:i386 libssl-dev:i386 libopus-dev:i386 libsodium-dev:i386)
EXECUTE_PROCESS(COMMAND sudo mv /usr/lib/i386-linux-gnu/pkgconfig/libsodium.pc /usr/lib/pkgconfig/)

