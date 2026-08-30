# SLPM-86007 (Vandal Hearts Japan) matching decomp — Phase 0 asm baseline.
BASENAME  := SLPM_860.07
TOOLS_DIR := tools
ASM_DIRS  := asm asm/data
C_DIRS    := $(shell find src -type d 2>/dev/null)
TARGET    := build/$(BASENAME)

BIN_DIRS  := assets
S_FILES := $(foreach dir,$(ASM_DIRS),$(wildcard $(dir)/*.s))
C_FILES := $(foreach dir,$(C_DIRS),$(wildcard $(dir)/*.c))
BIN_FILES := $(foreach dir,$(BIN_DIRS),$(wildcard $(dir)/*.bin))
O_FILES := $(foreach file,$(S_FILES),build/$(file).o) $(foreach file,$(C_FILES),build/$(file).o) $(foreach file,$(BIN_FILES),build/$(file).o)

CROSS   := mipsel-linux-gnu-
PYTHON  := python3
CPP     := cpp -P
AS      := $(CROSS)as -EL
LD      := $(CROSS)ld -EL
OBJCOPY := $(CROSS)objcopy
CC      := cc1_v263_decompals

SPLAT   := $(PYTHON) -m splat split
AS_FLAGS := -march=r3000 -mtune=r3000 -Iinclude -no-pad-sections -G0
D_FLAGS  := -Dmips -D__GNUC__=2 -D__OPTIMIZE__ -D__mips__ -D__mips -Dpsx -D__psx__ -D__psx -D_PSYQ -D__EXTENSIONS__ -D_MIPSEL -D__CHAR_UNSIGNED__ -D_LANGUAGE_C -DLANGUAGE_C
OPT_FLAGS := -O1
GP_OPT    := -G8
CC_FLAGS  = $(GP_OPT) $(OPT_FLAGS) -Wall -mgas -mgpopt -msoft-float -ffunction-cse -fpcc-struct-return -fgnu-linker -gcoff
CPP_FLAGS := -undef -Wall -lang-c $(D_FLAGS) -Iinclude -Iinclude/PsyQ -nostdinc
OBJCOPY_FLAGS := -O binary
LD_FLAGS  := --cref -Map build/$(BASENAME).map -T $(BASENAME).ld -T undefined_syms_auto.txt -T undefined_funcs_auto.txt -T undefined_additional.txt --no-check-sections
MASPSX    := $(PYTHON) $(TOOLS_DIR)/maspsx/maspsx.py --expand-div --aspsx-version=2.21 --use-comm-section

# Per-file compiler overrides (from the US build — the JP build uses the SAME toolchain/flags).
# Small-data threshold -G0 (nothing routes to .sdata) for the text/font/menu files; core/audio also
# uses the older cc1_v257 frontend at -O2. Applied to BOTH the .c.s (cc1) and .c.o (maspsx GP_OPT) steps.
build/src/core/audio.c.s: CC := cc1_v257_decompals
build/src/core/audio.c.s: OPT_FLAGS := -O2
build/src/core/audio.c.s: GP_OPT := -G0
build/src/core/audio.c.o: GP_OPT := -G0
build/src/core/glyphs.c.s: GP_OPT := -G0
build/src/core/glyphs.c.o: GP_OPT := -G0
build/src/ui/supplies.c.s: GP_OPT := -G0
build/src/ui/supplies.c.o: GP_OPT := -G0
build/src/world/dojo.c.s: GP_OPT := -G0
build/src/world/dojo.c.o: GP_OPT := -G0
build/src/world/map.c.s: GP_OPT := -G0
build/src/world/map.c.o: GP_OPT := -G0
build/src/world/town.c.s: GP_OPT := -G0
build/src/world/town.c.o: GP_OPT := -G0

default: check
clean:
	rm -rf asm build
extract:
	$(SPLAT) $(BASENAME).yaml
	$(PYTHON) tools/gen_assets_inc.py
# The hand-authored initializer includes are disc-derived and regenerated, never committed;
# splat only carves assets/*.bin, so `extract` must run the generator too or a fresh clone
# fails at `fatal error: assets/801009bc.inc: No such file or directory`.
assets-inc:
	$(PYTHON) tools/gen_assets_inc.py
check: $(TARGET)
	md5sum "$(BASENAME)" "$(TARGET)"
dirs:
	$(foreach dir,$(ASM_DIRS) $(C_DIRS),$(shell mkdir -p build/$(dir)))
$(TARGET): $(TARGET).elf
	$(OBJCOPY) $(OBJCOPY_FLAGS) $< $@
$(TARGET).elf: $(O_FILES)
	$(LD) $(LD_FLAGS) -o $@
build/%.s.o: %.s
	@mkdir -p $(@D)
	$(AS) $(AS_FLAGS) -o $@ $<
build/%.bin.o: %.bin
	@mkdir -p $(@D)
	$(LD) -r -b binary -o $@ $<
build/%.i: %.c
	@mkdir -p $(@D)
	$(CPP) $(CPP_FLAGS) -o $@ $<
build/%.c.s: build/%.i
	$(CC) $(CC_FLAGS) -o $@ $<
build/%.c.o: build/%.c.s
	$(MASPSX) $(GP_OPT) $< | $(AS) $(AS_FLAGS) -o $@
.SECONDARY:
.PHONY: clean default extract check dirs
SHELL = /bin/bash -e -o pipefail
