cmake_minimum_required (VERSION 3.6)
project(documentation_tests)

string(ASCII 27 Esc)

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -pthread -O0 -fPIC -rdynamic -DFMT_HEADER_ONLY -Wall -Werror")
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0")

file(GLOB example_list ./*.cpp)
foreach (example ${example_list})
	get_filename_component(examplename ${example} NAME)
	message(STATUS "Found example '${Esc}[1;34m${examplename}${Esc}[m'")
	add_executable(${examplename}_out ${example})
	target_link_libraries(${examplename}_out dl dpp)
endforeach(example)

