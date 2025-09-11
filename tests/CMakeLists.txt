cmake_minimum_required(VERSION 3.15)

project(
  topgg_tests
  LANGUAGES CXX
  HOMEPAGE_URL "https://docs.top.gg/docs"
  DESCRIPTION "Unit tests for testing the Top.gg C++ SDK."
)

set(TESTING ON)
set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type")

option(BUILD_SHARED_LIBS "Build shared libraries" ON)
option(TEST_API "Build unit tests for testing primary API support" ON)
option(TEST_BOT_AUTOPOSTER "Build unit tests for testing Discord bot autoposter support" OFF)
option(TEST_CPP_HTTPLIB_WEBHOOKS "Build unit tests for testing webhooks via cpp-httplib" OFF)
option(TEST_DROGON_WEBHOOKS "Build unit tests for testing webhooks via drogon" OFF)

if(NOT TEST_API AND NOT TEST_BOT_AUTOPOSTER)
set(ENABLE_API OFF)
endif()

if(TEST_CPP_HTTPLIB_WEBHOOKS)
set(ENABLE_CPP_HTTPLIB_WEBHOOKS ON)

if(TEST_DROGON_WEBHOOKS)
message(FATAL_ERROR "TEST_CPP_HTTPLIB_WEBHOOKS and TEST_DROGON_WEBHOOKS must NOT be ON at the same time.")
endif()
elseif(TEST_DROGON_WEBHOOKS)
set(ENABLE_DROGON_WEBHOOKS ON)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")
add_subdirectory(.. topgg)

function(add_test_file FILENAME DIRECTORY)
add_executable(${FILENAME} ${DIRECTORY}/${FILENAME}.cpp)

target_link_libraries(${FILENAME} PRIVATE topgg)

set_target_properties(${FILENAME} PROPERTIES
  CXX_STANDARD          20
  CXX_STANDARD_REQUIRED ON
)

if(MSVC)
target_compile_options(${FILENAME} PRIVATE /nologo $<$<CONFIG:Debug>:/diagnostics:caret /MDd /DDEBUG /D_DEBUG> $<$<CONFIG:Release>:/MD /O2 /Oi /Oy /Gy /DNDEBUG>)
else()
target_compile_options(${FILENAME} PRIVATE $<$<CONFIG:Release>:-O3> -Wall -Wextra -Wpedantic -Wformat=2 -Wnull-dereference -Wuninitialized -Wdeprecated)
endif()
endfunction()

if(TEST_API)
add_test_file(test_api .)
endif()

if(TEST_BOT_AUTOPOSTER)
add_test_file(test_bot_autoposter .)
endif()

if(ENABLE_CPP_HTTPLIB_WEBHOOKS)
file(GLOB WEBHOOKS cpp-httplib-webhooks/*.cpp)

foreach(WEBHOOK ${WEBHOOKS})
get_filename_component(NAME_WE ${WEBHOOK} NAME_WE)
add_test_file(${NAME_WE} cpp-httplib-webhooks)
endforeach()
elseif(ENABLE_DROGON_WEBHOOKS)
file(GLOB WEBHOOKS drogon-webhooks/*.cpp)

foreach(WEBHOOK ${WEBHOOKS})
get_filename_component(NAME_WE ${WEBHOOK} NAME_WE)
add_test_file(${NAME_WE} drogon-webhooks)
target_compile_definitions(${NAME_WE} PUBLIC __TOPGG_DROGON_WEBHOOKS__)
endforeach()
endif()