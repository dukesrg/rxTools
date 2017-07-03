# Copyright (C) 2015 The PASTA Team
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# version 2 as published by the Free Software Foundation
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

ifeq ($(strip $(RXTOOLSMK)),)
$(error "Please run `make` at the top directory")
endif

BUILD := build
DEPS_DIR := $(BUILD)/.deps
SOURCE := source

all: $(BUILD)/code.bin $(BUILD)/arm9loaderhax.bin

include $(RXTOOLSMK)

LD := $(CC)

INCDIR += -Isource -Isource/features -Isource/lib
CPPDEFS	:= -DSYS_PATH=\"$(SYS_PATH)\" -DPATCHES_PATH=\"$(PATCHES_PATH)\" -D_POSIX_C_SOURCE=200809L
CPPFLAGS := $(INCDIR) $(CPPDEFS)

CFORCE := -fomit-frame-pointer -ffast-math
CMACHINE := -mtune=arm946e-s
CWARNING := -Wall -Wno-multichar
CFLAGS := -g -Os -falign-functions=16 -falign-jumps=16 -falign-loops=16 -falign-labels=16 -std=c99 $(CWARNING) $(CFORCE) $(CMACHINE) $(CPPFLAGS) -DVERSION=\"$(GIT_VERSION)\"

# Use Thumb libraries because sbrk of ARM newlib in devkitPro doesn't work.
LDFLAGS := -nostartfiles -g $(CMACHINE) -mthumb -mthumb-interwork -Wl,-Tstub.ld

CCOBJS := $(addprefix $(BUILD)/,main.o features/CTRDecryptor.o \
	features/NandDumper.o features/TitleKeyDecrypt.o \
	features/configuration.o features/downgradeapp.o \
	features/AdvancedFileManager.o features/firm.o	\
	features/padgen.o features/screenshot.o	\
	lib/fatfs/diskio.o lib/fatfs/ff.o lib/fatfs/option/unicode.o \
	lib/tmio/tmio.o \
	lib/patch3ds/patch3ds.o \
	lib/signature.o lib/ticket.o lib/native_firm.o \
	lib/console.o lib/crypto.o lib/draw.o lib/fs.o lib/hid.o lib/lang.o \
	lib/log.o lib/i2c.o lib/jsmn/jsmn.o lib/menu.o lib/nand.o lib/ncch.o \
	lib/json.o lib/theme.o lib/tmd.o lib/progress.o lib/cfnt.o \
	lib/timer.o lib/aes.o lib/sha.o lib/rsa.o lib/movable_sed.o lib/bootsector.o)

OBJS	:= $(CCOBJS) $(addprefix $(BUILD)/, start.o lib/delay.o)

DEPS	:= $(subst $(BUILD)/,$(DEPS_DIR)/,$(CCOBJS))

$(call DEPDIR,$(BUILD)/rxtools.elf $(OBJS) $(DEPS))

MAKEDEP	= @$(COMPILE.c) -MM $< > $(patsubst $(BUILD)/%.o,$(DEPS_DIR)/%.d,$@)

.PHONY: $(BUILD)/code.bin $(BUILD)/arm9loaderhax.bin all clean

$(BUILD)/arm9loaderhax.bin: $(BUILD)/rxtools.bin
	@cp -av $< $@

$(BUILD)/code.bin: $(BUILD)/rxtools.bin $(BUILD)/CakeHax/D
	$(call Q,MAKE,CakeHax)$(MAKE) dir_out=../$(dir $@) dir_build=../$(BUILD)/CakeHax name=$(notdir $@) filepath=$(SYS_PATH)/ -C CakeHax bigpayload
	$(call Q,DD,$@)dd if=$< bs=896k conv=sync,notrunc | dd of=$@ seek=160 conv=notrunc

%.bin: %.elf
	$(call Q,OBJCOPY,$@)$(OBJCOPY) --set-section-flags .bss=alloc,load,contents -O binary $< $@

$(BUILD)/rxtools.elf: $(OBJS)
	$(call Q,LINK,$@)$(LINK.o) $^ $(OUTPUT_OPTION)

$(BUILD)/%.o: $(SOURCE)/%.c
	$(call Q,CC,$@)$(COMPILE.c) $< $(OUTPUT_OPTION)
	$(MAKEDEP)

$(BUILD)/%.o: $(SOURCE)/%.S
	$(call Q,CPPAS,$@)$(COMPILE.c) $< $(OUTPUT_OPTION)
	$(MAKEDEP)

$(BUILD)/%.o: $(SOURCE)/%.s
	$(call Q,AS,$@)$(AS) $< $(OUTPUT_OPTION)

$(BUILD)/%.o: $(BUILD)/%.bin
	$(call Q,OBJCOPY,$@)cd $(BUILD); $(OBJCOPY) -Ibinary -Oelf32-littlearm -Barmv5te $(subst $(BUILD)/,,$< $@)

$(foreach f,$(NEED_DIRS),$(eval $f : | $(dir $f)D))

%/D:
	$(call Q,MKDIR,$(dir $@))mkdir -p $(dir $@)

clean:
	$(call Q,RM,)rm -fr $(BUILD)

-include $(DEPS)
