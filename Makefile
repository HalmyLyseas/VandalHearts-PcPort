BASENAME        := SLUS_004.47

TOOLS_DIR       := tools

ASM_DIRS        := asm asm/data
C_DIRS          := src src/core src/states src/battle src/units src/world src/events src/spells src/maps src/ui
BIN_DIRS        := assets

TARGET          := build/$(BASENAME)

S_FILES         := $(foreach dir,$(ASM_DIRS),$(wildcard $(dir)/*.s))
C_FILES         := $(foreach dir,$(C_DIRS),$(wildcard $(dir)/*.c))
BIN_FILES       := $(foreach dir,$(BIN_DIRS),$(wildcard $(dir)/*.bin))

O_FILES         := $(foreach file,$(S_FILES),build/$(file).o)    \
                   $(foreach file,$(C_FILES),build/$(file).o)    \
                   $(foreach file,$(BIN_FILES),build/$(file).o)

CROSS           := mips-suse-linux-
PYTHON          := python3.11
CPP             := cpp -P
AS              := $(CROSS)as -EL
LD              := $(CROSS)ld -EL
OBJCOPY         := $(CROSS)objcopy

# Note: cc1_v263 compiled with CFLAGS="-std=gnu89 -m32 -DBYTES_BIG_ENDIAN=0 -DTARGET_MEM_FUNCTIONS"
#CC              := cc1_v263_EL
# Update: Switched to builds provided by decompals at https://github.com/decompals/old-gcc
CC              := cc1_v263_decompals

SPLAT           := $(PYTHON) -m splat split
SORT_SYM        := $(PYTHON) $(TOOLS_DIR)/sortSymbols.py --undefined-addrs-path=undefined_additional.txt --symbol-addrs-path=symbol_addrs.txt symbols.csv

AS_FLAGS        := -march=r3000 -mtune=r3000 -Iinclude -no-pad-sections -G0
D_FLAGS         := -Dmips -D__GNUC__=2 -D__OPTIMIZE__ -D__mips__ -D__mips -Dpsx -D__psx__ -D__psx -D_PSYQ -D__EXTENSIONS__ -D_MIPSEL -D__CHAR_UNSIGNED__ -D_LANGUAGE_C -DLANGUAGE_C

OPT_FLAGS       := -O1
GP_OPT          := -G8

CC_FLAGS        = $(GP_OPT) $(OPT_FLAGS) -Wall -mgas -mgpopt -msoft-float -ffunction-cse -fpcc-struct-return -fgnu-linker -gcoff

CPP_FLAGS       := -undef -Wall -lang-c $(D_FLAGS) -Iinclude -Iinclude/PsyQ -nostdinc
OBJCOPY_FLAGS   := -O binary
LD_FLAGS        := --cref -Map build/$(BASENAME).map -T $(BASENAME).ld -T undefined_syms_auto.txt -T undefined_funcs_auto.txt -T undefined_additional.txt --no-check-sections

MASPSX          := $(PYTHON) $(TOOLS_DIR)/maspsx/maspsx.py --expand-div --aspsx-version=2.21 --use-comm-section

build/src/core/audio.c.s: CC := cc1_v257_decompals
build/src/core/audio.c.s: OPT_FLAGS := -O2
build/src/core/audio.c.s: GP_OPT := -G0
build/src/core/audio.c.o: GP_OPT := -G0

build/src/core/glyphs.c.s: GP_OPT := -G0
build/src/core/glyphs.c.o: GP_OPT := -G0

#build/src/temp_sdata.c.s: GP_OPT := -G16

build/src/ui/supplies.c.s: GP_OPT := -G0
build/src/ui/supplies.c.o: GP_OPT := -G0
build/src/world/dojo.c.s: GP_OPT := -G0
build/src/world/dojo.c.o: GP_OPT := -G0
build/src/world/map.c.s: GP_OPT := -G0
build/src/world/map.c.o: GP_OPT := -G0

default: check

clean:
	rm -rf asm assets build

extract:
	$(SORT_SYM)
	$(SPLAT) $(BASENAME).yaml

check: $(TARGET)
	md5sum "$(BASENAME)" "$(TARGET)"

dirs:
	$(foreach dir,$(ASM_DIRS) $(BIN_DIRS) $(C_DIRS),$(shell mkdir -p build/$(dir)))

$(TARGET): $(TARGET).elf
	$(OBJCOPY) $(OBJCOPY_FLAGS) $< $@

$(TARGET).elf: $(O_FILES)
	$(LD) $(LD_FLAGS) -o $@

build/%.s.o: %.s
	$(AS) $(AS_FLAGS) -o $@ $<

build/%.bin.o: %.bin
	$(LD) -r -b binary -o $@ $<

build/%.i: %.c
	$(CPP) $(CPP_FLAGS) -o $@ $<

build/%.c.s: build/%.i
	$(CC) $(CC_FLAGS) -o $@ $<

build/%.c.o: build/%.c.s
	$(MASPSX) $(GP_OPT) $< | $(AS) $(AS_FLAGS) -o $@


.SECONDARY:
.PHONY: clean default extract check
SHELL = /bin/bash -e -o pipefail
