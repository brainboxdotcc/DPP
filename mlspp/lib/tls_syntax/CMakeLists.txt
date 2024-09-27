set(CURRENT_LIB_NAME tls_syntax)

###
### Library Config
###

file(GLOB_RECURSE LIB_HEADERS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")
file(GLOB_RECURSE LIB_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

add_library(${CURRENT_LIB_NAME} STATIC ${LIB_HEADERS} ${LIB_SOURCES})
target_include_directories(${CURRENT_LIB_NAME}
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include/${PROJECT_NAME}>
)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../../lib/bytes/include")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../../lib/hpke/include")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../../lib/mls_vectors/include")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../../lib/tls_syntax/include")
