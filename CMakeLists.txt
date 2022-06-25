cmake_minimum_required(VERSION 3.16)

# UTF-8 source
add_compile_options("$<$<AND:$<C_COMPILER_ID:MSVC>,$<COMPILE_LANGUAGE:C>>:/utf-8>")
add_compile_options("$<$<AND:$<CXX_COMPILER_ID:MSVC>,$<COMPILE_LANGUAGE:CXX>>:/utf-8>")
add_compile_options("$<$<AND:$<CXX_COMPILER_ID:MSVC>,$<COMPILE_LANGUAGE:CXX>>:/Zc:__cplusplus>")

set(PROJECT_NAME LayerExBlend2D)
set(PROJECT_VERSION 1.0)

project(${PROJECT_NAME} VERSION ${PROJECT_VERSION})

set(CMAKE_CXX_STANDARD 11)

set(APP_DIR "${CMAKE_CURRENT_LIST_DIR}"
    CACHE PATH "Application directory")

set(BLEND2D_DIR "${APP_DIR}/blend2d"
    CACHE PATH "Location of 'blend2d'")

set(BLEND2D_STATIC TRUE)
include("${BLEND2D_DIR}/CMakeLists.txt")

set(TPSTUB_DIR "${APP_DIR}/../tp_stubz_utf8"
    CACHE PATH "Location of tp_stub")

set(NCBIND_DIR "${APP_DIR}/../ncbind"
    CACHE PATH "Location of ncbind")


# library target.
add_library(${PROJECT_NAME} SHARED
  ${TPSTUB_DIR}/tp_stub.cpp
  ${NCBIND_DIR}/ncbind.cpp
  Blend2D.cpp
  main.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE
  ${TPSTUB_DIR}
  ${NCBIND_DIR}
)

# Add Blend2D dependency to your application. The dependency
# should provide everything needed - include paths, libraries,
# compiler flags, and compiler definitions.
target_link_libraries(${PROJECT_NAME} 
  Blend2D::Blend2D
)
