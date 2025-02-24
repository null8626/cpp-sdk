cmake_minimum_required(VERSION 3.10)

project(
  topgg
  LANGUAGES CXX
  HOMEPAGE_URL "https://docs.top.gg/docs"
  DESCRIPTION "The official C++ wrapper for the Top.gg API."
)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type")
option(BUILD_SHARED_LIBS "Build shared libraries" ON)
option(ENABLE_CORO "Support for C++20 coroutines" OFF)
option(TESTING "Enable this only if you are testing the library" OFF)

file(GLOB TOPGG_SOURCE_FILES src/*.cpp)

if(BUILD_SHARED_LIBS)
add_library(topgg SHARED ${TOPGG_SOURCE_FILES})

if(WIN32)
target_sources(topgg PRIVATE ${CMAKE_SOURCE_DIR}/topgg.rc)
endif()
else()
add_library(topgg STATIC ${TOPGG_SOURCE_FILES})
endif()

if(WIN32)
target_compile_definitions(topgg PRIVATE $<IF:$<BOOL:${BUILD_SHARED_LIBS}>,__TOPGG_BUILDING_DLL__,DPP_STATIC TOPGG_STATIC>)
endif()

if(ENABLE_CORO)
set(TOPGG_CXX_STANDARD 20)
target_compile_definitions(topgg PUBLIC DPP_CORO=ON)
else()
set(TOPGG_CXX_STANDARD 17)
endif()

if(TESTING)
target_compile_definitions(topgg PRIVATE __TOPGG_TESTING__)
endif()

set_target_properties(topgg PROPERTIES
  OUTPUT_NAME           topgg
  CXX_STANDARD          ${TOPGG_CXX_STANDARD}
  CXX_STANDARD_REQUIRED ON
)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)

find_package(DPP REQUIRED)

if(MSVC)
target_compile_options(topgg PUBLIC $<$<CONFIG:Debug>:/diagnostics:caret /MTd /DDEBUG /D_DEBUG> $<$<CONFIG:Release>:/MT /O2 /Oi /Oy /Gy /DNDEBUG>)
else()
target_compile_options(topgg PUBLIC $<$<CONFIG:Release>:-O3> -Wall -Wextra -Wpedantic -Wformat=2 -Wnull-dereference -Wuninitialized -Wdeprecated)
endif()

target_include_directories(topgg PUBLIC
  ${CMAKE_SOURCE_DIR}/include
  ${DPP_INCLUDE_DIR}
)

target_link_libraries(topgg ${DPP_LIBRARIES})