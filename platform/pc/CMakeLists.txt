# Vandal Hearts — native PC port build (Stage 2.4 cross-platform).
#
# This replaces platform/pc/Makefile for the PC-port build ONLY. The matching-decomp build
# (top-level Makefile, `make check`, the mipsel PSX toolchain) is entirely separate and untouched.
#
# Faithful translation of the Makefile: same per-file flag groups, the same symlink staging tree
# (so quoted `#include "PsyQ/x.h"` resolves to our clean-room headers, not the real Sony ones), and
# the same mid-build data-segment generator (compile objects -> link-probe for undefined symbols ->
# generate generated_data.c -> compile -> final link). See exchange/58 and the Makefile's own
# comments for why each piece exists.
#
# Linux is validated first; Windows (MinGW-w64) and macOS (Apple Silicon) toolchain files build on
# this. Windows note: Win64 is LLP64 (`long` is 32-bit) vs Linux LP64 (`long` is 64-bit) — harmless
# here because Stage 2.3 already mapped PSX `long`->`int` throughout the PC layer, so no struct
# depends on `long` being pointer-sized.

cmake_minimum_required(VERSION 3.16)
project(vandalhearts_pc C)

set(CMAKE_C_STANDARD 99)              # backend files are modern C; game files force -std=gnu89 per-source
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)       # -g, matching the Makefile's default
endif()

# Sanitizer flags (Stage 2.3/2.4 memory-debug). Empty by default; e.g.
#   cmake -B build_ubsan -DVH_SANITIZE="-fsanitize=bounds -fno-omit-frame-pointer"
# Applied to every compile, the generated_data.c compile, the link, AND the generator's probe link
# (via VH_SAN) so its instrumented objects link. This is the CMake equivalent of the Makefile's
# `make ubsan`. (32-bit ASan -- `make asan32` -- stays in the Makefile: it needs a 32-bit toolchain
# and the ASan sweep is already complete; UBSan at 64-bit is the remaining planned sanitizer run.)
set(VH_SANITIZE "" CACHE STRING "Sanitizer flags, e.g. -fsanitize=bounds -fno-omit-frame-pointer")
separate_arguments(VH_SAN_LIST NATIVE_COMMAND "${VH_SANITIZE}")

set(PC_DIR       ${CMAKE_CURRENT_SOURCE_DIR})
set(PROJECT_ROOT ${PC_DIR}/../..)
set(SRC_DIR      ${PROJECT_ROOT}/src)
set(GEN_DIR      ${CMAKE_BINARY_DIR}/datagen)     # generator's VH_BUILD_DIR (staging + work + output)
set(STAGE_DIR    ${GEN_DIR}/include_stage)
set(GEN_ASSET_DIR ${GEN_DIR}/generated_assets)
set(GEN_DATA_DIR ${GEN_DIR}/generated_sources)

set(VH_PSX_EXE "${PROJECT_ROOT}/SLUS_004.47" CACHE FILEPATH "Original US PS-X EXE used for generated data")
set(VH_KROM_SOURCE "${PROJECT_ROOT}/../psy-q/3.6/PSYQ/BIN/KROMDAT.BIN" CACHE FILEPATH
    "PsyQ KROMDAT.BIN or a PS1 BIOS containing the same font data")

# Windows: force-include the MinGW compat shim (BSD u_char/u_short/... the PsyQ headers use) ahead of
# every TU, at directory scope so it precedes the per-source -std/-include options and reaches the
# backend files too. Linux/macOS never include it. See include/pc_win_compat.h.
if(WIN32)
    add_compile_options(-include ${PC_DIR}/include/pc_win_compat.h)
endif()

# ---- staging symlink tree (== the Makefile `stage:` target) -----------------------------------
# Shadows the real PsyQ headers with ours. Rebuilt at configure time; reconfigure if a project
# header is added/removed under ../../include.
file(MAKE_DIRECTORY ${STAGE_DIR})
file(GLOB _stage_hdrs "${PROJECT_ROOT}/include/*.h" "${PROJECT_ROOT}/include/*.inc")
foreach(_h ${_stage_hdrs})
    get_filename_component(_hn ${_h} NAME)
    file(CREATE_LINK ${_h} ${STAGE_DIR}/${_hn} SYMBOLIC)
endforeach()
file(CREATE_LINK ${PC_DIR}/include/PsyQ        ${STAGE_DIR}/PsyQ        SYMBOLIC)
file(CREATE_LINK ${PC_DIR}/include/types.h     ${STAGE_DIR}/types.h     SYMBOLIC)
file(CREATE_LINK ${PC_DIR}/include/inline_gte.h ${STAGE_DIR}/inline_gte.h SYMBOLIC)

# ---- external libs ----------------------------------------------------------------------------
find_package(SDL2 REQUIRED)
if(NOT APPLE)
    find_package(OpenGL REQUIRED)
endif()
find_package(Python3 REQUIRED COMPONENTS Interpreter)   # data-file + data-segment generators
# libgpu.c's threaded hi-res rasterizer (P1) uses pthreads. Threads::Threads resolves to -lpthread on
# Linux, winpthreads on MinGW (bundled as libwinpthread-1.dll), nothing on macOS -- so the link is
# correct on every target instead of relying on glibc>=2.34 folding libpthread into libc.
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
find_library(OPENAL_LIB NAMES openal OpenAL32 REQUIRED)   # 'openal' on Linux, 'OpenAL32' on MinGW/Windows
if(NOT APPLE)
    find_path(OPENAL_INCLUDE_DIR AL/al.h REQUIRED)
else()
    # The SDK framework exposes headers as <OpenAL/al.h>; no Homebrew include path is needed.
    set(OPENAL_INCLUDE_DIR "")
endif()

# macOS responsibility gate: every offset-based data extractor must consume the byte-exact US
# executable. The stamp depends on the input itself, so replacing a file at the same path after
# configuration re-runs validation before any generated source or include is rebuilt.
set(PSX_VALIDATION_DEP ${VH_PSX_EXE})
if(APPLE)
    set(PSX_VALIDATION_STAMP ${GEN_DIR}/validated-slus-00447.stamp)
    add_custom_command(
        OUTPUT ${PSX_VALIDATION_STAMP}
        COMMAND ${Python3_EXECUTABLE} ${PC_DIR}/tools/validate_psx_exe.py ${VH_PSX_EXE}
        COMMAND ${CMAKE_COMMAND} -E touch ${PSX_VALIDATION_STAMP}
        DEPENDS ${VH_PSX_EXE} ${PC_DIR}/tools/validate_psx_exe.py
        COMMENT "Validating byte-exact SLUS_004.47 input"
        VERBATIM)
    add_custom_target(vh_validate_psx DEPENDS ${PSX_VALIDATION_STAMP})
    set(PSX_VALIDATION_DEP ${PSX_VALIDATION_STAMP})
endif()
if(APPLE)
    # The data generator links outside CMake targets and needs a concrete SDL library path.
    find_library(SDL2_GEN_LIB NAMES SDL2 SDL2-2.0 REQUIRED)
endif()

# libwebp: STANDARD HD-pack dependency (1.6). .webp replacements are ~7x smaller than raw .hdi (a full
# background pack is ~20MB vs ~150MB), so it ships on by default. -DVH_WEBP=OFF builds without it (the
# engine's `#ifdef VH_HD_WEBP` falls back to loading only raw .hdi packs). Mirrors the Makefile's NO_WEBP.
option(VH_WEBP "libwebp HD-pack support (.webp replacements)" ON)
if(VH_WEBP)
    find_library(WEBP_LIB NAMES webp REQUIRED)
    find_path(WEBP_INCLUDE_DIR webp/decode.h)
endif()

# libav (ffmpeg): STANDARD HD-pack dependency (1.6 HD FMVs). Decodes the H.264 .mp4 movies that replace
# the STR FMVs. On by default; -DVH_HDVIDEO=OFF keeps the native MDEC movie path (engine `#ifdef
# VH_HD_VIDEO`). Mirrors the Makefile's NO_HDVIDEO.
option(VH_HDVIDEO "libav HD FMV support (.mp4 replacements)" ON)
if(VH_HDVIDEO)
    find_library(AVFORMAT_LIB NAMES avformat REQUIRED)
    find_library(AVCODEC_LIB  NAMES avcodec  REQUIRED)
    find_library(AVUTIL_LIB   NAMES avutil   REQUIRED)
    find_library(SWSCALE_LIB  NAMES swscale  REQUIRED)
    find_path(AVCODEC_INCLUDE_DIR libavcodec/avcodec.h)
endif()

# ---- source lists -----------------------------------------------------------------------------
file(GLOB GAME_SRCS CONFIGURE_DEPENDS ${SRC_DIR}/*.c)
file(MAKE_DIRECTORY ${GEN_ASSET_DIR}/assets)
file(MAKE_DIRECTORY ${GEN_DATA_DIR})
set(PSX_INCLUDE_OUTPUTS
    ${GEN_ASSET_DIR}/assets/801009bc.inc
    ${GEN_ASSET_DIR}/assets/80100e9c.inc
    ${GEN_ASSET_DIR}/assets/801012e4.inc)
add_custom_command(
    OUTPUT ${PSX_INCLUDE_OUTPUTS}
    COMMAND ${Python3_EXECUTABLE} ${PC_DIR}/tools/gen_psx_includes.py
            ${VH_PSX_EXE} ${GEN_ASSET_DIR}/assets
    DEPENDS ${PC_DIR}/tools/gen_psx_includes.py ${PSX_VALIDATION_DEP}
    COMMENT "Generating build-local PS-X include assets"
    VERBATIM)
add_custom_target(vh_generated_assets DEPENDS ${PSX_INCLUDE_OUTPUTS})
if(APPLE)
    execute_process(
        COMMAND ${Python3_EXECUTABLE} ${PC_DIR}/tools/gen_apple_forward_decls.py
                ${SRC_DIR} ${STAGE_DIR}/apple_void_forward_decls.h
        COMMAND_ERROR_IS_FATAL ANY)
endif()

# Backend + hand-written data files, grouped by their Makefile compile profile.
set(BACKEND_PLAIN   libcd libspu libkernel libgpu pc_raster pc_gpu_trace pc_hdpack libsn pc_bootstrap pc_xa pc_spu pc_mdec pc_kanji_font pc_overlay pc_saves pc_hdvideo)
set(DATA_PERM       pc_battle_data pc_unit_anim_data pc_sprite_box_quads pc_event_anim_data pc_evt_entities)
set(DATA_PLAIN      pc_spell_descriptions pc_item_descriptions pc_string_table)

# Warning policy (mirrors the Makefile's GAME_WARN/PORT_WARN -- see the comment there): the decompiled
# game source gets its known-benign 1996-C warning classes silenced; PORT code compiles -Wall -Wextra
# and must stay warning-clean, so a new warning there is signal.
set(GAME_WARN "-Wno-incompatible-pointer-types;-Wno-builtin-declaration-mismatch;-Wno-discarded-qualifiers;-Wno-pointer-to-int-cast;-Wno-int-to-pointer-cast;-Wno-int-conversion")
if(APPLE)
    set(GAME_WARN "-Wno-incompatible-pointer-types;-Wno-ignored-qualifiers;-Wno-pointer-to-int-cast;-Wno-int-to-pointer-cast;-Wno-int-conversion;-Wno-error=implicit-function-declaration")
endif()
set(PORT_WARN "-Wall;-Wextra;-Wno-unused-parameter;-Wno-missing-field-initializers")
if(APPLE)
    # Treat warnings in port-owned translation units as errors on the Mac responsibility path. OpenAL
    # framework deprecation is a known SDK-level warning until that backend is replaced, not a source defect.
    list(APPEND PORT_WARN "-Werror" "-Wno-deprecated-declarations" "-Wno-macro-redefined"
         "-Wno-incompatible-library-redeclaration")
endif()

# ---- GENERATED game/BIOS-derived data files (gitignored; regenerated at build) ----------------
# Reconstruct copyrighted content (Konami UI text, PS1 BIOS kanji glyphs) from the user's OWN copy
# (byte-exact build/SLUS_004.47 + PsyQ KROMDAT.BIN, both already required), so the source tree ships
# no copyrighted data. Mirrors the Makefile's gen rules. See exchange/60.
set(GENERATED_DATA_C pc_kanji_font pc_spell_descriptions pc_item_descriptions pc_evt_entities pc_string_table)
foreach(_g ${GENERATED_DATA_C})
    string(REPLACE "pc_" "" _base "${_g}")   # gen_<base>.py, e.g. pc_string_table -> gen_string_table.py
    set(_generated_source ${GEN_DATA_DIR}/${_g}.c)
    add_custom_command(
        OUTPUT ${_generated_source}
        COMMAND ${CMAKE_COMMAND} -E env "VH_PSX_EXE=${VH_PSX_EXE}"
                "VH_KROM_SOURCE=${VH_KROM_SOURCE}"
                "VH_GENERATED_OUT=${_generated_source}"
                ${Python3_EXECUTABLE} ${PC_DIR}/tools/gen_${_base}.py
        DEPENDS ${PC_DIR}/tools/gen_${_base}.py ${PSX_VALIDATION_DEP} ${VH_KROM_SOURCE}
        WORKING_DIRECTORY ${PC_DIR}
        COMMENT "Generating build-local ${_g}.c (from the byte-exact binary / PsyQ KROMDAT)"
        VERBATIM)
    set_source_files_properties(${_generated_source} PROPERTIES GENERATED TRUE)
endforeach()

function(vh_source_path output stem)
    list(FIND GENERATED_DATA_C "${stem}" _generated_idx)
    if(_generated_idx GREATER -1)
        set(${output} "${GEN_DATA_DIR}/${stem}.c" PARENT_SCOPE)
    else()
        set(${output} "${PC_DIR}/src/${stem}.c" PARENT_SCOPE)
    endif()
endfunction()

set(ALL_OBJ_SRCS ${GAME_SRCS})
foreach(_b ${BACKEND_PLAIN} libetc pc_diag pc_battle_speed libsnd libgte pc_gpu_window pc_balance ${DATA_PERM} ${DATA_PLAIN})
    vh_source_path(_source "${_b}")
    list(APPEND ALL_OBJ_SRCS ${_source})
endforeach()

# ---- OBJECT library: everything except the generated data segment -----------------------------
# An OBJECT library so the generator can be handed the real object paths ($<TARGET_OBJECTS:...>);
# CMake's object layout does not match the Makefile glob the generator used to assume.
add_library(vh_objs OBJECT ${ALL_OBJ_SRCS})
add_dependencies(vh_objs vh_generated_assets)

# Common include dirs for our own backend/data files.
target_include_directories(vh_objs PRIVATE ${PC_DIR}/include ${STAGE_DIR}
                           ${SDL2_INCLUDE_DIRS} ${OPENAL_INCLUDE_DIR})
if(VH_WEBP)   # pc_hdpack.c's HD-pack loader (#ifdef VH_HD_WEBP) uses <webp/decode.h>
    target_compile_definitions(vh_objs PRIVATE VH_HD_WEBP)
    target_include_directories(vh_objs PRIVATE ${WEBP_INCLUDE_DIR})
endif()
if(VH_HDVIDEO)   # pc_hdvideo.c's HD FMV decoder (#ifdef VH_HD_VIDEO) uses <libav*/…>
    target_compile_definitions(vh_objs PRIVATE VH_HD_VIDEO)
    target_include_directories(vh_objs PRIVATE ${AVCODEC_INCLUDE_DIR})
endif()
# Sanitizer flags apply to every object (additive with the per-source options below).
if(VH_SAN_LIST)
    target_compile_options(vh_objs PRIVATE ${VH_SAN_LIST})
endif()

# -- game sources (../../src/*.c): the GAME_C_FLAGS profile --------------------------------------
# -D defines go through COMPILE_DEFINITIONS (CMake quotes the function-like `asm(x)=` correctly;
# passing it as a raw -D option tokenises the parens). Everything else is COMPILE_OPTIONS.
# The Makefile's `-D'asm(x)='` + `-include pc_forward_decls.h` are folded into ONE force-include
# prelude (cmake_game_prelude.h), because CMake per-source options drop function-like macros and
# de-duplicate repeated `-include` flags -- so a single `-include` is the only reliable form here.
foreach(_g ${GAME_SRCS})
    set_source_files_properties(${_g} PROPERTIES
        COMPILE_DEFINITIONS "PERMUTER;PC_PORT;PC_FEAT"
        COMPILE_OPTIONS "-std=gnu89;-fno-builtin-csqrt;-include;${PC_DIR}/include/cmake_game_prelude.h;${GAME_WARN}"
        INCLUDE_DIRECTORIES "${STAGE_DIR};${GEN_ASSET_DIR};${PC_DIR}/include;${PROJECT_ROOT}")
endforeach()
if(APPLE)
    foreach(_g ${GAME_SRCS})
        set_property(SOURCE ${_g} APPEND PROPERTY COMPILE_OPTIONS "-fwritable-strings")
        get_filename_component(_apple_stem ${_g} NAME_WE)
        set_property(SOURCE ${_g} APPEND PROPERTY COMPILE_DEFINITIONS "VH_APPLE_TU_${_apple_stem}")
    endforeach()
endif()
# Always-on runtime-gated probes (Makefile: UNCONDITIONAL, self-gating at runtime).
set_property(SOURCE ${SRC_DIR}/window.c ${SRC_DIR}/object.c ${SRC_DIR}/main_menu.c
             APPEND PROPERTY COMPILE_DEFINITIONS PC_DEBUG_UI_LOG)
# PC_DEBUG_PATH_STEP: diagnostic-only travel-cost/path OOB probe (src/path_grids.c). It root-caused
# the gTravelAscentCost/DescentCost overrun (now fixed by widening those tables) and the residual
# reports are the confirmed-benign map-boundary reads -- so it's off by default (pure console noise
# for players). Re-enable by uncommenting to re-investigate path/travel logic. The probe code stays
# in-tree, #ifdef-gated; the matching build never defines it (byte-exact unaffected either way).
# set_property(SOURCE ${SRC_DIR}/path_grids.c APPEND PROPERTY COMPILE_DEFINITIONS PC_DEBUG_PATH_STEP)

# -- backend files with special profiles --------------------------------------------------------
foreach(_e libetc pc_diag pc_battle_speed)   # pc_diag/pc_battle_speed: extracted from libetc.c, same profile
    set_source_files_properties(${PC_DIR}/src/${_e}.c PROPERTIES
        COMPILE_DEFINITIONS "PERMUTER"
        INCLUDE_DIRECTORIES "${PC_DIR}/include;${STAGE_DIR};${SDL2_INCLUDE_DIRS}")
endforeach()
set_source_files_properties(${PC_DIR}/src/libsnd.c PROPERTIES
    COMPILE_DEFINITIONS "PERMUTER"
    INCLUDE_DIRECTORIES "${PC_DIR}/include;${STAGE_DIR};${OPENAL_INCLUDE_DIR}")
set_source_files_properties(${PC_DIR}/src/libgte.c PROPERTIES COMPILE_OPTIONS "-fno-builtin")
set_source_files_properties(${PC_DIR}/src/pc_gpu_window.c PROPERTIES
    INCLUDE_DIRECTORIES "${PC_DIR}/include;${SDL2_INCLUDE_DIRS}")

# -- hand-written data files: DATA_PERM need gnu89+PERMUTER+stage -------------------------------
foreach(_d ${DATA_PERM})
    vh_source_path(_data_source "${_d}")
    set_source_files_properties(${_data_source} PROPERTIES
        COMPILE_DEFINITIONS "PERMUTER"
        COMPILE_OPTIONS "-std=gnu89;-fno-builtin-csqrt"
        INCLUDE_DIRECTORIES "${STAGE_DIR};${PC_DIR}/include")
endforeach()

# pc_balance.c is backend LOGIC (the Tactical Mode balance patcher), not data, but it reads game
# structs/globals, so it needs the same gnu89+PERMUTER+stage profile as DATA_PERM (matches the Makefile
# rule at Makefile:396-397). It lives in the foreach source list above; this sets its compile profile.
set_source_files_properties(${PC_DIR}/src/pc_balance.c PROPERTIES
    COMPILE_DEFINITIONS "PERMUTER"
    COMPILE_OPTIONS "-std=gnu89"
    INCLUDE_DIRECTORIES "${STAGE_DIR};${PC_DIR}/include")

# -- warning policy on PORT code (the same 18 stems the Makefile treats as backend) ---------------
# MUST come after every set_source_files_properties above: that call OVERWRITES COMPILE_OPTIONS,
# so these appends would otherwise be silently clobbered (bit pc_balance/libgte on first attempt).
foreach(_b ${BACKEND_PLAIN} libetc pc_diag pc_battle_speed libsnd libgte pc_gpu_window pc_balance)
    vh_source_path(_port_source "${_b}")
    set_property(SOURCE ${_port_source} APPEND PROPERTY COMPILE_OPTIONS "${PORT_WARN}")
endforeach()
foreach(_b libetc libsnd pc_balance)   # compile through the staged game headers -> game csqrt(int) decl
    set_property(SOURCE ${PC_DIR}/src/${_b}.c APPEND PROPERTY COMPILE_OPTIONS "-fno-builtin-csqrt")
endforeach()

# ---- data-segment generator (mid-build) -------------------------------------------------------
# Runs after the objects compile; link-probes them for undefined symbols and emits generated_data.c.
#
# Env vars are built into a list so that ONLY non-empty ones are passed: `cmake -E env NAME=`
# (empty value) makes cmake treat the following token as the command to execute ("permission
# denied"), which silently stopped the generator from ever running. VH_TARGET_MARCH is omitted
# entirely (generator falls back to native width via VH_CC -- correct for x86-64 Linux/Win and
# ARM64 macOS alike); VH_SAN is appended only when a sanitizer is configured.
# OpenGL link name differs by platform: -lGL on Linux, -lopengl32 (the Win32 ICD loader) on Windows.
# The generator only link-PROBES for undefined symbols, but a lib it cannot find (`ld: cannot find
# -lGL`) can abort the probe before the undefined-ref lines are emitted, so give it the right name.
# Library link names differ by platform, and EVERY -l here must resolve: a `ld: cannot find -lXXX`
# aborts the probe link before any "undefined reference" line is emitted, so the generator would
# see 0 undefined symbols and produce an empty data segment. OpenGL: -lGL (Linux) / -lopengl32
# (Win32 ICD loader). OpenAL: -lopenal (Linux) / -lOpenAL32 (MinGW import lib libOpenAL32.dll.a).
if(WIN32)
    set(_GEN_MINGW_SEARCH "")
    if(VH_MINGW_PREFIX)
        set(_GEN_MINGW_SEARCH "-L${VH_MINGW_PREFIX}/lib")
    endif()
    set(_GEN_GL_LIB "-lopengl32")
    set(_GEN_AL_LIB "-lOpenAL32")
    set(_GEN_SDL_LIB "-lSDL2")
elseif(APPLE)
    set(_GEN_GL_LIB "")
    set(_GEN_AL_LIB "-framework OpenAL")
    if(SDL2_GEN_LIB MATCHES "\\.framework$")
        # find_library returns the framework directory. A raw clang link needs -F/-framework;
        # passing the directory as though it were a dylib fails before undefined-symbol discovery.
        get_filename_component(_SDL2_FRAMEWORK_PARENT "${SDL2_GEN_LIB}" DIRECTORY)
        set(_GEN_SDL_LIB "-F${_SDL2_FRAMEWORK_PARENT} -framework SDL2")
    else()
        set(_GEN_SDL_LIB "${SDL2_GEN_LIB}")
    endif()
else()
    set(_GEN_GL_LIB "-lGL")
    set(_GEN_AL_LIB "-lopenal")
    set(_GEN_SDL_LIB "-lSDL2")
endif()
# libgpu.o references WebPDecodeRGBA/WebPFree (under VH_HD_WEBP); the probe link must resolve them so they
# don't pollute the undefined-symbol list the generator harvests.
set(_GEN_WEBP_LIB "")
if(VH_WEBP)
    if(APPLE)
        set(_GEN_WEBP_LIB "${WEBP_LIB}")
    else()
        set(_GEN_WEBP_LIB "-lwebp")
    endif()
endif()
set(_GEN_AV_LIBS "")
if(VH_HDVIDEO)
    if(APPLE)
        # The probe is a raw compiler invocation, outside CMake's target link interface. Use the
        # concrete Homebrew/CMake results so it does not depend on a global /opt/homebrew search path.
        set(_GEN_AV_LIBS "${AVFORMAT_LIB} ${AVCODEC_LIB} ${AVUTIL_LIB} ${SWSCALE_LIB}")
    else()
        set(_GEN_AV_LIBS "-lavformat -lavcodec -lavutil -lswscale")
    endif()
    if(WIN32)
        # The generator's undefined-symbol PROBE link (VH_LINK_LIBS) runs the cross linker directly. The
        # STATIC libav lives outside the default MinGW lib path, so without -L<prefix>/lib the probe link
        # HARD-fails on "cannot find -lavformat" -- it then prints no "undefined reference" lines, and the
        # generator emits an EMPTY data segment (every game global then undefined at the real link). Add the
        # search path (+ -lbcrypt for avutil's RNG); dependency order avformat->avcodec->swscale->avutil.
        set(_ff "${VH_MINGW_FFMPEG}")
        if(NOT _ff)
            set(_ff "$ENV{VH_MINGW_FFMPEG}")
        endif()
        if(_ff)
            set(_GEN_AV_LIBS "-L${_ff}/lib -lavformat -lavcodec -lswscale -lavutil -lbcrypt")
        endif()
    endif()
endif()
set(GENERATED_C ${GEN_DIR}/generated_data.c)
set(GENERATED_STAMP ${GEN_DIR}/generated_data.stamp)
set(GEN_ENV
    "VH_BUILD_DIR=${GEN_DIR}"
    "VH_CC=${CMAKE_C_COMPILER}"
    "VH_OBJ_FILES=$<JOIN:$<TARGET_OBJECTS:vh_objs>, >"
    "VH_PSX_EXE=${VH_PSX_EXE}"
    "VH_LINK_LIBS=${_GEN_MINGW_SEARCH} ${_GEN_SDL_LIB} ${_GEN_AL_LIB} ${_GEN_GL_LIB} -lm ${_GEN_WEBP_LIB} ${_GEN_AV_LIBS}")
if(APPLE AND CMAKE_OSX_ARCHITECTURES)
    # CMake adds -arch to its own compile/link commands, but the data generator invokes clang
    # directly. Keep its undefined-symbol link and executable sizeof probe on the same slice(s).
    set(_GEN_APPLE_ARCH_FLAGS "")
    foreach(_arch IN LISTS CMAKE_OSX_ARCHITECTURES)
        string(APPEND _GEN_APPLE_ARCH_FLAGS " -arch ${_arch}")
    endforeach()
    string(STRIP "${_GEN_APPLE_ARCH_FLAGS}" _GEN_APPLE_ARCH_FLAGS)
    list(APPEND GEN_ENV "VH_TARGET_MARCH=${_GEN_APPLE_ARCH_FLAGS}")
endif()
if(VH_SANITIZE)
    list(APPEND GEN_ENV "VH_SAN=${VH_SANITIZE}")
endif()
if(WIN32)
    # The generator's sizeof-probe compiles the staged PsyQ headers directly (not through CMake's
    # add_compile_options), so it needs the same MinGW compat shim force-included, or it hits the
    # u_char/u_short errors inside those headers.
    list(APPEND GEN_ENV "VH_EXTRA_CFLAGS=-include ${PC_DIR}/include/pc_win_compat.h")
    # Cross-compile: the sizeof probe is compiled AND RUN to read struct sizes, so it must build for
    # the HOST (Linux), not the MinGW target -- a .exe probe cannot execute here. gcc gives the same
    # pointer-free sizes as the Win64 target. Without this the probe .exe never runs and every symbol
    # is left undefined -> the final link fails.
    list(APPEND GEN_ENV "VH_HOST_CC=gcc")
endif()
add_custom_command(
    OUTPUT ${GENERATED_STAMP}
    BYPRODUCTS ${GENERATED_C}
    COMMAND ${CMAKE_COMMAND} -E env ${GEN_ENV}
            ${Python3_EXECUTABLE} ${PC_DIR}/tools/build_data_segment.py
    COMMAND ${CMAKE_COMMAND} -E touch ${GENERATED_STAMP}
    DEPENDS vh_objs $<TARGET_OBJECTS:vh_objs> ${PC_DIR}/tools/build_data_segment.py ${PSX_VALIDATION_DEP}
    WORKING_DIRECTORY ${PC_DIR}
    COMMENT "Generating data segment (generated_data.c)"
    VERBATIM)
set_source_files_properties(${GENERATED_C} PROPERTIES GENERATED TRUE
    OBJECT_DEPENDS ${GENERATED_STAMP}
    COMPILE_DEFINITIONS "PERMUTER"
    COMPILE_OPTIONS "-std=gnu89;-fno-builtin-csqrt"
    INCLUDE_DIRECTORIES "${STAGE_DIR};${PC_DIR}/include")
if(APPLE)
    set_property(SOURCE ${GENERATED_C} APPEND PROPERTY COMPILE_OPTIONS "-Wno-macro-redefined")
endif()

# ---- final executable -------------------------------------------------------------------------
add_executable(vandalhearts_pc $<TARGET_OBJECTS:vh_objs> ${GENERATED_C})
target_link_libraries(vandalhearts_pc PRIVATE SDL2::SDL2 ${OPENAL_LIB} m Threads::Threads)
if(NOT APPLE)
    target_link_libraries(vandalhearts_pc PRIVATE OpenGL::GL)
endif()
if(VH_WEBP)
    target_link_libraries(vandalhearts_pc PRIVATE ${WEBP_LIB})
endif()
if(VH_HDVIDEO)
    if(WIN32)
        # The Windows build links a STATIC libav (the shippable form -- see tools/build-ffmpeg-static.sh;
        # point CMake at it with -DCMAKE_PREFIX_PATH=<prefix>). Static archives resolve left-to-right, and
        # the four libs cross-reference, so wrap them in a link group (order-independent). +bcrypt for
        # avutil's BCryptGenRandom. On a shared libav these flags are harmless.
        target_link_libraries(vandalhearts_pc PRIVATE
            -Wl,--start-group ${AVFORMAT_LIB} ${AVCODEC_LIB} ${SWSCALE_LIB} ${AVUTIL_LIB} -Wl,--end-group
            bcrypt)
    else()
        target_link_libraries(vandalhearts_pc PRIVATE ${AVFORMAT_LIB} ${AVCODEC_LIB} ${AVUTIL_LIB} ${SWSCALE_LIB})
    endif()
endif()
if(UNIX AND NOT APPLE)
    # -rdynamic: the crash-dump backtrace in pc_bootstrap.c needs dynamic symbols. Linux/POSIX only
    # -- the Windows build has no backtrace()/signal path, and MinGW ld does not take -rdynamic.
    target_link_options(vandalhearts_pc PRIVATE -rdynamic)
endif()
if(WIN32)
    # SDL renames main->SDL_main via SDL_main.h wherever SDL.h is included (pc_gpu_window.c); the
    # real entry point is the game's own main() in src/main.c, so tell SDL to keep its hands off.
    target_compile_definitions(vandalhearts_pc PRIVATE SDL_MAIN_HANDLED)
    # Force the CONSOLE subsystem for bring-up/validation: SDL2's cmake target adds -mwindows (GUI),
    # which hides the game's stderr bootstrap/diagnostic prints (disc mount, window open, errors).
    # A console launch shows them; the SDL game window still opens either way. Switch back to GUI
    # (drop this / use -mwindows) for the clean end-user release once the Windows run is validated.
    target_link_options(vandalhearts_pc PRIVATE -mconsole -Wl,--no-insert-timestamp)

    # Bundle the runtime DLLs next to the .exe so it runs on a stock Windows box (no MSYS on PATH).
    # SDL2/OpenAL are genuine third-party deps; the rest are the MinGW GCC runtime that they (and our
    # own code) link against and are NOT Windows system DLLs, so they must ship too:
    #   libwinpthread-1  pthreads (our audio-streaming threads)
    #   libgcc_s_seh-1   GCC unwinder/runtime
    #   libstdc++-6      C++ runtime (OpenAL-soft is C++)
    #   libssp-0         stack-protector runtime (-fstack-protector)
    # The UCRT (api-ms-win-crt-*) and OPENGL32/KERNEL32 are OS components on Win10+ and need no bundling.
    set(_MINGW_RUNTIME_DIRS /usr/${MINGW_TARGET}/bin
        ${VH_MINGW_ROOT}/${MINGW_TARGET}/bin ${VH_MINGW_ROOT}/${MINGW_TARGET}/lib
        ${VH_MINGW_ROOT}/mingw/bin ${VH_MINGW_ROOT}/mingw/lib)
    if(VH_MINGW_PREFIX)
        list(PREPEND _MINGW_RUNTIME_DIRS ${VH_MINGW_PREFIX}/bin ${VH_MINGW_PREFIX}/lib)
    endif()
    foreach(_runtime_probe libwinpthread-1.dll libgcc_s_seh-1.dll libstdc++-6.dll libssp-0.dll)
        execute_process(COMMAND ${CMAKE_C_COMPILER} -print-file-name=${_runtime_probe}
            OUTPUT_VARIABLE _runtime_path OUTPUT_STRIP_TRAILING_WHITESPACE)
        if(IS_ABSOLUTE "${_runtime_path}" AND EXISTS "${_runtime_path}")
            get_filename_component(_runtime_dir "${_runtime_path}" DIRECTORY)
            list(APPEND _MINGW_RUNTIME_DIRS "${_runtime_dir}")
        endif()
    endforeach()
    list(REMOVE_DUPLICATES _MINGW_RUNTIME_DIRS)
    foreach(_dll SDL2.dll OpenAL32.dll libwinpthread-1.dll libgcc_s_seh-1.dll libstdc++-6.dll libssp-0.dll)
        unset(_MINGW_DLL CACHE)
        find_file(_MINGW_DLL NAMES ${_dll} PATHS ${_MINGW_RUNTIME_DIRS} NO_DEFAULT_PATH)
        if(_MINGW_DLL)
            add_custom_command(TARGET vandalhearts_pc POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        ${_MINGW_DLL} $<TARGET_FILE_DIR:vandalhearts_pc>
                COMMENT "Bundling ${_dll} next to the .exe")
        endif()
    endforeach()
    # libwebp (1.6 HD pack) + its sharpyuv split-out. Only the core decode lib is needed (NOT the
    # webpdecoder/demux/mux siblings), and the SONAME digit varies by distro (Arch: libwebp.dll; Debian:
    # libwebp-7.dll) -- so match "libwebp.dll" and "libwebp-<n>.dll" but not "libwebpdecoder.dll" etc.
    if(VH_WEBP)
        set(_WEBP_DLLS)
        foreach(_runtime_dir ${_MINGW_RUNTIME_DIRS})
            file(GLOB _found_webp ${_runtime_dir}/libwebp.dll ${_runtime_dir}/libwebp-*.dll
                                  ${_runtime_dir}/libsharpyuv.dll ${_runtime_dir}/libsharpyuv-*.dll)
            list(APPEND _WEBP_DLLS ${_found_webp})
        endforeach()
        list(REMOVE_DUPLICATES _WEBP_DLLS)
        foreach(_wd ${_WEBP_DLLS})
            add_custom_command(TARGET vandalhearts_pc POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different ${_wd} $<TARGET_FILE_DIR:vandalhearts_pc>
                COMMENT "Bundling ${_wd} next to the .exe")
        endforeach()
    endif()
    # libav (ffmpeg) for HD FMV decode. The core decode DLLs are avformat/avcodec/avutil/swscale (+
    # swresample). ffmpeg has many transitive codec deps, but H.264 decode + mp4 demux + yuv->rgb only
    # pull a subset; the SONAME suffix varies (avcodec-62.dll / libavcodec.dll). GLOB the av*/sw* set and
    # copy what the MinGW package ships. NOTE: verify the full runtime DLL set once mingw-w64-ffmpeg is
    # installed and a Windows release is built (a missing transitive dep shows as a load error at launch).
    if(VH_HDVIDEO)
        set(_AV_DLLS)
        foreach(_runtime_dir ${_MINGW_RUNTIME_DIRS})
            file(GLOB _found_av ${_runtime_dir}/avformat*.dll ${_runtime_dir}/avcodec*.dll
                                ${_runtime_dir}/avutil*.dll ${_runtime_dir}/swscale*.dll
                                ${_runtime_dir}/swresample*.dll ${_runtime_dir}/libavformat*.dll
                                ${_runtime_dir}/libavcodec*.dll ${_runtime_dir}/libavutil*.dll
                                ${_runtime_dir}/libswscale*.dll)
            list(APPEND _AV_DLLS ${_found_av})
        endforeach()
        list(REMOVE_DUPLICATES _AV_DLLS)
        foreach(_ad ${_AV_DLLS})
            add_custom_command(TARGET vandalhearts_pc POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different ${_ad} $<TARGET_FILE_DIR:vandalhearts_pc>
                COMMENT "Bundling ${_ad} next to the .exe")
        endforeach()
    endif()
endif()
# generated_data.c is compiled into this target (not vh_objs), so sanitizer flags land here; the
# sanitizer runtime must also be on the link line.
if(VH_SAN_LIST)
    target_compile_options(vandalhearts_pc PRIVATE ${VH_SAN_LIST})
    target_link_options(vandalhearts_pc PRIVATE ${VH_SAN_LIST})
endif()
set_target_properties(vandalhearts_pc PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Ship the user config template next to the binary (all platforms). copy_if_different, so a user's
# edited copy in the build dir is not clobbered on rebuild.
add_custom_command(TARGET vandalhearts_pc POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${PC_DIR}/vandalhearts.ini $<TARGET_FILE_DIR:vandalhearts_pc>
    COMMENT "Copying vandalhearts.ini next to the binary")

if(APPLE)
    string(REPLACE ";" "," _MAC_ARCHITECTURES "${CMAKE_OSX_ARCHITECTURES}")
    if(NOT _MAC_ARCHITECTURES)
        set(_MAC_ARCHITECTURES "native")
    endif()
    # This deliberately runs on every default build.  Git can become dirty, or HEAD can move,
    # without changing an object-file timestamp; tying the manifest only to the link step would
    # therefore leave plausible-looking but stale provenance beside an unchanged executable.
    add_custom_target(vh_macos_buildinfo ALL
        COMMAND ${Python3_EXECUTABLE} ${PC_DIR}/tools/gen_macos_buildinfo.py
                --repo ${PROJECT_ROOT}
                --binary $<TARGET_FILE:vandalhearts_pc>
                --psx ${VH_PSX_EXE}
                --krom ${VH_KROM_SOURCE}
                --output $<TARGET_FILE_DIR:vandalhearts_pc>/VH_BUILDINFO.txt
                --compiler ${CMAKE_C_COMPILER}
                --architectures ${_MAC_ARCHITECTURES}
                --build-type ${CMAKE_BUILD_TYPE}
                --webp ${VH_WEBP}
                --hdvideo ${VH_HDVIDEO}
        DEPENDS vandalhearts_pc ${PC_DIR}/tools/gen_macos_buildinfo.py
        COMMENT "Writing macOS reproducibility manifest"
        VERBATIM)
endif()
