cmake_minimum_required(VERSION 3.16)
add_compile_options("$<$<AND:$<C_COMPILER_ID:MSVC>,$<COMPILE_LANGUAGE:C>>:/utf-8>")
add_compile_options("$<$<AND:$<CXX_COMPILER_ID:MSVC>,$<COMPILE_LANGUAGE:CXX>>:/utf-8>")
add_compile_options("$<$<AND:$<CXX_COMPILER_ID:MSVC>,$<COMPILE_LANGUAGE:CXX>>:/Zc:__cplusplus>")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_VERBOSE_MAKEFILE ON)

set(PROJECT_NAME krkrblend2d)
set(PROJECT_VERSION 1.0)

project(${PROJECT_NAME} VERSION ${PROJECT_VERSION})

add_subdirectory(../ncbind ${CMAKE_CURRENT_BINARY_DIR}/ncbind)

set(BLEND2D_STATIC TRUE)
add_subdirectory(blend2d ${CMAKE_CURRENT_BINARY_DIR}/blend2d)
#include("${BLEND2D_DIR}/CMakeLists.txt")

# library target.
add_library(${PROJECT_NAME} SHARED
  src/Blend2D.cpp
  src/main.cpp
  src/nuklear.cpp
)

set_property(TARGET ${PROJECT_NAME} ncbind tp_stub blend2d PROPERTY
  MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

target_include_directories(${PROJECT_NAME}
PUBLIC
  ../nuklear/src
)  

target_link_libraries(${PROJECT_NAME} 
  ncbind
  blend2d
)
