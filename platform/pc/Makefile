# PC-port build. Compiles the real game source (../../src/*.c) against the
# platform/pc backends instead of the real PSX toolchain, plus the backend
# implementations themselves.
#
# See exchange/10-phase-c-real-compilation.md for why this looks the way it
# does -- in particular, why GAME_C_FLAGS uses -Ibuild/include_stage (a
# symlink tree) instead of just -I../../include: GCC resolves a quoted
# #include relative to the *including file's own directory* before it ever
# consults -I flags, and since ../../include/common.h (etc.) themselves
# `#include "PsyQ/whatever.h"`, that would otherwise always find the real
# proprietary Sony headers at ../../include/PsyQ/ (still present locally,
# gitignored) instead of our clean-room ones, no matter what -I says.
# include_stage/ symlinks every real project header in individually, plus a
# directory symlink for PsyQ/ pointing at OUR clean-room headers instead --
# same effect as replacing include/PsyQ/, without touching the real tree.

# ---- REGION (exchange/102) -- which game-code tree this build compiles ------------------------
#   make link              -> REGION=us (default; behaviour identical to the pre-REGION build)
#   make link REGION=jp    -> the jp/ tree (SLPM-86007) into build-jp/
# The 54 TUs + 11 headers named by shared_tus.txt / tools/check_shared.py are byte-identical
# between the trees once the PC gate conditionals are stripped, so BOTH regions compile them
# from the US tree and every gated fix/feature lands once. The JP region takes the remaining
# TUs from jp/src, and the four genuinely-different headers from jp/include (field.h via the
# hand-merged include/region-jp/field.h, which carries the US gate blocks).
# JP carries the full PC_FEAT set since the exchange/105 transplant (Tactical, ally-cycle,
# threat overlay, fast-forward); langpack-only PC_FEAT sites stay US-only by design.
REGION         ?= us
ifeq ($(REGION),jp)
GAME_ROOT      := ../../jp
BUILD_DIR      := build-jp
PSX_BASENAME   := SLPM_860.07
GAME_FEAT_DEF  := -DPC_FEAT
REGION_DEF     := -DVH_REGION_JP
# The JP binary satisfies these two names by absolute address (undefined_additional.txt):
# they are the same functions under second names -- see exchange/102 §3.
REGION_LDFLAGS := -Wl,--defsym,DrawSjisText=DrawText -Wl,--defsym,MsgBox_IsFinished=ConsumeMsgBoxPagePause
else
GAME_ROOT      := ../..
BUILD_DIR      := build
PSX_BASENAME   := SLUS_004.47
GAME_FEAT_DEF  := -DPC_FEAT
REGION_DEF     := -DVH_REGION_US
REGION_LDFLAGS :=
endif
STAGE_DIR      := $(BUILD_DIR)/include_stage

# The file INVENTORY always comes from the region's real tree (identical 88-file layout on
# both sides); which tree each file is COMPILED from is decided per file below (shared vs
# region -- see the game object rules).
SRC_DIR        := ../../src
GAME_REL       := $(patsubst $(GAME_ROOT)/src/%,%,$(wildcard $(GAME_ROOT)/src/*.c $(GAME_ROOT)/src/*/*.c))
GAME_O_FILES   := $(addprefix $(BUILD_DIR)/src/,$(GAME_REL:.c=.o))

BACKEND_SRCS   := libetc.c pc_lang.c pc_lang_list.c pc_lang_font.c pc_diag.c pc_battle_speed.c libcd.c libsnd.c libspu.c libkernel.c libgte.c libgpu.c pc_raster.c pc_gpu_trace.c pc_hdpack.c pc_gpu_window.c libsn.c pc_bootstrap.c pc_xa.c pc_mdec.c pc_kanji_font.c pc_spu.c pc_overlay.c pc_saves.c pc_movie_subs.c pc_balance.c pc_hdvideo.c
ifeq ($(REGION),jp)
# The langpack engine is US-only by design (built on the US ASCII text path; exchange/101
# §4.3); the JP core links pc_lang_stub.c (the engine's documented no-pack behaviour) instead.
BACKEND_SRCS   := $(filter-out pc_lang.c pc_lang_font.c,$(BACKEND_SRCS)) pc_lang_stub.c
endif
BACKEND_OBJS   := $(patsubst %.c,$(BUILD_DIR)/%.o,$(BACKEND_SRCS))

# TARGET WIDTH. Default is now 64-bit (Stage 2.3 complete, 2026-07-21).
#
#   make link                                 -> 64-bit (default)
#   make link M32=-m32 BUILD_DIR=build32      -> 32-bit reference build, side by side
#
# History, kept because it explains the shape of the code: the port was deliberately built -m32
# for a long time. Decompiled game source assumes the PS1's 32-bit-pointer memory layout in
# places, so a 64-bit build silently shifted struct fields instead of failing loudly, and -m32
# removed that whole class while the real game/backend bugs were being found. Everything that
# actually depended on the narrower width has since been fixed and validated by playing the demo
# battle at both widths -- see exchange/52-stage2-roadmap.md "Phase 2.3". In summary:
#   * PSX `long` is 32-bit; on LP64 it is 64 -> mapped to `int` across the PC-owned PsyQ layer
#     (a `long *` GTE out-param was writing 8 bytes into src/'s 4-byte locals: stack smash).
#   * raw byte offsets into the Object union (`d.bytes[4]`) -> gated field access.
#   * union members that deliberately alias `Object_Sprite.coords` -> gated alignment padding.
#   * the OT link stored a truncated host pointer -> replaced by the token bridge in libgpu.c.
#   * the data-segment generator probed sizeof() at a hardcoded -m32 -> takes the width from here.
# The variable stays overridable precisely so the 32-bit build remains available as a reference
# when a suspected width bug needs an A/B.
M32            ?= -m64

# SANITIZERS. Empty by default; threaded through every compile AND the link (both are required --
# a sanitizer applied to only one produces undefined __asan_* symbols).
#
#   make asan                     -> 64-bit AddressSanitizer build in build_asan/
#   make link SAN="-fsanitize=undefined" BUILD_DIR=build_ubsan
#
# Why this exists: Stage 2.3 turned up a class of bug that no static check can see -- an array
# index that is simply WRONG, where only the *consequence* changes with pointer width (a 3-entry
# caption table read at index 3; see exchange/bugs/bugreport-07.md). ASAN redzones globals and
# heap objects, so it catches those at the faulting instruction instead of one crash per play
# session. Note its blind spot, which that same bug report documents: a buffer that is a slice of
# a larger array (gCardFileBufferPtr = &gScratch1[64]) has no redzone between the slice and the
# rest, so overruns *within* the parent object are invisible to ASAN. Struct-width diffing
# (`gdb -ex "print sizeof(T)"` on build/ vs build32/) is the complementary check, not optional.
SAN            ?=

# OPTIMIZATION. Default is -O0 -g: fast compile + clean backtraces for gdb/ASan/UBSan. The SHIPPED
# release is always -O2 (packaging/make-release.sh uses CC='cc -O2' / CMake -O2), and the 1.5+ hi-res
# rasterizer + 1.6 HD sampler are far heavier at -O0 -- so PERF-TEST WITH `make link OPT=1`, never the
# bare debug build (a bare -O0 build reads as ~3x the CPU / lower fps than what actually ships).
#   make link OPT=1               -> optimized (-O2), representative of the release
# Folded into CC so it reaches every compile AND the link in one place; a command-line CC=... (as the
# release passes) overrides this and composes fine.
ifdef OPT
CC += -O2
endif

# Stage 2.3: the pkg-config search path has to follow the TARGET width, not be pinned to 32-bit.
# Only -m32 needs the explicit lib32 pkgconfig dir; at any other width the system default is
# already correct (and forcing lib32 there yields 32-bit .so paths that fail to link, which is
# how the first -m64 attempt failed). Overridable: `make ... PKG_CONFIG_32=` still works.
ifeq ($(M32),-m32)
PKG_CONFIG_32  := PKG_CONFIG_LIBDIR=/usr/lib32/pkgconfig PKG_CONFIG_PATH=
else
PKG_CONFIG_32  :=
endif

SDL2_CFLAGS    := $(shell $(PKG_CONFIG_32) pkg-config --cflags sdl2)
SDL2_LIBS      := $(shell $(PKG_CONFIG_32) pkg-config --libs sdl2)
OPENAL_CFLAGS  := $(shell $(PKG_CONFIG_32) pkg-config --cflags openal)
OPENAL_LIBS    := $(shell $(PKG_CONFIG_32) pkg-config --libs openal)

# -std=gnu89: restores the same tolerant-of-implicit-declarations C89/K&R
# semantics the real old-GCC-2.x toolchain used (a hard error under any
# modern default standard on GCC 14+, a warning under gnu89 -- see the
# exchange doc's "Traps" section). -fno-builtin-csqrt: our own csqrt(int)
# (a real confirmed GTE symbol) collides with glibc's complex-number
# built-in of the same name; harmless but noisy without this. -DPERMUTER:
# include/include_asm.h emits raw MIPS __asm__(".include ...") directives
# (splat/matching-decomp infrastructure, zero game-logic content) unless
# M2CTX or PERMUTER is defined, in which case INCLUDE_ASM/INCLUDE_RODATA
# become no-ops instead -- an escape hatch the real project already built
# for exactly this "building outside the real MIPS toolchain" scenario, no
# header changes needed. -Dasm(x)=: neutralizes the 2 real
# `register T v asm("regname")` MIPS-register-binding hints (decomp
# artifacts preserving exact original codegen, meaningless off-MIPS) in
# spells/support_magic.c/maps/unpack.c -- doesn't touch the separate __asm__(...)
# token used for real inline-asm blocks elsewhere. -I../..: resolves
# src/*.c's own `#include "assets/NNNNNNNN.inc"` lines (splat's
# already-extracted per-symbol data, generated by the Phase A/B toolchain
# against the real game, not game logic) since those are quoted relative to
# the project root.
# -include pc_forward_decls.h: 2 functions (Movie_GetNextFrame in core/cd.c,
# func_800C4350 in core/graphics.c) are called before their own definition with
# a return type an implicit int-returning forward reference can't match --
# see that header's own comment.
# -DPC_PORT: Stage-2.3 portability gate. Marks src/ edits that make the port
# 64-bit / cross-platform-correct (e.g. per-site NULL-deref guards replacing
# the x86-32-only fault-handler decoder -- see exchange/52-stage2-roadmap.md
# Phase 2.3). Defined ONLY here, so the matching build (top-level Makefile,
# `make check`) never sees these blocks and stays byte-exact. Same discipline
# as -DPERMUTER / -DPC_DEBUG_*: every PC_PORT block must be re-verified with
# `make check`.
# -DPC_FEAT: Stage-3 gameplay/QoL gate (bidirectional ally-cycle, threat overlay,
# ...). New player-facing behaviour, distinct from PC_PORT (correctness) so it's
# greppable on its own. Also PC-build-only; matching build never defines it.
# Warning policy (post-1.6 maintenance): the decompiled game source is 1996 C compiled as-is -- its
# recurring, known-benign warning classes (K&R declarations vs modern prototypes, PSX pointer-width
# idioms) are silenced HERE so they stop drowning the build output. PORT code (the backend rules
# below) instead compiles with $(PORT_WARN) and must stay warning-clean -- a new warning there is
# signal, not noise. Do NOT add game-side suppressions to port code or vice versa.
GAME_WARN      := -Wno-incompatible-pointer-types -Wno-builtin-declaration-mismatch \
                  -Wno-discarded-qualifiers -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast \
                  -Wno-int-conversion
# -Wno-unused-parameter: the PsyQ-API shims keep Sony's exact signatures; unused args are inherent.
PORT_WARN      := -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers
# Region identity rides along in PORT_WARN so EVERY backend rule gets it without editing each
# recipe -- pc_platform.h derives card id / HD pack id / boot LBA from it, and a backend that
# starts using those macros must never silently default to the US values in a JP build.
PORT_WARN      += $(REGION_DEF)
# P5 unified build (exchange/104): UNIFIED=1 compiles the region cores destined for the fat
# binary -- pc_bootstrap.c's four bootstrap constructors compile out (pc_region_main.c calls
# the pieces explicitly), everything else is identical. Single-region `make link` never sets it.
ifeq ($(UNIFIED),1)
PORT_WARN      += -DVH_UNIFIED
endif
# The JP alias defsyms in raw-ld spelling (the blob partial link is ld -r, not cc).
ifeq ($(REGION),jp)
REGION_DEFSYM_LD := --defsym DrawSjisText=DrawText --defsym MsgBox_IsFinished=ConsumeMsgBoxPagePause
else
REGION_DEFSYM_LD :=
endif

GAME_C_FLAGS   := $(M32) $(SAN) -std=gnu89 -fno-builtin-csqrt -DPERMUTER -DPC_PORT $(GAME_FEAT_DEF) $(REGION_DEF) -D'asm(x)=' \
                   -I$(STAGE_DIR) -Iinclude -I$(GAME_ROOT) \
                   -include pc_forward_decls.h -g $(GAME_WARN)

# libwebp: STANDARD port dependency (1.6 HD pack). WebP-compressed replacements are ~7x smaller than raw
# .hdi (a full background pack is ~20MB vs ~150MB), so it ships on by default. NO_WEBP=1 builds without it
# (the engine's `#ifdef VH_HD_WEBP` then falls back to loading only raw .hdi packs) -- for a minimal-deps
# build. The release + AppImage/Windows packaging bundle libwebp (build-appimage.sh / the MinGW DLL).
ifndef NO_WEBP
HD_WEBP_CFLAGS := -DVH_HD_WEBP $(shell pkg-config --cflags libwebp 2>/dev/null)
HD_WEBP_LIBS   := $(shell pkg-config --libs libwebp 2>/dev/null || echo -lwebp)
endif

# libav (ffmpeg): STANDARD port dependency (1.6 HD pack, videos). Decodes the HD-upscaled FMVs (H.264
# mp4) that replace the STR movies. On by default; NO_HDVIDEO=1 builds without it (the engine's
# `#ifdef VH_HD_VIDEO` then keeps the native MDEC movie path). Packaging bundles the av DLLs (Windows).
HD_VIDEO_PKGS  := libavformat libavcodec libswscale libavutil
ifndef NO_HDVIDEO
HD_VIDEO_CFLAGS := -DVH_HD_VIDEO $(shell pkg-config --cflags $(HD_VIDEO_PKGS) 2>/dev/null)
HD_VIDEO_LIBS   := $(shell pkg-config --libs $(HD_VIDEO_PKGS) 2>/dev/null)
endif

.PHONY: default stage clean sweep gen-data link crash-trace asan asan32 ubsan check-shared objs blob unified
default: $(GAME_O_FILES) $(BACKEND_OBJS)

# Generates build/generated_data.c -- real definitions for the global
# variables the matching-decomp data segment never gives a C-level
# defining declaration (see exchange/11-phase-c-data-segment.md). Run
# after `make default`; safe to rerun any time (regenerates from scratch).
#
# The recipe hangs off the generated .c FILE rather than the `gen-data` phony, and
# $(BUILD_DIR)/generated_data.o depends on that file -- so the dependency make enforces is the
# real one (this object is built from that source) instead of an indirection through a phony.
# `default` is phony, so this still regenerates on every invocation, exactly as before.
$(BUILD_DIR)/generated_data.c: default
	@# Stage 2.3: the generator probes real sizeof() values to slice the data segment out of the
	@# ELF, so it MUST run at the same width as this build -- every pointer-containing struct
	@# changes size (Object is 96 bytes at -m32 and 160 at -m64). A width mismatch does not fail
	@# loudly, it silently emits wrongly-sized objects. Pass our own $(M32) rather than letting
	@# the script assume. VH_BUILD_DIR likewise lets a 32- and 64-bit tree coexist.
	VH_TARGET_MARCH='$(M32)' VH_BUILD_DIR='$(BUILD_DIR)' VH_SAN='$(SAN)' \
	VH_GAME_ROOT='$(GAME_ROOT)' VH_PSX_BASENAME='$(PSX_BASENAME)' python3 tools/build_data_segment.py

# Kept as a convenience alias so `make gen-data` still works as documented elsewhere.
gen-data: $(BUILD_DIR)/generated_data.c

# Full link, including the generated data segment.
# AddressSanitizer build, into its own tree so it never clobbers the normal one.
#
#   make asan            # build build_asan/vandalhearts_pc
#   ./run_asan.sh        # run it with the options it needs (see that script)
#
# -fno-omit-frame-pointer: without it ASAN's stack traces are mostly useless here, since the
# gnu89 game source is full of small static functions that otherwise vanish.
# -fsanitize-recover=address: report and CONTINUE instead of aborting on the first hit. That is
# what makes this a sweep rather than a one-bug-per-run loop -- with halt_on_error=0 a single
# playthrough surfaces every distinct site at once. (Reports still name the exact instruction.)
# ⚠️ USE asan32, NOT asan, on x86-64. AddressSanitizer's 64-bit shadow memory starts at
# 0x7fff8000, which swallows the fixed PSX KUSEG RAM arena pc_bootstrap.c reserves at
# 0x80000000 -- the MAP_FIXED_NOREPLACE mmap fails, and then every hardcoded PSX scratch-buffer
# literal baked into decompiled source (`(void *)0x80140878` in src/core/cd.c's gSoundSets et al)
# writes into ASAN's own shadow. The game dies during the first CdRead. There is no ASAN option
# to relocate the shadow under GCC, so a 64-bit ASAN build of this port is simply not usable.
# The 32-bit shadow lives at 0x20000000 instead, leaving 0x80000000 free -- asan32 runs fine.
# This costs nothing in coverage: the bugs ASAN finds (out-of-bounds on globals/heap/stack) are
# width-independent. The width-DEPENDENT class it cannot see -- structs whose size changes at
# LP64 -- is covered by diffing sizeof() between build/ and build32/ under gdb instead.
# Kept buildable anyway, since the failure is a runtime one and worth being able to reproduce.
asan:
	$(MAKE) link BUILD_DIR=build_asan \
	    SAN="-fsanitize=address -fsanitize-recover=address -fno-omit-frame-pointer"

asan32:
	$(MAKE) link BUILD_DIR=build_asan32 M32=-m32 \
	    SAN="-fsanitize=address -fsanitize-recover=address -fno-omit-frame-pointer"

# UndefinedBehaviorSanitizer, bounds check only -- and this one DOES work at 64-bit, which is the
# point of it. UBSan needs no shadow-memory region, so it coexists with the 0x80000000 PSX RAM
# arena that makes 64-bit ASAN impossible (verified: the arena reserves normally and the game runs).
#
# It covers the gap asan32 leaves. asan32 only proves things about logic that is width-independent;
# this instruments array accesses in the ACTUAL 64-bit build. Verified to fire on exactly the
# gClutIds pattern (128 writes into a [124] array) at -m64.
#
# LIMIT, so its silence is not over-read: -fsanitize=bounds only instruments accesses where the
# array bound is statically visible in that translation unit. Anything reached through a pointer
# (gCardFileBufferPtr[...], the gScratch1 slices) is NOT checked -- that is the same blind spot
# asan32 has, for a different reason, and neither tool covers it. Struct-width diffing does.
#
#   make ubsan && UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=0:log_path=ubsan.log \
#       ./build_ubsan/vandalhearts_pc
ubsan:
	$(MAKE) link BUILD_DIR=build_ubsan \
	    SAN="-fsanitize=bounds -fno-omit-frame-pointer"

# Data-segment + reconstruction objects. US links the committed hand-written/checked-in
# reconstructions from src/ (unchanged); JP regenerates every reconstruction from its own
# byte-exact executable into $(GEN_DIR) via the region-parametric generators (exchange/102 P1;
# each generator is validated by regenerating the US file semantically identical first).
DATA_OBJS := $(BUILD_DIR)/generated_data.o
ifeq ($(REGION),jp)
GEN_DIR   := $(BUILD_DIR)/gen
DATA_OBJS += $(BUILD_DIR)/pc_battle_data.o $(BUILD_DIR)/pc_unit_anim_data.o $(BUILD_DIR)/pc_sprite_box_quads.o \
             $(BUILD_DIR)/pc_event_anim_data.o $(BUILD_DIR)/pc_evt_entities.o $(BUILD_DIR)/pc_spell_descriptions.o \
             $(BUILD_DIR)/pc_item_descriptions.o $(BUILD_DIR)/pc_string_table.o

# JP generated sources: region env + output into the build tree (never into src/, whose
# copies are the US originals). VH_PSX_EXE uses the retail copy at the jp root so the
# generators work before the first jp matching build.
$(GEN_DIR)/pc_string_table.c:        tools/gen_string_table.py        ; @mkdir -p $(GEN_DIR); VH_REGION=jp VH_PSX_EXE=$(GAME_ROOT)/SLPM_860.07 VH_GENERATED_OUT=$@ python3 $<
$(GEN_DIR)/pc_spell_descriptions.c:  tools/gen_spell_descriptions.py  ; @mkdir -p $(GEN_DIR); VH_REGION=jp VH_PSX_EXE=$(GAME_ROOT)/SLPM_860.07 VH_GENERATED_OUT=$@ python3 $<
$(GEN_DIR)/pc_item_descriptions.c:   tools/gen_item_descriptions.py   ; @mkdir -p $(GEN_DIR); VH_REGION=jp VH_PSX_EXE=$(GAME_ROOT)/SLPM_860.07 VH_GENERATED_OUT=$@ python3 $<
$(GEN_DIR)/pc_evt_entities.c:        tools/gen_evt_entities.py        ; @mkdir -p $(GEN_DIR); VH_REGION=jp VH_GAME_ROOT=$(GAME_ROOT) VH_PSX_EXE=$(GAME_ROOT)/SLPM_860.07 VH_GENERATED_OUT=$@ python3 $<
$(GEN_DIR)/pc_sprite_box_quads.c:    tools/gen_sprite_box_quads.py    ; @mkdir -p $(GEN_DIR); VH_REGION=jp VH_GAME_ROOT=$(GAME_ROOT) VH_PSX_EXE=$(GAME_ROOT)/SLPM_860.07 VH_GENERATED_OUT=$@ python3 $<
$(GEN_DIR)/pc_battle_data.c:         tools/gen_battle_data.py         ; @mkdir -p $(GEN_DIR); VH_REGION=jp VH_GAME_ROOT=$(GAME_ROOT) VH_PSX_EXE=$(GAME_ROOT)/SLPM_860.07 VH_GENERATED_OUT=$@ python3 $<
$(GEN_DIR)/pc_unit_anim_data.c:      tools/gen_unit_anim_data.py      ; @mkdir -p $(GEN_DIR); VH_REGION=jp VH_GAME_ROOT=$(GAME_ROOT) VH_PSX_EXE=$(GAME_ROOT)/SLPM_860.07 VH_GENERATED_OUT=$@ python3 $<
$(GEN_DIR)/pc_event_anim_data.c:     tools/gen_event_anim_data.py     ; @mkdir -p $(GEN_DIR); VH_REGION=jp VH_GAME_ROOT=$(GAME_ROOT) VH_PSX_EXE=$(GAME_ROOT)/SLPM_860.07 VH_GENERATED_OUT=$@ python3 $<

# JP data objects compile from the generated sources with the same include profile as their
# US counterparts (game headers where the US rule stages them, plain -Iinclude otherwise).
$(BUILD_DIR)/pc_battle_data.o $(BUILD_DIR)/pc_unit_anim_data.o $(BUILD_DIR)/pc_sprite_box_quads.o $(BUILD_DIR)/pc_event_anim_data.o $(BUILD_DIR)/pc_evt_entities.o: $(BUILD_DIR)/%.o: $(GEN_DIR)/%.c | stage
	$(CC) -fno-builtin-csqrt $(M32) $(SAN) -std=gnu89 -DPERMUTER -I$(STAGE_DIR) -Iinclude -MMD -MP -c -o $@ $<
$(BUILD_DIR)/pc_spell_descriptions.o $(BUILD_DIR)/pc_item_descriptions.o $(BUILD_DIR)/pc_string_table.o: $(BUILD_DIR)/%.o: $(GEN_DIR)/%.c
	$(CC) $(M32) $(SAN) -Iinclude -g -MMD -MP -c -o $@ $<
else
DATA_OBJS += $(BUILD_DIR)/pc_battle_data.o $(BUILD_DIR)/pc_unit_anim_data.o $(BUILD_DIR)/pc_sprite_box_quads.o \
             $(BUILD_DIR)/pc_event_anim_data.o $(BUILD_DIR)/pc_evt_entities.o $(BUILD_DIR)/pc_spell_descriptions.o \
             $(BUILD_DIR)/pc_item_descriptions.o $(BUILD_DIR)/pc_string_table.o
endif

# ---- P5 unified binary (exchange/104) ---------------------------------------------------------
# `make unified` builds BOTH region cores (own build dirs -- flags differ, and the stale-flag
# trap is real), partial-links each region's ENTIRE object set into one relocatable blob,
# renames every defined global with a us_/jp_ prefix, and links both blobs under the shared
# pc_region_main.c. Only one core ever executes per process; data-init constructors in both
# blobs run harmlessly at load (their own renamed globals).
objs: default $(DATA_OBJS)

blob: objs
	ld -r -o $(BUILD_DIR)/core_$(REGION)_raw.o $(GAME_O_FILES) $(BACKEND_OBJS) $(DATA_OBJS) $(REGION_DEFSYM_LD)
	@# Rename all DEFINED external symbols (incl. weak + common: a tentative definition left
	@# unrenamed would silently MERGE across the two blobs at the final link). Locals need no
	@# rename (file scope). On COFF, .refptr.* thunks must be filtered -- see exchange/104.
	nm -g --defined-only $(BUILD_DIR)/core_$(REGION)_raw.o | 	    awk '$$3 != "" && $$3 !~ /^\.refptr/ {print $$3" $(REGION)_"$$3}' > $(BUILD_DIR)/core_$(REGION).map
	objcopy --redefine-syms=$(BUILD_DIR)/core_$(REGION).map $(BUILD_DIR)/core_$(REGION)_raw.o $(BUILD_DIR)/core_$(REGION).o

unified:
	$(MAKE) UNIFIED=1 REGION=us BUILD_DIR=build-uni-us blob
	$(MAKE) UNIFIED=1 REGION=jp BUILD_DIR=build-uni-jp blob
	mkdir -p build-uni
	$(CC) $(M32) $(SAN) $(PORT_WARN) -Iinclude -g -c -o build-uni/pc_region_main.o src/pc_region_main.c
	$(CC) $(M32) $(SAN) build-uni/pc_region_main.o build-uni-us/core_us.o build-uni-jp/core_jp.o 	    $(SDL2_LIBS) $(OPENAL_LIBS) -lGL -lm -lpthread $(HD_WEBP_LIBS) $(HD_VIDEO_LIBS) -rdynamic 	    -o build-uni/vandalhearts_pc
	@cp -n vandalhearts.ini build-uni/ 2>/dev/null || true

# `gen-data` is not listed here: $(BUILD_DIR)/generated_data.o already depends on the generated
# source, so the generator still runs, and naming it in both places just duplicated the edge.
link: $(DATA_OBJS)
	$(CC) $(M32) $(SAN) $(GAME_O_FILES) $(BACKEND_OBJS) $(DATA_OBJS) \
	    $(REGION_LDFLAGS) \
	    $(SDL2_LIBS) $(OPENAL_LIBS) -lGL -lm -lpthread $(HD_WEBP_LIBS) $(HD_VIDEO_LIBS) -rdynamic -o $(BUILD_DIR)/vandalhearts_pc
	@cp -n vandalhearts.ini $(BUILD_DIR)/ 2>/dev/null || true   # ship config template (don't clobber an edited copy)

$(BUILD_DIR)/generated_data.o: $(BUILD_DIR)/generated_data.c
	$(CC) -fno-builtin-csqrt $(M32) $(SAN) -std=gnu89 -DPERMUTER -I$(STAGE_DIR) -Iinclude -MMD -MP -c -o $@ $(BUILD_DIR)/generated_data.c

ifneq ($(REGION),jp)  # US data objects come from src/; JP builds its own from $(GEN_DIR) above
# Real, hand-extracted definitions for the flagged pointer globals fixed so far
# (gBattleEnemyUnitInitialStates, gBattlePartyUnitInitialStates -- see the file's own header
# comment and exchange/12-phase-c-bootstrap.md Bug 11). Needs the same staged-include flags as
# generated_data.o since it #includes battle.h and needs the real struct layouts.
$(BUILD_DIR)/pc_battle_data.o: src/pc_battle_data.c | stage
	$(CC) -fno-builtin-csqrt $(M32) $(SAN) -std=gnu89 -DPERMUTER -I$(STAGE_DIR) -Iinclude -MMD -MP -c -o $@ $<

# Real, hand-extracted definition for gUnitAnimSets (3-level pointer structure -- see the
# file's own header comment and exchange/12-phase-c-bootstrap.md Bug 12).
$(BUILD_DIR)/pc_unit_anim_data.o: src/pc_unit_anim_data.c | stage
	$(CC) -fno-builtin-csqrt $(M32) $(SAN) -std=gnu89 -DPERMUTER -I$(STAGE_DIR) -Iinclude -MMD -MP -c -o $@ $<

# Real, hand-extracted definition for gSpriteBoxQuads (single-level -- see the file's own
# header comment and exchange/12-phase-c-bootstrap.md Bug 13).
$(BUILD_DIR)/pc_sprite_box_quads.o: src/pc_sprite_box_quads.c | stage
	$(CC) -fno-builtin-csqrt $(M32) $(SAN) -std=gnu89 -DPERMUTER -I$(STAGE_DIR) -Iinclude -MMD -MP -c -o $@ $<

# Reconstructed cutscene/event unit animation-set pointer tables (gAnimSet_* -- were NULL in the
# data-gen, so cutscene units drew with gfxIdx=0 and were invisible; see the file's own header).
$(BUILD_DIR)/pc_event_anim_data.o: src/pc_event_anim_data.c | stage
	$(CC) -fno-builtin-csqrt $(M32) $(SAN) -std=gnu89 -DPERMUTER -I$(STAGE_DIR) -Iinclude -MMD -MP -c -o $@ $<

endif

# ---- GENERATED game/BIOS-derived data files (gitignored; regenerated at build) ----------------
# These reconstruct copyrighted content (Konami UI text, PS1 BIOS kanji glyphs) from the user's OWN
# copy -- the byte-exact ../../build/SLUS_004.47 and the PsyQ KROMDAT.BIN, both already required by
# the build -- so no copyrighted data is committed to the source tree (same model as
# generated_data.c). Each .c is regenerated when missing or when its generator changes; the ELF must
# exist (produced by the matching build) or the generator errors clearly. See exchange/60.
src/pc_kanji_font.c:         tools/gen_kanji_font.py         ; python3 $<
src/pc_spell_descriptions.c: tools/gen_spell_descriptions.py ; python3 $<
src/pc_item_descriptions.c:  tools/gen_item_descriptions.py  ; python3 $<
src/pc_evt_entities.c:       tools/gen_evt_entities.py       ; python3 $<
src/pc_string_table.c:       tools/gen_string_table.py       ; python3 $<

ifneq ($(REGION),jp)

# Reconstructed gEvtEntities[95] event-entity property tables (were NULL -> invisible cutscene
# units; see the file's own header).
$(BUILD_DIR)/pc_evt_entities.o: src/pc_evt_entities.c | stage
	$(CC) -fno-builtin-csqrt $(M32) $(SAN) -std=gnu89 -DPERMUTER -I$(STAGE_DIR) -Iinclude -MMD -MP -c -o $@ $<

# Reconstructed gSpellDescriptions[72] battle spell/item info strings (were NULL -> blank
# description window; see gen_spell_descriptions.py). Plain data, no game headers.
$(BUILD_DIR)/pc_spell_descriptions.o: src/pc_spell_descriptions.c
	$(CC) $(M32) $(SAN) -Iinclude -g -MMD -MP -c -o $@ $<

# Reconstructed gItemDescriptions/gItemDescriptions2[101] item/equipment info strings (were
# NULL -> blank overworld/shop item-description window; see gen_item_descriptions.py).
$(BUILD_DIR)/pc_item_descriptions.o: src/pc_item_descriptions.c
	$(CC) $(M32) $(SAN) -Iinclude -g -MMD -MP -c -o $@ $<

# Startup fixup for gStringTable (absolute PSX pointers dangle on PC -> blank world-map
# destination names and every "#N" string-table escape; see gen_string_table.py).
$(BUILD_DIR)/pc_string_table.o: src/pc_string_table.c
	$(CC) $(M32) $(SAN) -Iinclude -g -MMD -MP -c -o $@ $<
endif


# Regenerate the symlink staging tree. Order-only prerequisite on the game
# object rule below -- cheap to redo, and must reflect the current state of
# ../../include/*.h (a file could be added/removed there at any time).
stage:
	@rm -rf $(STAGE_DIR)
	@mkdir -p $(STAGE_DIR)
	@for f in ../../include/*.h ../../include/*.inc; do \
		[ -f "$$f" ] && ln -sf "$$(realpath "$$f")" "$(STAGE_DIR)/$$(basename "$$f")"; \
	done
ifeq ($(REGION),jp)
	@# Region headers (exchange/102 "Shared-HEADER rule"): the US copies staged above carry
	@# the PC gate blocks and are stripped-identical for 11 of 15 headers; the 4 genuinely
	@# different ones are overridden -- audio/card/units from jp/include as-is (gate-free,
	@# no gates needed), field.h from the hand-merged variant that keeps the US gates.
	@for f in audio.h card.h units.h; do \
		ln -sf "$$(realpath $(GAME_ROOT)/include/$$f)" "$(STAGE_DIR)/$$f"; \
	done
	@ln -sf "$$(realpath include/region-jp/field.h)" $(STAGE_DIR)/field.h
endif
	@ln -sfn "$$(realpath include/PsyQ)" $(STAGE_DIR)/PsyQ
	@ln -sfn "$$(realpath include/types.h)" $(STAGE_DIR)/types.h
	@ln -sfn "$$(realpath include/inline_gte.h)" $(STAGE_DIR)/inline_gte.h

# Region shared-source guard (exchange/102 §4): proves every TU/header in the shared lists is
# still byte-identical between src/ and jp/src after gate-stripping. Run after any src/ edit
# that touches a shared file; a diverged file must move out of shared_tus.txt.
check-shared:
	python3 tools/check_shared.py

# -MMD -MP emits a .d file listing every header this object depends on, and the -include at the
# bottom of this Makefile feeds them back to make. WITHOUT this the rule depends only on the .c,
# so editing a header rebuilt NOTHING -- which produced a build still containing the OLD code
# while appearing to succeed (bugreport-05: the addPrim macro fix looked like it had failed).
# -MP adds phony targets for the headers so a deleted/renamed header doesn't wedge the build.
ifeq ($(REGION),jp)
# JP region: per-file source selection (exchange/102 §4). Shared TUs compile from the US
# tree (their gated fixes serve both regions -- identity guarded by `make check-shared`);
# the rest from jp/src. Static pattern rules so each object names its real source file.
SHARED_REL := $(shell grep -v '^\#' shared_tus.txt)
JPONLY_REL := $(filter-out $(SHARED_REL),$(GAME_REL))
$(addprefix $(BUILD_DIR)/src/,$(SHARED_REL:.c=.o)): $(BUILD_DIR)/src/%.o: ../../src/%.c | stage
	@mkdir -p $(dir $@)
	$(CC) $(GAME_C_FLAGS) -MMD -MP -c -o $@ $<
$(addprefix $(BUILD_DIR)/src/,$(JPONLY_REL:.c=.o)): $(BUILD_DIR)/src/%.o: $(GAME_ROOT)/src/%.c | stage
	@mkdir -p $(dir $@)
	$(CC) $(GAME_C_FLAGS) -MMD -MP -c -o $@ $<
else
$(BUILD_DIR)/src/%.o: $(SRC_DIR)/%.c | stage
	@mkdir -p $(dir $@)
	$(CC) $(GAME_C_FLAGS) -MMD -MP -c -o $@ $<
endif

# Debug (exchange/30-* study): NO_FADE=1 compiles core/screen_effects.c with the fade overlay
# forced transparent, so the battle scene renders at full brightness from its first frame.
# The matching build never defines this. Toggling requires rebuilding this one object
# (the recipe below rm's it so `make link NO_FADE=1` / plain `make link` always re-pick).
ifeq ($(NO_FADE),1)
$(BUILD_DIR)/src/core/screen_effects.o: GAME_C_FLAGS += -DPC_DEBUG_DISABLE_FADE
endif
$(BUILD_DIR)/src/core/screen_effects.o: FORCE

# Debug (feedback-10 follow-up): NO_LOADING=1 forces the "Now Loading" screen off (core/cd.c) so the
# field shows from the field-enable frame at pitch 128, matching real hardware's loading-suppressed
# capture for a confound-free frame-1 pose comparison. Matching build never defines this.
ifeq ($(NO_LOADING),1)
$(BUILD_DIR)/src/core/cd.o: GAME_C_FLAGS += -DPC_DEBUG_NO_LOADING
endif
$(BUILD_DIR)/src/core/cd.o: FORCE

# Debug (feedback-14 follow-up): SPRITE_LOG=1 instruments RenderUnitSprite (core/object.c) to log each
# unit sprite's cull/projection fate to vh_sprite_fate.csv (when VH_SPRITE_LOG env is set), for the
# matched-pose missing-sprites repro. Matching build never defines this.
ifeq ($(SPRITE_LOG),1)
$(BUILD_DIR)/src/core/object.o: GAME_C_FLAGS += -DPC_DEBUG_SPRITE_LOG
$(BUILD_DIR)/src/events/entities.o: GAME_C_FLAGS += -DPC_DEBUG_SPRITE_LOG
endif

# Stage 2.3 UI-visibility probe. Compiled in UNCONDITIONALLY -- the logging itself is gated at
# runtime by VH_UI_LOG=1, so no make flag is needed and there is no way to run the probe build
# and forget the compile-time switch. (It first went inside the SPRITE_LOG ifeq above by
# mistake, so the hooks silently were not compiled and the probe produced no log at all.)
$(BUILD_DIR)/src/ui/window.o: GAME_C_FLAGS += -DPC_DEBUG_UI_LOG
# NB: object.o also gets -DPC_DEBUG_SPRITE_LOG here (not just on line ~298). GNU Make's SECOND
# target-specific `+=` for the same target appends only to the GLOBAL value, dropping the first
# one -- so an unconditional UI_LOG line here silently clobbered the (conditional) SPRITE_LOG line,
# leaving object.o's SPRITE_LOG hooks (PC_DebugObjPrim4Log, PC_DebugOpaqueGfx) never compiled.
# Both are runtime-gated (VH_OBJPRIM4_LOG / VH_OPAQUE_GFX_LOG), so always-compiling them is free.
$(BUILD_DIR)/src/core/object.o: GAME_C_FLAGS += -DPC_DEBUG_UI_LOG -DPC_DEBUG_SPRITE_LOG
$(BUILD_DIR)/src/states/main_menu.o: GAME_C_FLAGS += -DPC_DEBUG_UI_LOG
$(BUILD_DIR)/src/ui/window.o: FORCE
$(BUILD_DIR)/src/core/object.o: FORCE
$(BUILD_DIR)/src/events/entities.o: FORCE

# Stage 2.3 pathfinding-step probe (exchange/58 battle-5 finding). DIAGNOSTIC-ONLY: it root-caused
# the gTravelAscentCost/DescentCost overrun (now fixed by widening those tables), and the residual
# reports are the confirmed-benign map-boundary reads -- so it is OFF by default now (it printed on
# every steep step = console noise for players). Re-enable by uncommenting the line below to
# re-investigate path/travel logic; the probe stays in-tree, #ifdef-gated. Matching build never
# defines PC_DEBUG_PATH_STEP either way, so byte-exact is unaffected.
# $(BUILD_DIR)/src/battle/path_grids.o: GAME_C_FLAGS += -DPC_DEBUG_PATH_STEP
# $(BUILD_DIR)/src/battle/path_grids.o: FORCE

# Debug (AI targeting study): AI_LOG=1 instruments the AI spell-target scorer (battle/ai.c func_800569A0) to
# log every candidate target's score breakdown -- incl. the gAdvantage type-matchup term (your
# mage->armor doctrine) -- to vh_ai_log.txt (when VH_AI_LOG env is set). Highest score = the target
# the caster picks. For diagnosing "why did that caster choose that target". Matching build never
# defines this (PC_DEBUG_AI_LOG only added under AI_LOG=1), so byte-exact is unaffected.
ifeq ($(AI_LOG),1)
$(BUILD_DIR)/src/battle/ai.o: GAME_C_FLAGS += -DPC_DEBUG_AI_LOG
endif

# Witness pass (decomp-improvement): SPELLFX_LOG=1 logs every gSpellsEx FX dispatch (spell,
# slot, handler symbol) to vh_spellfx_log.txt + stderr, runtime-gated on VH_SPELLFX_LOG.
# Matching build never defines this (only added under SPELLFX_LOG=1), byte-exact unaffected.
ifeq ($(SPELLFX_LOG),1)
$(BUILD_DIR)/src/battle/executors.o: GAME_C_FLAGS += -DPC_DEBUG_SPELLFX_LOG
endif
$(BUILD_DIR)/src/battle/executors.o: FORCE
$(BUILD_DIR)/src/battle/ai.o: FORCE

# Debug (epilogue-ocarina-silent bug): AUDIO_CMD_LOG=1 logs every PerformAudioCommand (core/audio.c) to
# vh_audio_cmd.csv (when VH_AUDIO_LOG env is set) -- to confirm whether the ending's PLAY_XA(102)
# (0x366) is ever issued. Matching build never defines PC_DEBUG_AUDIO_LOG, so byte-exact holds.
ifeq ($(AUDIO_CMD_LOG),1)
$(BUILD_DIR)/src/core/audio.o: GAME_C_FLAGS += -DPC_DEBUG_AUDIO_LOG
endif
$(BUILD_DIR)/src/core/audio.o: FORCE

# Debug (otz-overflow / black-terrain study): TERRAIN_LOG=1 instruments RenderMapTile /
# RenderEdgeMapTile (core/graphics.c) to accumulate per-frame terrain otz stats + black-tile counts
# into vh_terrain_otz_pc.csv (when VH_TERRAIN_LOG env is set). Matching build never defines this.
ifeq ($(TERRAIN_LOG),1)
$(BUILD_DIR)/src/core/graphics.o: GAME_C_FLAGS += -DPC_DEBUG_TERRAIN_LOG
endif
$(BUILD_DIR)/src/core/graphics.o: FORCE
FORCE:

# -I$(STAGE_DIR) -DPERMUTER: the camera-target-trace diagnostic (2026-07-12) needs the real
# State/Object/graphics struct layouts (gState, gObjectArray, gCameraPos/Rotation/Zoom) --
# same staged-include reasoning as libsnd.c above. -Iinclude first so libetc.c's own top-level
# `#include "PsyQ/libetc.h"` still resolves to this platform's own shim, not the real one.
# -DPERMUTER: common.h's chain pulls in include_asm.h, whose INCLUDE_ASM/INCLUDE_RODATA/
# labels.inc __asm__ blocks are only skipped under PERMUTER (or M2CTX) -- same reason
# pc_battle_data.c/pc_sprite_box_quads.c/libsnd.c already define it.
$(BUILD_DIR)/libetc.o: src/libetc.c | stage
	$(CC) $(M32) $(SAN) $(PORT_WARN) -fno-builtin-csqrt -DPERMUTER -Iinclude -I$(STAGE_DIR) $(SDL2_CFLAGS) -g -MMD -MP -c -o $@ $<

# Extracted from libetc.c -- same staged-include profile (they read the game's state/object headers).
$(BUILD_DIR)/pc_diag.o: src/pc_diag.c | stage
	$(CC) $(M32) $(SAN) $(PORT_WARN) -fno-builtin-csqrt -DPERMUTER -Iinclude -I$(STAGE_DIR) $(SDL2_CFLAGS) -g -MMD -MP -c -o $@ $<

$(BUILD_DIR)/pc_battle_speed.o: src/pc_battle_speed.c | stage
	$(CC) $(M32) $(SAN) $(PORT_WARN) -fno-builtin-csqrt -DPERMUTER -Iinclude -I$(STAGE_DIR) $(SDL2_CFLAGS) -g -MMD -MP -c -o $@ $<

$(BUILD_DIR)/libcd.o: src/libcd.c
	$(CC) $(M32) $(SAN) $(PORT_WARN) -Iinclude -g -MMD -MP -c -o $@ $<

# -Ibuild/include_stage + -DPERMUTER: libsnd.c reaches into core/cd.c's real
# gCdFiles/gVabLoader globals (see the SsVabTransBodyPartly comment), which
# pulls in the real project's cd_files.h -> common.h chain -- needs the
# same include-shadowing fix and asm-escape-hatch as real game source (see
# GAME_C_FLAGS above), but NOT -std=gnu89 (this is our own file, written in
# ordinary modern C, not K&R-compatible game source).
$(BUILD_DIR)/libsnd.o: src/libsnd.c | stage
	$(CC) $(M32) $(SAN) $(PORT_WARN) -fno-builtin-csqrt -DPERMUTER -Iinclude -I$(STAGE_DIR) $(OPENAL_CFLAGS) -g -MMD -MP -c -o $@ $<

$(BUILD_DIR)/libspu.o: src/libspu.c
	$(CC) $(M32) $(SAN) $(PORT_WARN) -Iinclude -g -MMD -MP -c -o $@ $<

$(BUILD_DIR)/libkernel.o: src/libkernel.c
	$(CC) $(M32) $(SAN) $(PORT_WARN) -Iinclude -g -MMD -MP -c -o $@ $<

$(BUILD_DIR)/libgte.o: src/libgte.c
	$(CC) $(M32) $(SAN) $(PORT_WARN) -Iinclude -fno-builtin -g -MMD -MP -c -o $@ $<

$(BUILD_DIR)/libgpu.o: src/libgpu.c
	$(CC) $(M32) $(SAN) $(PORT_WARN) -Iinclude -g -MMD -MP -c -o $@ $<

$(BUILD_DIR)/pc_gpu_trace.o: src/pc_gpu_trace.c
	$(CC) $(M32) $(SAN) $(PORT_WARN) -Iinclude -g -MMD -MP -c -o $@ $<

# Optional language-pack loader + its UTF-8 font engine (inert without a pack).
$(BUILD_DIR)/pc_lang.o: src/pc_lang.c
	$(CC) $(M32) $(SAN) $(PORT_WARN) -Iinclude -g -MMD -MP -c -o $@ $<

# Pack ENUMERATION -- both regions (the JP overlay lists packs for a pending US-disc restart).
$(BUILD_DIR)/pc_lang_list.o: src/pc_lang_list.c
	$(CC) $(M32) $(SAN) $(PORT_WARN) -Iinclude -g -MMD -MP -c -o $@ $<

# JP region only: the langpack engine's no-pack behaviour as stubs (see the file header).
$(BUILD_DIR)/pc_lang_stub.o: src/pc_lang_stub.c
	$(CC) $(M32) $(SAN) $(PORT_WARN) -Iinclude -g -MMD -MP -c -o $@ $<

$(BUILD_DIR)/pc_lang_font.o: src/pc_lang_font.c
	$(CC) $(M32) $(SAN) $(PORT_WARN) -Iinclude -g -MMD -MP -c -o $@ $<

# The hot per-pixel path (DDA spans, texture sampling, blending) -- same flags as libgpu.o had.
$(BUILD_DIR)/pc_raster.o: src/pc_raster.c
	$(CC) $(M32) $(SAN) $(PORT_WARN) -Iinclude -g -MMD -MP -c -o $@ $<

$(BUILD_DIR)/pc_hdpack.o: src/pc_hdpack.c
	$(CC) $(M32) $(SAN) $(PORT_WARN) -Iinclude $(HD_WEBP_CFLAGS) -g -MMD -MP -c -o $@ $<

$(BUILD_DIR)/pc_gpu_window.o: src/pc_gpu_window.c
	$(CC) $(M32) $(SAN) $(PORT_WARN) -Iinclude $(SDL2_CFLAGS) -g -MMD -MP -c -o $@ $<

# Stage-3 (1.1C) in-game options overlay model + its RETURN TO TITLE action. The action (moved
# here from pc_balance.c, 2026-08-22) rewrites engine state (gState etc.), so the overlay now
# compiles through the staged game headers with the same gnu89+PERMUTER profile as pc_balance.o.
$(BUILD_DIR)/pc_overlay.o: src/pc_overlay.c | stage
	$(CC) $(M32) $(SAN) $(PORT_WARN) -fno-builtin-csqrt -std=gnu89 -DPERMUTER -I$(STAGE_DIR) -Iinclude -MMD -MP -c -o $@ $<

# Stage-3 (1.2b) save-management file ops -- pure C, no SDL/game headers.
$(BUILD_DIR)/pc_saves.o: src/pc_saves.c
	$(CC) $(M32) $(SAN) $(PORT_WARN) -Iinclude -g -MMD -MP -c -o $@ $<

# Langpack F3 movie subtitle cues -- pure C, no SDL/game headers.
$(BUILD_DIR)/pc_movie_subs.o: src/pc_movie_subs.c
	$(CC) $(M32) $(SAN) $(PORT_WARN) -Iinclude -g -MMD -MP -c -o $@ $<

# Stage-3 (1.3) Tactical-mode balance package -- patches mutable game tables (gUnitInfo, etc.), so it
# needs the real struct layouts via the staged include chain, same flags as the pc data files above.
$(BUILD_DIR)/pc_balance.o: src/pc_balance.c | stage
	$(CC) $(M32) $(SAN) $(PORT_WARN) -fno-builtin-csqrt -std=gnu89 -DPERMUTER -I$(STAGE_DIR) -Iinclude -MMD -MP -c -o $@ $<

$(BUILD_DIR)/libsn.o: src/libsn.c
	$(CC) $(M32) $(SAN) $(PORT_WARN) -Iinclude -g -MMD -MP -c -o $@ $<

$(BUILD_DIR)/pc_bootstrap.o: src/pc_bootstrap.c
	$(CC) $(M32) $(SAN) $(PORT_WARN) -Iinclude -g -MMD -MP -c -o $@ $<

$(BUILD_DIR)/pc_xa.o: src/pc_xa.c
	$(CC) $(M32) $(SAN) $(PORT_WARN) -Iinclude -g -MMD -MP -c -o $@ $<

# Software SPU voice renderer (Phase 2.1) -- OpenAL streaming sink like pc_xa.o.
$(BUILD_DIR)/pc_spu.o: src/pc_spu.c
	$(CC) $(M32) $(SAN) $(PORT_WARN) -Iinclude -g -MMD -MP -c -o $@ $<

$(BUILD_DIR)/pc_mdec.o: src/pc_mdec.c
	$(CC) $(M32) $(SAN) $(PORT_WARN) -Iinclude -g -MMD -MP -c -o $@ $<

# HD FMV decoder (1.6): libav H.264 -> RGB, replacing the STR movie picture. libav flags applied here.
$(BUILD_DIR)/pc_hdvideo.o: src/pc_hdvideo.c
	$(CC) $(M32) $(SAN) $(PORT_WARN) -Iinclude $(HD_VIDEO_CFLAGS) -g -MMD -MP -c -o $@ $<

# Generated PS1 BIOS kanji charset-2 glyphs (see tools/gen_kanji_font.py) used
# by Krom2RawAdd in libkernel.c -- pure data, no game headers.
# Region-dependent CONTENT (exchange/102 P3): US embeds the audited 209-glyph subset from
# src/ (unchanged); JP regenerates the FULL charset-2+3 ROM font (3489 glyphs) into GEN_DIR,
# since the JP game draws its entire text repertoire through Krom2RawAdd.
ifeq ($(REGION),jp)
$(GEN_DIR)/pc_kanji_font.c: tools/gen_kanji_font.py
	@mkdir -p $(GEN_DIR); VH_REGION=jp VH_GENERATED_OUT=$@ python3 $<
$(BUILD_DIR)/pc_kanji_font.o: $(GEN_DIR)/pc_kanji_font.c
	$(CC) $(M32) $(SAN) $(PORT_WARN) -Iinclude -g -MMD -MP -c -o $@ $<
else
$(BUILD_DIR)/pc_kanji_font.o: src/pc_kanji_font.c
	$(CC) $(M32) $(SAN) $(PORT_WARN) -Iinclude -g -MMD -MP -c -o $@ $<
endif

# Compiles every src/*.c independently and reports a pass/fail summary
# without stopping at the first error -- the fast way to see the whole
# picture during iteration, as opposed to `make default`'s normal
# stop-on-first-error behavior.
sweep: stage
	@ok=0; fail=0; failed_files=""; \
	mkdir -p $(BUILD_DIR)/src; \
	for f in $(SRC_DIR)/*.c $(SRC_DIR)/*/*.c; do \
		name=$$(echo $${f#$(SRC_DIR)/} | tr / _); \
		if $(CC) $(GAME_C_FLAGS) -c -o $(BUILD_DIR)/src/$$name.o $$f 2>$(BUILD_DIR)/src/$$name.log; then \
			ok=$$((ok+1)); \
		else \
			fail=$$((fail+1)); failed_files="$$failed_files $$name"; \
		fi; \
	done; \
	echo "$$ok/$$((ok+fail)) files compiled"; \
	if [ $$fail -gt 0 ]; then echo "FAILED:$$failed_files"; fi

clean:
	rm -rf $(BUILD_DIR)/src $(BUILD_DIR)/*.o $(STAGE_DIR)

# Resolve the game-code sites in vh_null_reads.log (the NULL-read fixup handler's list) to file:line.
# Run the game cap-less, hit some battles, then: `make crash-trace`. Each logged site is `SYMBOL+0xOFF`;
# we look SYMBOL up in the (link-time, addr2line-compatible) symbol table and add the offset.
crash-trace:
	@test -f vh_null_reads.log || { echo "no vh_null_reads.log yet (run the game first)"; exit 0; }
	@grep -oE '[A-Za-z_][A-Za-z0-9_]*\+0x[0-9a-f]+' vh_null_reads.log | sort -u | \
	  grep -vE '^(PC_SigCrash|PC_LogNullRead|PC_DumpDiag)\+' | \
	  while IFS='+' read sym off; do \
	    base=$$(nm $(BUILD_DIR)/vandalhearts_pc 2>/dev/null | awk -v s="$$sym" '$$3==s {print $$1; exit}'); \
	    if [ -n "$$base" ]; then \
	      addr=$$(printf '0x%x' $$((0x$$base + $$off))); \
	      printf '%-32s -> ' "$$sym+$$off"; addr2line -e $(BUILD_DIR)/vandalhearts_pc $$addr; \
	    else printf '%-32s -> (symbol not found)\n' "$$sym+$$off"; fi; \
	  done

# Header-dependency files emitted by -MMD above (see the pattern rule's comment). Must be last:
# make merges these in as extra prerequisites so a header edit rebuilds exactly what includes it.
-include $(BUILD_DIR)/src/*.d $(BUILD_DIR)/src/*/*.d $(BUILD_DIR)/*.d
