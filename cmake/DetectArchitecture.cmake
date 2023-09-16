include(CheckCXXSourceRuns)

function(check_instruction_set INSTRUCTION_SET_NAME INSTRUCTION_SET_FLAG INSTRUCTION_SET_INTRINSIC)

    set(INSTRUCTION_SET_CODE "
        #include <immintrin.h>
        #include <stdint.h>
        int main()
        {
            ${INSTRUCTION_SET_INTRINSIC};
            return 0;
        }
    ")

    set(CMAKE_REQUIRED_FLAGS "${INSTRUCTION_SET_FLAG}")
    CHECK_CXX_SOURCE_RUNS("${INSTRUCTION_SET_CODE}" "${INSTRUCTION_SET_NAME}")
    if(${INSTRUCTION_SET_NAME})
        set(AVX_TYPE "${INSTRUCTION_SET_NAME}" PARENT_SCOPE)
        set(AVX_FLAG "${INSTRUCTION_SET_FLAG}" PARENT_SCOPE)
        set(AVX_NAME "${INSTRUCTION_SET_NAME}" PARENT_SCOPE)
    else()
        return()
    endif()
endfunction()

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(INSTRUCTION_SETS
        "T_AVX?/arch:AVX?__m128i value{}#auto result = _mm_extract_epi32(value, 0)"
        "T_AVX2?/arch:AVX2?__m256i value{}#auto result = _mm256_extract_epi32(value, 0)"
        "T_AVX512?/arch:AVX512?int32_t result[16]#const _mm512i& value{}#_mm512_store_si512(result, value)"
    )
else()
    set(INSTRUCTION_SETS
        "T_AVX?-mavx?__m128i value{}#auto result = _mm_extract_epi32(value, 0)"
        "T_AVX2?-mavx2?__m256i value{}#auto result = _mm256_extract_epi32(value, 0)"
        "T_AVX512?-mavx512f?int32_t result[16]#const _mm512i& value{}#_mm512_store_si512(result, value)"
)
endif()

set(CMAKE_REQUIRED_FLAGS_SAVE "${CMAKE_REQUIRED_FLAGS}")

set(AVX_NAME "T_fallback")

# This is only supported on x86/x64, it is completely skipped and forced to T_fallback anywhere else
if ((${CMAKE_SYSTEM_PROCESSOR} MATCHES "x86_64") OR (${CMAKE_SYSTEM_PROCESSOR} MATCHES "i386") OR (${CMAKE_SYSTEM_PROCESSOR} MATCHES "AMD64"))

	foreach(INSTRUCTION_SET IN LISTS INSTRUCTION_SETS)
		string(REPLACE "?" ";" CURRENT_LIST "${INSTRUCTION_SET}")
		list(GET CURRENT_LIST 0 INSTRUCTION_SET_NAME)
		list(GET CURRENT_LIST 1 INSTRUCTION_SET_FLAG)
		string(REPLACE "." ";" INSTRUCTION_SET_FLAG "${INSTRUCTION_SET_FLAG}")
		list(GET CURRENT_LIST 2 INSTRUCTION_SET_INTRINSIC)
		string(REPLACE "#" ";" INSTRUCTION_SET_INTRINSIC "${INSTRUCTION_SET_INTRINSIC}")
		check_instruction_set("${INSTRUCTION_SET_NAME}" "${INSTRUCTION_SET_FLAG}" "${INSTRUCTION_SET_INTRINSIC}")
	endforeach()

	string(REPLACE "T_" "" AVX_DISPLAY ${AVX_NAME})
	message(STATUS "Detected ${CMAKE_SYSTEM_PROCESSOR} SSE type: ${AVX_DISPLAY}")
	set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS_SAVE}")
else()
	message(STATUS "SSE not supported by architecture ${CMAKE_SYSTEM_PROCESSOR} ${AVX_NAME}")
	set(AVX_NAME "T_fallback")
	set(AVX_TYPE "T_fallback")
endif()
