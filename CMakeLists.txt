cmake_minimum_required(VERSION 2.8.8)

set(PROJECT_NAME_STR QR-Database)
project(${PROJECT_NAME_STR} C CXX)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(FATAL_ERROR "should use gcc (not clang)")
endif()

set(DEFINE "-DHAVE_SSE2 -DSFMT_MEXP=19937")

if(CMAKE_COMPILER_IS_GNUCXX)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -ansi -Wno-deprecated -std=c++11 -fopenmp -O3 -msse2 ${DEFINE}")
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11 -O3 -msse2 ${DEFINE}")
endif()

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /openmp /arch:SSE2 ${DEFINE}")
endif()

set(SFMT_DIR ${PROJECT_SOURCE_DIR}/SFMT)
include_directories(${SFMT_DIR})
add_library(sfmt STATIC
    ${SFMT_DIR}/SFMT.c
)

add_executable(${PROJECT_NAME_STR}
    ${PROJECT_SOURCE_DIR}/qr.cpp
)

target_link_libraries(${PROJECT_NAME_STR} sfmt)
