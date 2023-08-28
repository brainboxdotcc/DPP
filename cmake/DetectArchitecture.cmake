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
        "T_AVX?/arch:AVX?auto result = _mm_testz_ps(__m128{}, __m128{})"
        "T_AVX2?/arch:AVX2?auto result = _mm256_add_epi32(__m256i{}, __m256i{})"
        "T_AVX512?/arch:AVX512?auto result = _mm512_add_ps(__m512i{}, __m512i{}).auto result2 = _mm512_cmplt_epu8_mask(__m512i{}, __m512i{})"
    )
else()
    set(INSTRUCTION_SETS
        "T_AVX?-mavx.-mpclmul.-mbmi?auto result = _mm_testz_ps(__m128{}, __m128{})"
        "T_AVX2?-mavx2.-mavx.-mpclmul.-mbmi?auto result = _mm256_add_epi32(__m256i{}, __m256i{})"
        "T_AVX512?-mavx512bw.-mavx512f.-mavx2.-mavx.-mpclmul.-mbmi?auto result = _mm512_add_ps(__m512i{}, __m512i{}).auto result2 = _mm512_cmplt_epu8_mask(__m512i{}, __m512i{})"
    )
endif()

set(CMAKE_REQUIRED_FLAGS_SAVE "${CMAKE_REQUIRED_FLAGS}")

set(AVX_NAME "T_Fallback")
if ((${CMAKE_SYSTEM_PROCESSOR} MATCHES "x86_64") OR (${CMAKE_SYSTEM_PROCESSOR} MATCHES "i386"))

	foreach(INSTRUCTION_SET IN LISTS INSTRUCTION_SETS)
		string(REPLACE "?" ";" CURRENT_LIST "${INSTRUCTION_SET}")
		list(GET CURRENT_LIST 0 INSTRUCTION_SET_NAME)
		list(GET CURRENT_LIST 1 INSTRUCTION_SET_FLAG)
		string(REPLACE "." ";" INSTRUCTION_SET_FLAG "${INSTRUCTION_SET_FLAG}")
		list(GET CURRENT_LIST 2 INSTRUCTION_SET_INTRINSIC)
		string(REPLACE "." ";" INSTRUCTION_SET_INTRINSIC "${INSTRUCTION_SET_INTRINSIC}")
		check_instruction_set("${INSTRUCTION_SET_NAME}" "${INSTRUCTION_SET_FLAG}" "${INSTRUCTION_SET_INTRINSIC}")
	endforeach()

	string(REPLACE "T_" "" AVX_DISPLAY ${AVX_NAME})
	message(STATUS "Detected ${CMAKE_SYSTEM_PROCESSOR} SSE type: ${AVX_DISPLAY}")
	set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS_SAVE}")
else()
	message(STATUS "SSE not supported by architecture ${CMAKE_SYSTEM_PROCESSOR}")
endif()
