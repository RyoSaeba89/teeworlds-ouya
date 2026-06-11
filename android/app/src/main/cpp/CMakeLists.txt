cmake_minimum_required(VERSION 3.4.1)
project(teeworlds)

# --------------------------------------------------------------------------
# Teeworlds 0.7.5 -> OUYA (Tegra 3, GLES2 via gl4es, API 16, armeabi-v7a)
# Builds the Teeworlds client engine + game + bundled externals into libmain.so
# (SDL loads "SDL_main" from this lib). gl4es translates the fixed-function GL.
# --------------------------------------------------------------------------

add_definitions(-D__ANDROID__)

# Path to the Teeworlds source root (this file lives in android/app/src/main/cpp)
get_filename_component(TW "${CMAKE_CURRENT_SOURCE_DIR}/../../../../.." ABSOLUTE)
set(TWSRC ${TW}/src)
set(EXT  ${TWSRC}/engine/external)

# char is unsigned on ARM; Teeworlds (like Cube) assumes signed char for net parsing.
set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -fsigned-char -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fsigned-char -Wall")

# ------------------------- prebuilt deps ----------------------------------
set(LIBDIR ${CMAKE_CURRENT_SOURCE_DIR}/lib)

add_library(sdl SHARED IMPORTED)
set_target_properties(sdl PROPERTIES IMPORTED_LOCATION ${LIBDIR}/SDL2/${ANDROID_ABI}/libSDL2.so)

add_library(hidapi SHARED IMPORTED)
set_target_properties(hidapi PROPERTIES IMPORTED_LOCATION ${LIBDIR}/SDL2/${ANDROID_ABI}/libhidapi.so)

add_library(gl4es STATIC IMPORTED)
set_target_properties(gl4es PROPERTIES IMPORTED_LOCATION ${LIBDIR}/gl4es/${ANDROID_ABI}/libGL.a)

add_library(freetype STATIC IMPORTED)
set_target_properties(freetype PROPERTIES IMPORTED_LOCATION ${LIBDIR}/freetype/${ANDROID_ABI}/libfreetype.a)

# ------------------------- include paths ----------------------------------
include_directories(
  ${TWSRC}                       # base/ engine/ game/ generated/
  ${EXT}/zlib
  ${EXT}/pnglite
  ${EXT}/wavpack
  ${EXT}/md5
  ${EXT}/json-parser
  ${CMAKE_CURRENT_SOURCE_DIR}/include/SDL2
  ${CMAKE_CURRENT_SOURCE_DIR}/include/freetype
  ${CMAKE_CURRENT_SOURCE_DIR}/include/gl4es
)

# ------------------------- source globs -----------------------------------
# bundled externals
file(GLOB SRC_ZLIB    ${EXT}/zlib/*.c)
file(GLOB SRC_PNGLITE ${EXT}/pnglite/*.c)
file(GLOB SRC_WAVPACK ${EXT}/wavpack/*.c)   # .c only (skip arm.S asm)
file(GLOB SRC_JSON    ${EXT}/json-parser/*.c)
file(GLOB SRC_MD5     ${EXT}/md5/*.c)

# base (only the bundled-crypto path; skip openssl/libtomcrypt variants)
set(SRC_BASE
  ${TWSRC}/base/system.c
  ${TWSRC}/base/hash.c
  ${TWSRC}/base/hash_bundled.c
)

# engine shared + client
file(GLOB SRC_ENGINE_SHARED ${TWSRC}/engine/shared/*.cpp)
file(GLOB SRC_ENGINE_CLIENT ${TWSRC}/engine/client/*.cpp)

# game shared + client + editor
file(GLOB SRC_GAME_SHARED ${TWSRC}/game/*.cpp)
file(GLOB_RECURSE SRC_GAME_CLIENT ${TWSRC}/game/client/*.cpp)
file(GLOB SRC_GAME_EDITOR ${TWSRC}/game/editor/*.cpp)

# generated (pre-generated once with datasrc/compile.py + cmd5.py + git_revision.py)
set(SRC_GENERATED
  ${TWSRC}/generated/protocol.cpp
  ${TWSRC}/generated/client_data.cpp
  ${TWSRC}/generated/git_revision.cpp
)

# android glue (gl4es env constructor, sha256 shim, gamepad, data path)
file(GLOB SRC_ANDROID ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/*.c)

add_library(main SHARED
  ${SRC_BASE}
  ${SRC_ENGINE_SHARED}
  ${SRC_ENGINE_CLIENT}
  ${SRC_GAME_SHARED}
  ${SRC_GAME_CLIENT}
  ${SRC_GAME_EDITOR}
  ${SRC_GENERATED}
  ${SRC_ZLIB}
  ${SRC_PNGLITE}
  ${SRC_WAVPACK}
  ${SRC_JSON}
  ${SRC_MD5}
  ${SRC_ANDROID}
)

find_library(log-lib log)
find_library(android-lib android)

target_link_libraries(main
  gl4es
  freetype
  sdl
  hidapi
  EGL
  ${log-lib}
  ${android-lib}
)
