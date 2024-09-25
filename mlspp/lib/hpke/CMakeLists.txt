set(CURRENT_LIB_NAME hpke)

###
### Library Config
###

file(GLOB_RECURSE LIB_HEADERS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")
file(GLOB_RECURSE LIB_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

# -Werror=dangling-reference

add_library(${CURRENT_LIB_NAME} STATIC ${LIB_HEADERS} ${LIB_SOURCES})
add_dependencies(${CURRENT_LIB_NAME} bytes tls_syntax)
target_include_directories(${CURRENT_LIB_NAME}
  PRIVATE
    "${JSON_INCLUDE_INTERFACE}")

target_link_libraries(${CURRENT_LIB_NAME} 
  PUBLIC
    bytes tls_syntax
  PRIVATE
    OpenSSL::Crypto
)

target_include_directories(${CURRENT_LIB_NAME}
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include/${PROJECT_NAME}>
  PRIVATE
    ${OPENSSL_INCLUDE_DIR}
)

###
### Install
###

install(TARGETS ${CURRENT_LIB_NAME} EXPORT mlspp-targets)
install(
  DIRECTORY
        include
  DESTINATION
    ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}
)

target_compile_options(
        "${CURRENT_LIB_NAME}"
        PUBLIC
        "$<$<PLATFORM_ID:Windows>:/bigobj;/Zc:preprocessor>"
        PRIVATE
        "$<$<PLATFORM_ID:Windows>:$<$<CONFIG:Debug>:/sdl;/Od;/DEBUG;/MP;/DFD_SETSIZE=1024>>"
        "$<$<PLATFORM_ID:Windows>:$<$<CONFIG:Release>:/O2;/Oi;/Oy;/GL;/Gy;/sdl;/MP;/DFD_SETSIZE=1024>>"
        "$<$<PLATFORM_ID:Linux>:$<$<CONFIG:Debug>:-Wall;-Wno-dangling-reference;-Wempty-body;-Wno-psabi;-Wunknown-pragmas;-Wignored-qualifiers;-Wimplicit-fallthrough;-Wmissing-field-initializers;-Wsign-compare;-Wtype-limits;-Wuninitialized;-Wshift-negative-value;-pthread;-g;-Og;-fPIC>>"
        "$<$<PLATFORM_ID:Linux>:$<$<CONFIG:Release>:-Wall;-Wno-dangling-reference;-Wempty-body;-Wno-psabi;-Wunknown-pragmas;-Wignored-qualifiers;-Wimplicit-fallthrough;-Wmissing-field-initializers;-Wsign-compare;-Wtype-limits;-Wuninitialized;-Wshift-negative-value;-pthread;-O3;-fPIC>>"
        "${AVX_FLAG}"
)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../../lib/bytes/include")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../../lib/hpke/include")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../../lib/mls_vectors/include")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../../lib/tls_syntax/include")

