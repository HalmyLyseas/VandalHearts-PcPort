# Vandal Hearts — native PC port build, the CMake twin of platform/pc/Makefile (same per-file
# flag groups, staging tree and mid-build data-segment generator; the Makefile is the reference).
# The matching-decomp build is separate. See docs/building.md, "How the port build works".

cmake_minimum_required(VERSION 3.16)
project(vandalhearts_pc C)

set(CMAKE_C_STANDARD 99)              # backend files are modern C; game files force -std=gnu89 per-source
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)       # -g, matching the Makefile's default
endif()

# Sanitizer flags, e.g. -DVH_SANITIZE="-fsanitize=bounds -fno-omit-frame-pointer" (the twin of
# `make ubsan`; 32-bit ASan stays in the Makefile). Applied to every compile, the link AND the
# generator's probe link (via VH_SAN) so its instrumented objects link.
set(VH_SANITIZE "" CACHE STRING "Sanitizer flags, e.g. -fsanitize=bounds -fno-omit-frame-pointer")
separate_arguments(VH_SAN_LIST NATIVE_COMMAND "${VH_SANITIZE}")

set(PC_DIR       ${CMAKE_CURRENT_SOURCE_DIR})
set(PROJECT_ROOT ${PC_DIR}/../..)
set(SRC_DIR      ${PROJECT_ROOT}/src)

# ---- REGION (mirrors the Makefile) ------------------------------------------------------------
# VH_REGION=us|jp selects the game-code tree; VH_UNIFIED_CORE=ON compiles this core for the
# unified binary. See docs/building.md, "The unified binary".
set(VH_REGION "us" CACHE STRING "Game region core: us or jp")
option(VH_UNIFIED_CORE "Compile for the unified binary (VH_UNIFIED: explicit bootstrap)" OFF)
option(VH_BLOB "Produce the prefix-renamed core_<region>.o instead of an executable" OFF)
set(VH_UNIFIED_CORES "" CACHE STRING "Final unified link: semicolon list of core_us.o;core_jp.o")
if(VH_REGION STREQUAL "jp")
    set(GAME_ROOT ${PROJECT_ROOT}/jp)
    set(PSX_BASENAME SLPM_860.07)
    set(REGION_DEF VH_REGION_JP)
    set(GAME_DEFS "PERMUTER;PC_PORT;PC_FEAT;VH_REGION_JP")  # the JP core carries the full PC_FEAT set
else()
    set(GAME_ROOT ${PROJECT_ROOT})
    set(PSX_BASENAME SLUS_004.47)
    set(REGION_DEF VH_REGION_US)
    set(GAME_DEFS "PERMUTER;PC_PORT;PC_FEAT;VH_REGION_US")
endif()
set(GEN_DIR      ${CMAKE_BINARY_DIR}/datagen)     # generator's VH_BUILD_DIR (staging + work + output)
set(STAGE_DIR    ${GEN_DIR}/include_stage)
set(GEN_ASSET_DIR ${GEN_DIR}/generated_assets)
set(GEN_DATA_DIR ${GEN_DIR}/generated_sources)

set(VH_PSX_EXE "${GAME_ROOT}/${PSX_BASENAME}" CACHE FILEPATH "Original PS-X EXE of this region, used for generated data")
set(VH_KROM_SOURCE "${PROJECT_ROOT}/external/psy-q/3.6/PSYQ/BIN/KROMDAT.BIN" CACHE FILEPATH
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
if(VH_REGION STREQUAL "jp")
    # Shared-header rule: 11 headers stay the US gate-carrying copies staged above; the 4
    # genuinely different ones come from jp/include (field.h via the hand-merged region-jp
    # variant that keeps the US gate blocks). docs/building.md, "Region source selection".
    foreach(_rh audio.h card.h units.h)
        file(CREATE_LINK ${GAME_ROOT}/include/${_rh} ${STAGE_DIR}/${_rh} SYMBOLIC)
    endforeach()
    file(CREATE_LINK ${PC_DIR}/include/region-jp/field.h ${STAGE_DIR}/field.h SYMBOLIC)
endif()
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

# libwebp: standard HD-pack dependency (.webp is ~7x smaller than raw .hdi). -DVH_WEBP=OFF builds
# without it; the engine's `#ifdef VH_HD_WEBP` then loads only raw .hdi packs. Mirrors NO_WEBP.
option(VH_WEBP "libwebp HD-pack support (.webp replacements)" ON)
if(VH_WEBP)
    find_library(WEBP_LIB NAMES webp REQUIRED)
    find_path(WEBP_INCLUDE_DIR webp/decode.h)
endif()

# libav (ffmpeg): standard HD-pack dependency (H.264 .mp4 movies replacing the STR FMVs).
# -DVH_HDVIDEO=OFF keeps the native MDEC path (engine `#ifdef VH_HD_VIDEO`). Mirrors NO_HDVIDEO.
option(VH_HDVIDEO "libav HD FMV support (.mp4 replacements)" ON)
if(VH_HDVIDEO)
    find_library(AVFORMAT_LIB NAMES avformat REQUIRED)
    find_library(AVCODEC_LIB  NAMES avcodec  REQUIRED)
    find_library(AVUTIL_LIB   NAMES avutil   REQUIRED)
    find_library(SWSCALE_LIB  NAMES swscale  REQUIRED)
    find_path(AVCODEC_INCLUDE_DIR libavcodec/avcodec.h)
endif()

# ---- source lists -----------------------------------------------------------------------------
if(NOT VH_UNIFIED_CORES)   # normal / blob modes; the unified-final link skips to the executable
# Inventory from the region's own tree; shared TUs (shared_tus.txt) always compile from the US
# tree so gated fixes land once; the rest from the region tree.
file(GLOB _game_inventory RELATIVE ${GAME_ROOT}/src CONFIGURE_DEPENDS
     ${GAME_ROOT}/src/*.c ${GAME_ROOT}/src/*/*.c)
set(GAME_SRCS "")
if(VH_REGION STREQUAL "jp")
    file(STRINGS ${PC_DIR}/shared_tus.txt _shared_tus REGEX "^[^#]")
    foreach(_rel ${_game_inventory})
        list(FIND _shared_tus "${_rel}" _shared_idx)
        if(_shared_idx GREATER -1)
            list(APPEND GAME_SRCS ${SRC_DIR}/${_rel})
        else()
            list(APPEND GAME_SRCS ${GAME_ROOT}/src/${_rel})
        endif()
    endforeach()
else()
    foreach(_rel ${_game_inventory})
        list(APPEND GAME_SRCS ${SRC_DIR}/${_rel})
    endforeach()
endif()
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
set(BACKEND_PLAIN   pc_lang pc_lang_list pc_lang_font libcd libspu libkernel libgpu pc_raster pc_gpu_trace pc_hdpack libsn pc_bootstrap pc_xa pc_spu pc_mdec pc_kanji_font pc_overlay pc_saves pc_movie_subs pc_hdvideo)
if(VH_REGION STREQUAL "jp")
    # Langpack is US-only by design; the JP core links the documented no-pack stubs instead.
    list(REMOVE_ITEM BACKEND_PLAIN pc_lang pc_lang_font)
    list(APPEND BACKEND_PLAIN pc_lang_stub)
endif()
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

# GENERATED game/BIOS-derived data files (gitignored): reconstruct copyrighted content (Konami UI
# text, PS1 BIOS kanji glyphs) from the user's own byte-exact executable + PsyQ KROMDAT.BIN, so
# the source tree ships none of it. Mirrors the Makefile's gen rules.
set(GENERATED_DATA_C pc_kanji_font pc_spell_descriptions pc_item_descriptions pc_evt_entities pc_string_table)
if(VH_REGION STREQUAL "jp")
    # The JP core regenerates ALL reconstructions from SLPM_860.07 (the src/ copies are the US
    # originals); the generators are region-parametric.
    list(APPEND GENERATED_DATA_C pc_battle_data pc_unit_anim_data pc_sprite_box_quads pc_event_anim_data)
endif()
foreach(_g ${GENERATED_DATA_C})
    string(REPLACE "pc_" "" _base "${_g}")   # gen_<base>.py, e.g. pc_string_table -> gen_string_table.py
    set(_generated_source ${GEN_DATA_DIR}/${_g}.c)
    add_custom_command(
        OUTPUT ${_generated_source}
        COMMAND ${CMAKE_COMMAND} -E env "VH_PSX_EXE=${VH_PSX_EXE}"
                "VH_KROM_SOURCE=${VH_KROM_SOURCE}"
                "VH_GENERATED_OUT=${_generated_source}"
                "VH_REGION=${VH_REGION}" "VH_GAME_ROOT=${GAME_ROOT}"
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
if(NOT VH_REGION STREQUAL "jp")   # the 3 generated US incs; jp resolves everything from jp/assets
    add_dependencies(vh_objs vh_generated_assets)
endif()
# Region identity for every port TU (mirrors the Makefile's PORT_WARN += REGION_DEF), plus the
# unified-core flag that compiles pc_bootstrap.c's bootstrap constructors out.
target_compile_definitions(vh_objs PRIVATE ${REGION_DEF})
if(VH_UNIFIED_CORE)
    target_compile_definitions(vh_objs PRIVATE VH_UNIFIED)
endif()

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
# Folded into one force-included prelude (cmake_game_prelude.h) rather than repeated -include
# flags. See docs/building.md, "Game-source compile flags".
foreach(_g ${GAME_SRCS})
    if(VH_REGION STREQUAL "jp")
        # jp/assets holds ALL its own .inc/.bin extractions (incl. the 14 same-named ones that
        # must NOT resolve to the US copies), so GAME_ROOT replaces GEN_ASSET_DIR+PROJECT_ROOT.
        set(_game_incdirs "${STAGE_DIR};${PC_DIR}/include;${GAME_ROOT}")
    else()
        set(_game_incdirs "${STAGE_DIR};${GEN_ASSET_DIR};${PC_DIR}/include;${PROJECT_ROOT}")
    endif()
    set_source_files_properties(${_g} PROPERTIES
        COMPILE_DEFINITIONS "${GAME_DEFS}"
        COMPILE_OPTIONS "-std=gnu89;-fno-builtin-csqrt;-include;${PC_DIR}/include/cmake_game_prelude.h;${GAME_WARN}"
        INCLUDE_DIRECTORIES "${_game_incdirs}")
endforeach()
if(APPLE)
    foreach(_g ${GAME_SRCS})
        set_property(SOURCE ${_g} APPEND PROPERTY COMPILE_OPTIONS "-fwritable-strings")
        get_filename_component(_apple_stem ${_g} NAME_WE)
        set_property(SOURCE ${_g} APPEND PROPERTY COMPILE_DEFINITIONS "VH_APPLE_TU_${_apple_stem}")
    endforeach()
endif()
# Always-on runtime-gated probes (Makefile: UNCONDITIONAL, self-gating at runtime).
set_property(SOURCE ${SRC_DIR}/ui/window.c ${SRC_DIR}/core/object.c ${SRC_DIR}/states/main_menu.c
             APPEND PROPERTY COMPILE_DEFINITIONS PC_DEBUG_UI_LOG)
# PC_DEBUG_PATH_STEP (src/battle/path_grids.c) is a diagnostic-only path probe, off by default:
# its remaining reports are benign map-boundary reads. Re-enable with a set_property(SOURCE ...
# APPEND PROPERTY COMPILE_DEFINITIONS PC_DEBUG_PATH_STEP) line here (mirrors the Makefile).

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

# pc_balance.c is backend logic (the Tactical Mode balance patcher) that reads game structs and
# globals, so it needs the same gnu89+PERMUTER+stage profile as DATA_PERM (matches the Makefile).
set_source_files_properties(${PC_DIR}/src/pc_balance.c PROPERTIES
    COMPILE_DEFINITIONS "PERMUTER"
    COMPILE_OPTIONS "-std=gnu89"
    INCLUDE_DIRECTORIES "${STAGE_DIR};${PC_DIR}/include")

# pc_overlay.c's RETURN TO TITLE action rewrites engine state, so it takes the same staged
# profile as pc_balance.c (matches the Makefile rule).
set_source_files_properties(${PC_DIR}/src/pc_overlay.c PROPERTIES
    COMPILE_DEFINITIONS "PERMUTER"
    COMPILE_OPTIONS "-std=gnu89"
    INCLUDE_DIRECTORIES "${STAGE_DIR};${PC_DIR}/include")

# -- warning policy on PORT code (the stems the Makefile treats as backend) -----------------------
# MUST come after every set_source_files_properties above: that call OVERWRITES COMPILE_OPTIONS,
# so these appends would otherwise be silently clobbered.
foreach(_b ${BACKEND_PLAIN} libetc pc_diag pc_battle_speed libsnd libgte pc_gpu_window pc_balance)
    vh_source_path(_port_source "${_b}")
    set_property(SOURCE ${_port_source} APPEND PROPERTY COMPILE_OPTIONS "${PORT_WARN}")
endforeach()
foreach(_b libetc libsnd pc_balance pc_overlay)   # compile through the staged game headers -> game csqrt(int) decl
    set_property(SOURCE ${PC_DIR}/src/${_b}.c APPEND PROPERTY COMPILE_OPTIONS "-fno-builtin-csqrt")
endforeach()

# ---- data-segment generator (mid-build) -------------------------------------------------------
# Link-probes the compiled objects for undefined symbols and emits generated_data.c.
# See docs/building.md, "The data-segment generator's width".
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
# pc_hdpack.o references WebPDecodeRGBA/WebPFree (under VH_HD_WEBP); the probe link must resolve
# them so they don't pollute the undefined-symbol list the generator harvests.
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
        # The STATIC libav lives outside the default MinGW lib path: without -L<prefix>/lib the probe
        # link hard-fails on "cannot find -lavformat" and the generator emits an EMPTY data segment.
        # + -lbcrypt for avutil's RNG; dependency order avformat->avcodec->swscale->avutil.
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
    "VH_GAME_ROOT=${GAME_ROOT}"
    "VH_PSX_BASENAME=${PSX_BASENAME}"
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
    # The sizeof probe is compiled AND RUN, so it must build for the HOST, not the MinGW target
    # (a .exe cannot execute here); gcc gives the same pointer-free sizes as Win64. Without this
    # every symbol is left undefined and the final link fails.
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

endif()   # NOT VH_UNIFIED_CORES (source lists / objects / generators)

# ---- final executable (or region-core blob) ---------------------------------------------------
if(VH_BLOB)
    # Unified core: partial-link this region's ENTIRE object set and prefix-rename every defined
    # external (incl. weak+common; COFF .refptr thunks re-keyed per blob -- see
    # tools/prefix_blob.py for the COMDAT-dedup hazard).
    add_library(vh_gen_obj OBJECT ${GENERATED_C})
    target_compile_definitions(vh_gen_obj PRIVATE ${REGION_DEF})
    foreach(_t NM OBJCOPY OBJDUMP LINKER)   # some toolchains leave these unset; fall back to PATH names
        if(NOT CMAKE_${_t})
            string(TOLOWER ${_t} _tl)
            if(_tl STREQUAL "linker")
                set(_tl ld)
            endif()
            set(CMAKE_${_t} ${_tl})
        endif()
    endforeach()
    set(_BLOB_DEFSYM "")
    if(VH_REGION STREQUAL "jp")
        set(_BLOB_DEFSYM --defsym DrawSjisText=DrawText --defsym MsgBox_IsFinished=ConsumeMsgBoxPagePause)
    endif()
    add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/core_${VH_REGION}.o
        COMMAND ${CMAKE_LINKER} -r -o ${CMAKE_BINARY_DIR}/core_${VH_REGION}_raw.o
                "$<TARGET_OBJECTS:vh_objs>" "$<TARGET_OBJECTS:vh_gen_obj>" ${_BLOB_DEFSYM}
        COMMAND ${Python3_EXECUTABLE} ${PC_DIR}/tools/prefix_blob.py
                ${CMAKE_BINARY_DIR}/core_${VH_REGION}_raw.o
                ${CMAKE_BINARY_DIR}/core_${VH_REGION}.o ${VH_REGION}_
                --nm=${CMAKE_NM} --objcopy=${CMAKE_OBJCOPY} --objdump=${CMAKE_OBJDUMP}
        DEPENDS vh_objs vh_gen_obj "$<TARGET_OBJECTS:vh_objs>" "$<TARGET_OBJECTS:vh_gen_obj>"
                ${PC_DIR}/tools/prefix_blob.py
        COMMAND_EXPAND_LISTS VERBATIM
        COMMENT "Partial-linking + prefix-renaming region core ${VH_REGION}")
    add_custom_target(vh_core ALL DEPENDS ${CMAKE_BINARY_DIR}/core_${VH_REGION}.o)
elseif(VH_UNIFIED_CORES)
    # Unified final link: the shared layer + both prefixed cores. All platform link logic below
    # (libs, -mconsole, DLL bundling) applies to this target exactly as to the normal exe.
    add_executable(vandalhearts_pc ${PC_DIR}/src/pc_region_main.c ${VH_UNIFIED_CORES})
    set_source_files_properties(${PC_DIR}/src/pc_region_main.c PROPERTIES
        COMPILE_OPTIONS "${PORT_WARN}")
else()
add_executable(vandalhearts_pc $<TARGET_OBJECTS:vh_objs> ${GENERATED_C})
if(VH_REGION STREQUAL "jp")
    # The JP binary carries these two names for one address each (see the Makefile).
    target_link_options(vandalhearts_pc PRIVATE
        "LINKER:--defsym,DrawSjisText=DrawText"
        "LINKER:--defsym,MsgBox_IsFinished=ConsumeMsgBoxPagePause")
endif()
endif()
if(NOT VH_BLOB)   # everything below configures the executable target
target_link_libraries(vandalhearts_pc PRIVATE SDL2::SDL2 ${OPENAL_LIB} m Threads::Threads)
if(NOT APPLE)
    target_link_libraries(vandalhearts_pc PRIVATE OpenGL::GL)
endif()
if(VH_WEBP)
    target_link_libraries(vandalhearts_pc PRIVATE ${WEBP_LIB})
endif()
if(VH_HDVIDEO)
    if(WIN32)
        # The Windows build links a STATIC libav (tools/build-ffmpeg-static.sh). The four archives
        # cross-reference, so they go in a link group; +bcrypt for avutil's BCryptGenRandom. Harmless
        # on a shared libav. See docs/cross-platform.md, "Toolchain".
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
    # real entry point is the game's own main() in src/core/main.c, so tell SDL to keep its hands off.
    target_compile_definitions(vandalhearts_pc PRIVATE SDL_MAIN_HANDLED)
    # CONSOLE subsystem: SDL2's cmake target adds -mwindows (GUI), which hides the game's stderr
    # diagnostics (disc mount, window open, errors). The SDL game window opens either way;
    # -mwindows would give a console-free end-user launch.
    target_link_options(vandalhearts_pc PRIVATE -mconsole -Wl,--no-insert-timestamp)

    # Bundle the runtime DLLs next to the .exe so it runs on a stock Windows box: SDL2/OpenAL plus
    # the MinGW GCC runtime (winpthread, gcc_s_seh, stdc++ for OpenAL-soft, ssp), none of which are
    # OS DLLs. The UCRT and OPENGL32/KERNEL32 are. See docs/cross-platform.md, "The distributable".
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
    # libwebp + its sharpyuv split-out. Only the core decode lib is needed (NOT the webpdecoder/
    # demux/mux siblings), and the SONAME digit varies by distro (libwebp.dll vs libwebp-7.dll),
    # so match "libwebp.dll" and "libwebp-<n>.dll" but not "libwebpdecoder.dll".
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
    # libav DLLs, only present with a SHARED libav (the release links it statically and ships none).
    # The SONAME suffix varies (avcodec-62.dll / libavcodec.dll), so GLOB the av*/sw* set; a missing
    # transitive codec DLL shows as a load error at launch.
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
endif()   # NOT VH_BLOB
