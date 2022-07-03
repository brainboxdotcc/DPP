set(CMAKE_SYSTEM_NAME Linux)

# Possibly needed tweak
# set(CMAKE_SYSTEM_PROCESSOR i386)
set(CMAKE_C_COMPILER gcc-8)
set(CMAKE_CXX_COMPILER g++-8)

# Below call is necessary to avoid non-RT problem.
set(CMAKE_LIBRARY_ARCHITECTURE i386-linux-gnu)
set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE i386)
set(CPACK_RPM_PACKAGE_ARCHITECTURE i686)

# If you have installed cross compiler to somewhere else, please specify that path.
set(COMPILER_ROOT /usr/bin)

include_directories(
	/usr/include/i386-linux-gnu
)

set(ZLIB_LIBRARY /lib/i386-linux-gnu/libz.so.1.2.11)
set(OPENSSL_CRYPTO_LIBRARY /usr/lib/i386-linux-gnu/libcrypto.so.1.1)
set(OPENSSL_SSL_LIBRARY /usr/lib/i386-linux-gnu/libssl.so.1.1)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32 " CACHE INTERNAL "" FORCE)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32 " CACHE INTERNAL "" FORCE)

execute_process(COMMAND sudo dpkg --add-architecture i386)
execute_process(COMMAND sudo apt update)
execute_process(COMMAND sudo apt install -y g++-8 gcc-8-multilib glibc-*:i386 libc6-dev-i386 g++-8-multilib zlib1g-dev:i386 libssl-dev:i386 libopus-dev:i386 libsodium-dev:i386)
execute_process(COMMAND sudo mv /usr/lib/i386-linux-gnu/pkgconfig/libsodium.pc /usr/lib/pkgconfig/)
