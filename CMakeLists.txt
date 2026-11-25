cmake_minimum_required(VERSION 3.10)

project(
  topgg
  LANGUAGES CXX
  HOMEPAGE_URL "https://docs.top.gg/docs"
  DESCRIPTION "The community-maintained C++17 SDK for Top.gg."
)

get_property(_has_shared GLOBAL PROPERTY TARGET_SUPPORTS_SHARED_LIBS)

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type")
option(BUILD_SHARED_LIBS "Build shared libraries" ${_has_shared})

file(GLOB TOPGG_SOURCE_FILES ${CMAKE_CURRENT_LIST_DIR}/src/*.cpp)

if(BUILD_SHARED_LIBS)
add_library(topgg SHARED ${TOPGG_SOURCE_FILES})

if(WIN32)
target_sources(topgg PRIVATE ${CMAKE_CURRENT_LIST_DIR}/topgg.rc)
endif()
else()
add_library(topgg STATIC ${TOPGG_SOURCE_FILES})
endif()

set_target_properties(topgg PROPERTIES
  OUTPUT_NAME           topgg
  CXX_STANDARD          17
  CXX_STANDARD_REQUIRED ON
)

if(MSVC)
target_compile_options(topgg PRIVATE /EHsc /permissive- $<$<CONFIG:Debug>:/diagnostics:caret /MDd> $<$<CONFIG:Release>:/MD /O2 /Oi /Oy /Gy>)
else()
target_compile_options(topgg PRIVATE $<$<CONFIG:Release>:-O3> -Wall -Wextra -Wpedantic -Wformat=2 -Wnull-dereference -Wuninitialized -Wdeprecated)
endif()

if(MSVC)
target_compile_definitions(topgg PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
target_compile_definitions(topgg PRIVATE _DEBUG)
endif()

find_library(OPENSSL_LIBRARY REQUIRED NAMES ssl)
find_library(CRYPTO_LIBRARY REQUIRED NAMES crypto)
find_path(OPENSSL_INCLUDE_DIR NAMES openssl/ssl.h HINTS include)
find_package(CURL REQUIRED)

target_include_directories(topgg PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include ${OPENSSL_INCLUDE_DIR})
target_link_libraries(topgg PRIVATE CURL::libcurl ${OPENSSL_LIBRARY} ${CRYPTO_LIBRARY})