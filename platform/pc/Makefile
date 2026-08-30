# PC-port build: compiles the game source (../../src or ../../jp/src) against the platform/pc
# backends with a host compiler. GAME_C_FLAGS uses the include_stage symlink tree, not
# -I../../include. See docs/building.md, "How the port build works".

# REGION selects the game-code tree: `make link` (us, build/) or `make link REGION=jp`
# (SLPM-86007, build-jp/). Both regions compile the shared TUs from the US tree.
# See docs/building.md, "Region source selection".
REGION         ?= us
ifeq ($(REGION),jp)
GAME_ROOT      := ../../jp
BUILD_DIR      := build-jp
PSX_BASENAME   := SLPM_860.07
GAME_FEAT_DEF  := -DPC_FEAT
REGION_DEF     := -DVH_REGION_JP
# The JP binary carries these two names for one address each (undefined_additional.txt):
# the same function under a second name, satisfied at link time.
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

# The file inventory comes from the region's own tree (identical 88-file layout on both sides);
# which tree each file is compiled from is decided per file in the game object rules below.
SRC_DIR        := ../../src
GAME_REL       := $(patsubst $(GAME_ROOT)/src/%,%,$(wildcard $(GAME_ROOT)/src/*.c $(GAME_ROOT)/src/*/*.c))
GAME_O_FILES   := $(addprefix $(BUILD_DIR)/src/,$(GAME_REL:.c=.o))

BACKEND_SRCS   := libetc.c pc_lang.c pc_lang_list.c pc_lang_font.c pc_diag.c pc_battle_speed.c libcd.c libsnd.c libspu.c libkernel.c libgte.c libgpu.c pc_raster.c pc_gpu_trace.c pc_hdpack.c pc_gpu_window.c libsn.c pc_bootstrap.c pc_xa.c pc_mdec.c pc_kanji_font.c pc_spu.c pc_overlay.c pc_saves.c pc_movie_subs.c pc_balance.c pc_hdvideo.c
ifeq ($(REGION),jp)
# The langpack engine is US-only by design (built on the US ASCII text path); the JP core
# links pc_lang_stub.c (the engine's documented no-pack behaviour) instead.
BACKEND_SRCS   := $(filter-out pc_lang.c pc_lang_font.c,$(BACKEND_SRCS)) pc_lang_stub.c
endif
BACKEND_OBJS   := $(patsubst %.c,$(BUILD_DIR)/%.o,$(BACKEND_SRCS))

# TARGET WIDTH: 64-bit by default; `make link M32=-m32 BUILD_DIR=build32` builds the 32-bit
# reference side by side. The 32-bit build is the A/B oracle for width bugs (struct layouts
# that shift silently at LP64). See docs/memory-safety.md, "Going 64-bit".
M32            ?= -m64

# SANITIZERS. Empty by default; threaded through every compile AND the link (a sanitizer applied
# to only one leaves undefined __asan_* symbols). `make asan32` / `make ubsan` below set it.
# See docs/memory-safety.md, "AddressSanitizer" and "UBSan".
SAN            ?=

# OPTIMIZATION. Default -O0 -g (clean backtraces); the shipped release is always -O2, and the
# hi-res rasterizer is ~3x heavier at -O0, so perf-test with `make link OPT=1`. Folded into CC so
# it reaches every compile and the link. See docs/building.md, "Optimization".
ifdef OPT
CC += -O2
endif

# The pkg-config search path follows the TARGET width: only -m32 needs the explicit lib32
# pkgconfig dir; forcing it at -m64 yields 32-bit .so paths that fail to link.
# Overridable: `make ... PKG_CONFIG_32=`.
ifeq ($(M32),-m32)
PKG_CONFIG_32  := PKG_CONFIG_LIBDIR=/usr/lib32/pkgconfig PKG_CONFIG_PATH=
else
PKG_CONFIG_32  :=
endif

SDL2_CFLAGS    := $(shell $(PKG_CONFIG_32) pkg-config --cflags sdl2)
SDL2_LIBS      := $(shell $(PKG_CONFIG_32) pkg-config --libs sdl2)
OPENAL_CFLAGS  := $(shell $(PKG_CONFIG_32) pkg-config --cflags openal)
OPENAL_LIBS    := $(shell $(PKG_CONFIG_32) pkg-config --libs openal)

# Game-source profile: -std=gnu89, -fno-builtin-csqrt, -DPERMUTER, -D'asm(x)=', -I../.. and
# -include pc_forward_decls.h are each explained in docs/building.md, "Game-source compile
# flags"; the PC_PORT/PC_FEAT gates are defined ONLY here, never by the matching build.

# Warning policy: the decompiled 1996 C gets its known-benign warning classes silenced here;
# port code compiles with $(PORT_WARN) and must stay warning-clean (a new warning is signal).
# Never add game-side suppressions to port code or vice versa.
GAME_WARN      := -Wno-incompatible-pointer-types -Wno-builtin-declaration-mismatch \
                  -Wno-discarded-qualifiers -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast \
                  -Wno-int-conversion
# -Wno-unused-parameter: the PsyQ-API shims keep Sony's exact signatures; unused args are inherent.
PORT_WARN      := -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers
# Region identity rides along in PORT_WARN so EVERY backend rule gets it: pc_platform.h derives
# card id / HD pack id / boot LBA from it, and a backend using those macros must never silently
# default to the US values in a JP build.
PORT_WARN      += $(REGION_DEF)
# UNIFIED=1 compiles a region core for the all-regions binary: pc_bootstrap.c's bootstrap
# constructors compile out (pc_region_main.c calls the pieces explicitly). Single-region
# `make link` never sets it. See docs/building.md, "The unified binary".
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

# libwebp: standard dependency for HD-pack backgrounds (.webp is ~7x smaller than raw .hdi, ~20MB
# vs ~150MB per pack). NO_WEBP=1 builds without it; the engine's `#ifdef VH_HD_WEBP` then loads
# only raw .hdi packs. Packaging bundles libwebp (build-appimage.sh / the MinGW DLL).
ifndef NO_WEBP
HD_WEBP_CFLAGS := -DVH_HD_WEBP $(shell pkg-config --cflags libwebp 2>/dev/null)
HD_WEBP_LIBS   := $(shell pkg-config --libs libwebp 2>/dev/null || echo -lwebp)
endif

# libav (ffmpeg): standard dependency for HD-pack movies (H.264 mp4 replacing the STR FMVs).
# NO_HDVIDEO=1 builds without it; the engine's `#ifdef VH_HD_VIDEO` then keeps the native MDEC
# path. Releases link a minimal static libav (docs/cross-platform.md).
HD_VIDEO_PKGS  := libavformat libavcodec libswscale libavutil
ifndef NO_HDVIDEO
HD_VIDEO_CFLAGS := -DVH_HD_VIDEO $(shell pkg-config --cflags $(HD_VIDEO_PKGS) 2>/dev/null)
HD_VIDEO_LIBS   := $(shell pkg-config --libs $(HD_VIDEO_PKGS) 2>/dev/null)
endif

.PHONY: default stage clean sweep gen-data link crash-trace asan asan32 ubsan check-shared check-comments objs blob unified
default: $(GAME_O_FILES) $(BACKEND_OBJS)

# Generates build/generated_data.c: definitions for the globals the matching-decomp data segment
# never declares at C level (docs/pc-port/data-segment.md). Hangs off the generated FILE so
# generated_data.o has a real prerequisite; `default` is phony, so it regenerates every run.
$(BUILD_DIR)/generated_data.c: default
	@# Stage 2.3: the generator probes real sizeof() values to slice the data segment out of the
	@# ELF, so it MUST run at the same width as this build -- every pointer-containing struct
	@# changes size (Object is 96 bytes at -m32 and 160 at -m64). A width mismatch does not fail
	@# loudly, it silently emits wrongly-sized objects. Pass our own $(M32) rather than letting
	@# the script assume. VH_BUILD_DIR likewise lets a 32- and 64-bit tree coexist.
	VH_TARGET_MARCH='$(M32)' VH_BUILD_DIR='$(BUILD_DIR)' VH_SAN='$(SAN)' \
	VH_GAME_ROOT='$(GAME_ROOT)' VH_PSX_BASENAME='$(PSX_BASENAME)' python3 tools/build_data_segment.py

# Convenience alias for the generated file.
gen-data: $(BUILD_DIR)/generated_data.c

# AddressSanitizer builds (run with ./run_asan.sh). USE asan32 on x86-64: the 64-bit ASan shadow
# (0x7fff8000) swallows the PSX RAM arena at 0x80000000 and the game dies in the first CdRead;
# the 32-bit shadow is at 0x20000000. See docs/memory-safety.md, "AddressSanitizer".
asan:
	$(MAKE) link BUILD_DIR=build_asan \
	    SAN="-fsanitize=address -fsanitize-recover=address -fno-omit-frame-pointer"

asan32:
	$(MAKE) link BUILD_DIR=build_asan32 M32=-m32 \
	    SAN="-fsanitize=address -fsanitize-recover=address -fno-omit-frame-pointer"

# UBSan bounds check (run with ./run_ubsan.sh). Needs no shadow memory, so it works at the
# shipped 64-bit width; it only instruments statically-sized array accesses.
# See docs/memory-safety.md, "UBSan".
ubsan:
	$(MAKE) link BUILD_DIR=build_ubsan \
	    SAN="-fsanitize=bounds -fno-omit-frame-pointer"

# Data-segment + reconstruction objects. US links the reconstructions in src/; JP regenerates
# every reconstruction from its own byte-exact executable into $(GEN_DIR) with the
# region-parametric generators.
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

# ---- unified binary: `make unified` builds both region cores in their own build dirs, ---------
# partial-links each into one relocatable blob with every defined global prefixed us_/jp_, and
# links both under pc_region_main.c. See docs/building.md, "The unified binary".
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

# `gen-data` is not a prerequisite here: $(BUILD_DIR)/generated_data.o already depends on the
# generated source, so the generator still runs.
link: $(DATA_OBJS)
	$(CC) $(M32) $(SAN) $(GAME_O_FILES) $(BACKEND_OBJS) $(DATA_OBJS) \
	    $(REGION_LDFLAGS) \
	    $(SDL2_LIBS) $(OPENAL_LIBS) -lGL -lm -lpthread $(HD_WEBP_LIBS) $(HD_VIDEO_LIBS) -rdynamic -o $(BUILD_DIR)/vandalhearts_pc
	@cp -n vandalhearts.ini $(BUILD_DIR)/ 2>/dev/null || true   # ship config template (don't clobber an edited copy)

$(BUILD_DIR)/generated_data.o: $(BUILD_DIR)/generated_data.c
	$(CC) -fno-builtin-csqrt $(M32) $(SAN) -std=gnu89 -DPERMUTER -I$(STAGE_DIR) -Iinclude -MMD -MP -c -o $@ $(BUILD_DIR)/generated_data.c

ifneq ($(REGION),jp)  # US data objects come from src/; JP builds its own from $(GEN_DIR) above
# Reconstructed pointer globals (gBattleEnemyUnitInitialStates, gBattlePartyUnitInitialStates;
# see the file header). Needs the staged-include flags of generated_data.o: it includes battle.h
# for the real struct layouts.
$(BUILD_DIR)/pc_battle_data.o: src/pc_battle_data.c | stage
	$(CC) -fno-builtin-csqrt $(M32) $(SAN) -std=gnu89 -DPERMUTER -I$(STAGE_DIR) -Iinclude -MMD -MP -c -o $@ $<

# Reconstructed gUnitAnimSets (a 3-level pointer structure; see the file header).
$(BUILD_DIR)/pc_unit_anim_data.o: src/pc_unit_anim_data.c | stage
	$(CC) -fno-builtin-csqrt $(M32) $(SAN) -std=gnu89 -DPERMUTER -I$(STAGE_DIR) -Iinclude -MMD -MP -c -o $@ $<

# Reconstructed gSpriteBoxQuads (single-level; see the file header).
$(BUILD_DIR)/pc_sprite_box_quads.o: src/pc_sprite_box_quads.c | stage
	$(CC) -fno-builtin-csqrt $(M32) $(SAN) -std=gnu89 -DPERMUTER -I$(STAGE_DIR) -Iinclude -MMD -MP -c -o $@ $<

# Reconstructed cutscene/event unit animation-set pointer tables (gAnimSet_*): the data-gen
# leaves them NULL, which draws cutscene units with gfxIdx=0, invisible (see the file header).
$(BUILD_DIR)/pc_event_anim_data.o: src/pc_event_anim_data.c | stage
	$(CC) -fno-builtin-csqrt $(M32) $(SAN) -std=gnu89 -DPERMUTER -I$(STAGE_DIR) -Iinclude -MMD -MP -c -o $@ $<

endif

# GENERATED game/BIOS-derived data files (gitignored): they reconstruct copyrighted content
# (Konami UI text, PS1 BIOS kanji glyphs) from the user's own byte-exact SLUS_004.47 + PsyQ
# KROMDAT.BIN, so none is committed. Regenerated when missing or when the generator changes.
src/pc_kanji_font.c:         tools/gen_kanji_font.py         ; python3 $<
src/pc_spell_descriptions.c: tools/gen_spell_descriptions.py ; python3 $<
src/pc_item_descriptions.c:  tools/gen_item_descriptions.py  ; python3 $<
src/pc_evt_entities.c:       tools/gen_evt_entities.py       ; python3 $<
src/pc_string_table.c:       tools/gen_string_table.py       ; python3 $<

ifneq ($(REGION),jp)

# Reconstructed gEvtEntities[95] event-entity property tables (NULL in the data-gen -> invisible
# cutscene units; see the file header).
$(BUILD_DIR)/pc_evt_entities.o: src/pc_evt_entities.c | stage
	$(CC) -fno-builtin-csqrt $(M32) $(SAN) -std=gnu89 -DPERMUTER -I$(STAGE_DIR) -Iinclude -MMD -MP -c -o $@ $<

# Reconstructed gSpellDescriptions[72] battle spell/item info strings (NULL in the data-gen ->
# blank description window; see gen_spell_descriptions.py). Plain data, no game headers.
$(BUILD_DIR)/pc_spell_descriptions.o: src/pc_spell_descriptions.c
	$(CC) $(M32) $(SAN) -Iinclude -g -MMD -MP -c -o $@ $<

# Reconstructed gItemDescriptions/gItemDescriptions2[101] item/equipment info strings (NULL in
# the data-gen -> blank overworld/shop item-description window; see gen_item_descriptions.py).
$(BUILD_DIR)/pc_item_descriptions.o: src/pc_item_descriptions.c
	$(CC) $(M32) $(SAN) -Iinclude -g -MMD -MP -c -o $@ $<

# Startup fixup for gStringTable (absolute PSX pointers dangle on PC -> blank world-map
# destination names and every "#N" string-table escape; see gen_string_table.py).
$(BUILD_DIR)/pc_string_table.o: src/pc_string_table.c
	$(CC) $(M32) $(SAN) -Iinclude -g -MMD -MP -c -o $@ $<
endif


# Regenerate the symlink staging tree (docs/building.md, "The header-staging tree"). Order-only
# prerequisite of every staged compile: cheap, and it must reflect the current ../../include/*.h.
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

# Region shared-source guard: proves every TU/header in the shared lists is still byte-identical
# between src/ and jp/src after gate-stripping. Run after any src/ edit that touches a shared
# file; a diverged file must move out of shared_tus.txt.
check-shared:
	python3 tools/check_shared.py

# Comment hygiene: no comment run over 3 lines, no project-log vocabulary (dates, local paths,
# stage/version markers, past-tense narration) in any tracked file's comments.
check-comments:
	python3 tools/comment_hygiene.py
	python3 tools/check_doc_pointers.py


# -MMD -MP emits a .d file per object (fed back by the -include at the bottom) so a header edit
# rebuilds what includes it; -MP keeps a deleted header from wedging the build.
# See docs/building.md, "Header dependency tracking".
ifeq ($(REGION),jp)
# JP region: shared TUs compile from the US tree (identity guarded by `make check-shared`), the
# rest from jp/src. Static pattern rules so each object names its real source file.
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

# Debug: NO_FADE=1 compiles core/screen_effects.c with the fade overlay forced transparent, so
# the battle scene renders at full brightness from its first frame. The matching build never
# defines this; the FORCE prerequisite makes each `make link` re-pick the setting.
ifeq ($(NO_FADE),1)
$(BUILD_DIR)/src/core/screen_effects.o: GAME_C_FLAGS += -DPC_DEBUG_DISABLE_FADE
endif
$(BUILD_DIR)/src/core/screen_effects.o: FORCE

# Debug: NO_LOADING=1 forces the "Now Loading" screen off (core/cd.c) so the field shows from the
# field-enable frame at pitch 128, matching a loading-suppressed hardware capture for a
# confound-free frame-1 pose comparison. Matching build never defines this.
ifeq ($(NO_LOADING),1)
$(BUILD_DIR)/src/core/cd.o: GAME_C_FLAGS += -DPC_DEBUG_NO_LOADING
endif
$(BUILD_DIR)/src/core/cd.o: FORCE

# Debug: SPRITE_LOG=1 instruments RenderUnitSprite (core/object.c) to log each unit sprite's
# cull/projection fate to vh_sprite_fate.csv (when VH_SPRITE_LOG is set), for diagnosing missing
# sprites against a matched-pose hardware capture. Matching build never defines this.
ifeq ($(SPRITE_LOG),1)
$(BUILD_DIR)/src/core/object.o: GAME_C_FLAGS += -DPC_DEBUG_SPRITE_LOG
$(BUILD_DIR)/src/events/entities.o: GAME_C_FLAGS += -DPC_DEBUG_SPRITE_LOG
endif

# UI-visibility probe, compiled in unconditionally: the logging is gated at runtime by
# VH_UI_LOG=1, so there is no compile-time switch to forget.
$(BUILD_DIR)/src/ui/window.o: GAME_C_FLAGS += -DPC_DEBUG_UI_LOG
# object.o gets SPRITE_LOG here too: a SECOND target-specific `+=` for one target appends only to
# the global value and drops the first, so a separate line would clobber the conditional one
# above. Both hooks are runtime-gated (VH_OBJPRIM4_LOG / VH_OPAQUE_GFX_LOG), so this is free.
$(BUILD_DIR)/src/core/object.o: GAME_C_FLAGS += -DPC_DEBUG_UI_LOG -DPC_DEBUG_SPRITE_LOG
$(BUILD_DIR)/src/states/main_menu.o: GAME_C_FLAGS += -DPC_DEBUG_UI_LOG
$(BUILD_DIR)/src/ui/window.o: FORCE
$(BUILD_DIR)/src/core/object.o: FORCE
$(BUILD_DIR)/src/events/entities.o: FORCE

# Diagnostic-only pathfinding-step probe (battle/path_grids.c, PC_DEBUG_PATH_STEP), off by
# default: its remaining reports are benign map-boundary reads that print on every steep step.
# Enable with `GAME_C_FLAGS += -DPC_DEBUG_PATH_STEP` (+ FORCE) on that object to re-investigate.

# Debug: AI_LOG=1 instruments the AI spell-target scorer (battle/ai.c func_800569A0) to log every
# candidate's score breakdown, incl. the gAdvantage type-matchup term, to vh_ai_log.txt (when
# VH_AI_LOG is set). Highest score = the caster's pick. Matching build never defines this.
ifeq ($(AI_LOG),1)
$(BUILD_DIR)/src/battle/ai.o: GAME_C_FLAGS += -DPC_DEBUG_AI_LOG
endif

# Debug: SPELLFX_LOG=1 logs every gSpellsEx FX dispatch (spell, slot, handler symbol) to
# vh_spellfx_log.txt + stderr, runtime-gated on VH_SPELLFX_LOG. Matching build never defines this.
ifeq ($(SPELLFX_LOG),1)
$(BUILD_DIR)/src/battle/executors.o: GAME_C_FLAGS += -DPC_DEBUG_SPELLFX_LOG
endif
$(BUILD_DIR)/src/battle/executors.o: FORCE
$(BUILD_DIR)/src/battle/ai.o: FORCE

# Debug: AUDIO_CMD_LOG=1 logs every PerformAudioCommand (core/audio.c) to vh_audio_cmd.csv (when
# VH_AUDIO_LOG is set), e.g. to confirm whether a given PLAY_XA command is ever issued.
# Matching build never defines PC_DEBUG_AUDIO_LOG.
ifeq ($(AUDIO_CMD_LOG),1)
$(BUILD_DIR)/src/core/audio.o: GAME_C_FLAGS += -DPC_DEBUG_AUDIO_LOG
endif
$(BUILD_DIR)/src/core/audio.o: FORCE

# Debug: TERRAIN_LOG=1 instruments RenderMapTile / RenderEdgeMapTile (core/graphics.c) to
# accumulate per-frame terrain otz stats + black-tile counts into vh_terrain_otz_pc.csv (when
# VH_TERRAIN_LOG is set). Matching build never defines this.
ifeq ($(TERRAIN_LOG),1)
$(BUILD_DIR)/src/core/graphics.o: GAME_C_FLAGS += -DPC_DEBUG_TERRAIN_LOG
endif
$(BUILD_DIR)/src/core/graphics.o: FORCE
FORCE:

# -I$(STAGE_DIR) -DPERMUTER: the camera-target diagnostics read the real State/Object/graphics
# struct layouts, so this compiles through the staged game headers (PERMUTER disables the
# INCLUDE_ASM blocks they pull in). -Iinclude first so "PsyQ/libetc.h" resolves to our shim.
$(BUILD_DIR)/libetc.o: src/libetc.c | stage
	$(CC) $(M32) $(SAN) $(PORT_WARN) -fno-builtin-csqrt -DPERMUTER -Iinclude -I$(STAGE_DIR) $(SDL2_CFLAGS) -g -MMD -MP -c -o $@ $<

# Split out of libetc.c; same staged-include profile (they read the game's state/object headers).
$(BUILD_DIR)/pc_diag.o: src/pc_diag.c | stage
	$(CC) $(M32) $(SAN) $(PORT_WARN) -fno-builtin-csqrt -DPERMUTER -Iinclude -I$(STAGE_DIR) $(SDL2_CFLAGS) -g -MMD -MP -c -o $@ $<

$(BUILD_DIR)/pc_battle_speed.o: src/pc_battle_speed.c | stage
	$(CC) $(M32) $(SAN) $(PORT_WARN) -fno-builtin-csqrt -DPERMUTER -Iinclude -I$(STAGE_DIR) $(SDL2_CFLAGS) -g -MMD -MP -c -o $@ $<

$(BUILD_DIR)/libcd.o: src/libcd.c
	$(CC) $(M32) $(SAN) $(PORT_WARN) -Iinclude -g -MMD -MP -c -o $@ $<

# -I$(STAGE_DIR) -DPERMUTER: libsnd.c reaches into core/cd.c's gCdFiles/gVabLoader globals (see
# SsVabTransBodyPartly), which pulls in the cd_files.h -> common.h chain; it needs the staged
# includes and the asm escape hatch like game source, but not -std=gnu89 (it is modern C).
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

# The hot per-pixel path (DDA spans, texture sampling, blending); same flags as libgpu.o.
$(BUILD_DIR)/pc_raster.o: src/pc_raster.c
	$(CC) $(M32) $(SAN) $(PORT_WARN) -Iinclude -g -MMD -MP -c -o $@ $<

$(BUILD_DIR)/pc_hdpack.o: src/pc_hdpack.c
	$(CC) $(M32) $(SAN) $(PORT_WARN) -Iinclude $(HD_WEBP_CFLAGS) -g -MMD -MP -c -o $@ $<

$(BUILD_DIR)/pc_gpu_window.o: src/pc_gpu_window.c
	$(CC) $(M32) $(SAN) $(PORT_WARN) -Iinclude $(SDL2_CFLAGS) -g -MMD -MP -c -o $@ $<

# In-game options overlay model + its RETURN TO TITLE action, which rewrites engine state
# (gState etc.), so it compiles through the staged game headers with pc_balance.o's profile.
$(BUILD_DIR)/pc_overlay.o: src/pc_overlay.c | stage
	$(CC) $(M32) $(SAN) $(PORT_WARN) -fno-builtin-csqrt -std=gnu89 -DPERMUTER -I$(STAGE_DIR) -Iinclude -MMD -MP -c -o $@ $<

# Save-management file ops -- pure C, no SDL/game headers.
$(BUILD_DIR)/pc_saves.o: src/pc_saves.c
	$(CC) $(M32) $(SAN) $(PORT_WARN) -Iinclude -g -MMD -MP -c -o $@ $<

# Langpack movie subtitle cues -- pure C, no SDL/game headers.
$(BUILD_DIR)/pc_movie_subs.o: src/pc_movie_subs.c
	$(CC) $(M32) $(SAN) $(PORT_WARN) -Iinclude -g -MMD -MP -c -o $@ $<

# Tactical-mode balance package -- patches mutable game tables (gUnitInfo, etc.), so it needs the
# real struct layouts via the staged include chain, same flags as the pc data files above.
$(BUILD_DIR)/pc_balance.o: src/pc_balance.c | stage
	$(CC) $(M32) $(SAN) $(PORT_WARN) -fno-builtin-csqrt -std=gnu89 -DPERMUTER -I$(STAGE_DIR) -Iinclude -MMD -MP -c -o $@ $<

$(BUILD_DIR)/libsn.o: src/libsn.c
	$(CC) $(M32) $(SAN) $(PORT_WARN) -Iinclude -g -MMD -MP -c -o $@ $<

$(BUILD_DIR)/pc_bootstrap.o: src/pc_bootstrap.c
	$(CC) $(M32) $(SAN) $(PORT_WARN) -Iinclude -g -MMD -MP -c -o $@ $<

$(BUILD_DIR)/pc_xa.o: src/pc_xa.c
	$(CC) $(M32) $(SAN) $(PORT_WARN) -Iinclude -g -MMD -MP -c -o $@ $<

# Software SPU voice renderer -- OpenAL streaming sink like pc_xa.o.
$(BUILD_DIR)/pc_spu.o: src/pc_spu.c
	$(CC) $(M32) $(SAN) $(PORT_WARN) -Iinclude -g -MMD -MP -c -o $@ $<

$(BUILD_DIR)/pc_mdec.o: src/pc_mdec.c
	$(CC) $(M32) $(SAN) $(PORT_WARN) -Iinclude -g -MMD -MP -c -o $@ $<

# HD FMV decoder: libav H.264 -> RGB, replacing the STR movie picture. libav flags applied here.
$(BUILD_DIR)/pc_hdvideo.o: src/pc_hdvideo.c
	$(CC) $(M32) $(SAN) $(PORT_WARN) -Iinclude $(HD_VIDEO_CFLAGS) -g -MMD -MP -c -o $@ $<

# Generated PS1 BIOS kanji glyphs (tools/gen_kanji_font.py) for Krom2RawAdd in libkernel.c; pure
# data. US embeds the 209-glyph subset it uses; JP regenerates the full charset-2+3 ROM font
# (3489 glyphs) into GEN_DIR, since the JP game draws all its text through Krom2RawAdd.
ifeq ($(REGION),jp)
$(GEN_DIR)/pc_kanji_font.c: tools/gen_kanji_font.py
	@mkdir -p $(GEN_DIR); VH_REGION=jp VH_GENERATED_OUT=$@ python3 $<
$(BUILD_DIR)/pc_kanji_font.o: $(GEN_DIR)/pc_kanji_font.c
	$(CC) $(M32) $(SAN) $(PORT_WARN) -Iinclude -g -MMD -MP -c -o $@ $<
else
$(BUILD_DIR)/pc_kanji_font.o: src/pc_kanji_font.c
	$(CC) $(M32) $(SAN) $(PORT_WARN) -Iinclude -g -MMD -MP -c -o $@ $<
endif

# Compiles every src/*.c independently and reports a pass/fail summary without stopping at the
# first error, unlike `make default`.
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

# Resolve the game-code sites in vh_null_reads.log (the NULL-read fixup handler's list) to
# file:line. Each logged site is `SYMBOL+0xOFF`; SYMBOL is looked up with nm and the offset
# added for addr2line.
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

# Header-dependency files emitted by -MMD above. Must be last: make merges these in as extra
# prerequisites so a header edit rebuilds exactly what includes it.
-include $(BUILD_DIR)/src/*.d $(BUILD_DIR)/src/*/*.d $(BUILD_DIR)/*.d
