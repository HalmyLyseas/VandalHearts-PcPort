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
find_package(OpenGL REQUIRED)
find_package(Python3 REQUIRED COMPONENTS Interpreter)   # data-file + data-segment generators
find_library(OPENAL_LIB NAMES openal OpenAL32 REQUIRED)   # 'openal' on Linux, 'OpenAL32' on MinGW/Windows
find_path(OPENAL_INCLUDE_DIR AL/al.h)

# ---- source lists -----------------------------------------------------------------------------
file(GLOB GAME_SRCS ${SRC_DIR}/*.c)

# Backend + hand-written data files, grouped by their Makefile compile profile.
set(BACKEND_PLAIN   libcd libspu libkernel libgpu libsn pc_bootstrap pc_xa pc_spu pc_mdec pc_kanji_font pc_overlay pc_saves)
set(DATA_PERM       pc_battle_data pc_unit_anim_data pc_sprite_box_quads pc_event_anim_data pc_evt_entities)
set(DATA_PLAIN      pc_spell_descriptions pc_item_descriptions pc_string_table)

set(ALL_OBJ_SRCS ${GAME_SRCS})
foreach(_b ${BACKEND_PLAIN} libetc libsnd libgte pc_gpu_window pc_balance ${DATA_PERM} ${DATA_PLAIN})
    list(APPEND ALL_OBJ_SRCS ${PC_DIR}/src/${_b}.c)
endforeach()

# ---- GENERATED game/BIOS-derived data files (gitignored; regenerated at build) ----------------
# Reconstruct copyrighted content (Konami UI text, PS1 BIOS kanji glyphs) from the user's OWN copy
# (byte-exact build/SLUS_004.47 + PsyQ KROMDAT.BIN, both already required), so the source tree ships
# no copyrighted data. Mirrors the Makefile's gen rules. See exchange/60.
set(GENERATED_DATA_C pc_kanji_font pc_spell_descriptions pc_item_descriptions pc_evt_entities pc_string_table)
foreach(_g ${GENERATED_DATA_C})
    string(REPLACE "pc_" "" _base "${_g}")   # gen_<base>.py, e.g. pc_string_table -> gen_string_table.py
    add_custom_command(
        OUTPUT ${PC_DIR}/src/${_g}.c
        COMMAND ${Python3_EXECUTABLE} ${PC_DIR}/tools/gen_${_base}.py
        DEPENDS ${PC_DIR}/tools/gen_${_base}.py
        WORKING_DIRECTORY ${PC_DIR}
        COMMENT "Generating src/${_g}.c (from the byte-exact binary / PsyQ KROMDAT)"
        VERBATIM)
    set_source_files_properties(${PC_DIR}/src/${_g}.c PROPERTIES GENERATED TRUE)
endforeach()

# ---- OBJECT library: everything except the generated data segment -----------------------------
# An OBJECT library so the generator can be handed the real object paths ($<TARGET_OBJECTS:...>);
# CMake's object layout does not match the Makefile glob the generator used to assume.
add_library(vh_objs OBJECT ${ALL_OBJ_SRCS})

# Common include dirs for our own backend/data files.
target_include_directories(vh_objs PRIVATE ${PC_DIR}/include ${STAGE_DIR}
                           ${SDL2_INCLUDE_DIRS} ${OPENAL_INCLUDE_DIR})
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
        COMPILE_OPTIONS "-std=gnu89;-fno-builtin-csqrt;-include;${PC_DIR}/include/cmake_game_prelude.h"
        INCLUDE_DIRECTORIES "${STAGE_DIR};${PC_DIR}/include;${PROJECT_ROOT}")
endforeach()
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
set_source_files_properties(${PC_DIR}/src/libetc.c PROPERTIES
    COMPILE_DEFINITIONS "PERMUTER"
    INCLUDE_DIRECTORIES "${PC_DIR}/include;${STAGE_DIR};${SDL2_INCLUDE_DIRS}")
set_source_files_properties(${PC_DIR}/src/libsnd.c PROPERTIES
    COMPILE_DEFINITIONS "PERMUTER"
    INCLUDE_DIRECTORIES "${PC_DIR}/include;${STAGE_DIR};${OPENAL_INCLUDE_DIR}")
set_source_files_properties(${PC_DIR}/src/libgte.c PROPERTIES COMPILE_OPTIONS "-fno-builtin")
set_source_files_properties(${PC_DIR}/src/pc_gpu_window.c PROPERTIES
    INCLUDE_DIRECTORIES "${PC_DIR}/include;${SDL2_INCLUDE_DIRS}")

# -- hand-written data files: DATA_PERM need gnu89+PERMUTER+stage -------------------------------
foreach(_d ${DATA_PERM})
    set_source_files_properties(${PC_DIR}/src/${_d}.c PROPERTIES
        COMPILE_DEFINITIONS "PERMUTER"
        COMPILE_OPTIONS "-std=gnu89"
        INCLUDE_DIRECTORIES "${STAGE_DIR};${PC_DIR}/include")
endforeach()

# pc_balance.c is backend LOGIC (the Tactical Mode balance patcher), not data, but it reads game
# structs/globals, so it needs the same gnu89+PERMUTER+stage profile as DATA_PERM (matches the Makefile
# rule at Makefile:396-397). It lives in the foreach source list above; this sets its compile profile.
set_source_files_properties(${PC_DIR}/src/pc_balance.c PROPERTIES
    COMPILE_DEFINITIONS "PERMUTER"
    COMPILE_OPTIONS "-std=gnu89"
    INCLUDE_DIRECTORIES "${STAGE_DIR};${PC_DIR}/include")

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
    set(_GEN_GL_LIB "-lopengl32")
    set(_GEN_AL_LIB "-lOpenAL32")
else()
    set(_GEN_GL_LIB "-lGL")
    set(_GEN_AL_LIB "-lopenal")
endif()
set(GENERATED_C ${GEN_DIR}/generated_data.c)
set(GEN_ENV
    "VH_BUILD_DIR=${GEN_DIR}"
    "VH_CC=${CMAKE_C_COMPILER}"
    "VH_OBJ_FILES=$<JOIN:$<TARGET_OBJECTS:vh_objs>, >"
    "VH_LINK_LIBS=-lSDL2 ${_GEN_AL_LIB} ${_GEN_GL_LIB} -lm")
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
    OUTPUT ${GENERATED_C}
    COMMAND ${CMAKE_COMMAND} -E env ${GEN_ENV}
            ${Python3_EXECUTABLE} ${PC_DIR}/tools/build_data_segment.py
    DEPENDS vh_objs $<TARGET_OBJECTS:vh_objs> ${PC_DIR}/tools/build_data_segment.py
    WORKING_DIRECTORY ${PC_DIR}
    COMMENT "Generating data segment (generated_data.c)"
    VERBATIM)
set_source_files_properties(${GENERATED_C} PROPERTIES GENERATED TRUE
    COMPILE_DEFINITIONS "PERMUTER"
    COMPILE_OPTIONS "-std=gnu89"
    INCLUDE_DIRECTORIES "${STAGE_DIR};${PC_DIR}/include")

# ---- final executable -------------------------------------------------------------------------
add_executable(vandalhearts_pc $<TARGET_OBJECTS:vh_objs> ${GENERATED_C})
target_link_libraries(vandalhearts_pc PRIVATE SDL2::SDL2 ${OPENAL_LIB} OpenGL::GL m)
if(NOT WIN32)
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
    target_link_options(vandalhearts_pc PRIVATE -mconsole)

    # Bundle the runtime DLLs next to the .exe so it runs on a stock Windows box (no MSYS on PATH).
    # SDL2/OpenAL are genuine third-party deps; the rest are the MinGW GCC runtime that they (and our
    # own code) link against and are NOT Windows system DLLs, so they must ship too:
    #   libwinpthread-1  pthreads (our audio-streaming threads)
    #   libgcc_s_seh-1   GCC unwinder/runtime
    #   libstdc++-6      C++ runtime (OpenAL-soft is C++)
    #   libssp-0         stack-protector runtime (-fstack-protector)
    # The UCRT (api-ms-win-crt-*) and OPENGL32/KERNEL32 are OS components on Win10+ and need no bundling.
    set(_MINGW_BIN /usr/${MINGW_TARGET}/bin)
    foreach(_dll SDL2.dll OpenAL32.dll libwinpthread-1.dll libgcc_s_seh-1.dll libstdc++-6.dll libssp-0.dll)
        if(EXISTS ${_MINGW_BIN}/${_dll})
            add_custom_command(TARGET vandalhearts_pc POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        ${_MINGW_BIN}/${_dll} $<TARGET_FILE_DIR:vandalhearts_pc>
                COMMENT "Bundling ${_dll} next to the .exe")
        endif()
    endforeach()
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
