# Define default board
BOARD ?= Z-TURN
BOARD_DIR ?= boards/$(BOARD)

ifeq ($(wildcard $(BOARD_DIR)/.),)
$(error Invalid BOARD specified: $(BOARD_DIR))
endif

include ../../py/mkenv.mk
include $(BOARD_DIR)/mpconfigboard.mk

# qstr definitions (must come before including py.mk)
QSTR_DEFS = qstrdefsport.h

# MicroPython feature configurations
MICROPY_ROM_TEXT_COMPRESSION ?= 1

# File containing description of content to be frozen into firmware.
FROZEN_MANIFEST ?= manifest.py

# include py core make definitions
include $(TOP)/py/py.mk
include $(TOP)/extmod/extmod.mk

CROSS_COMPILE ?= $${XILINX_VITIS}/gnu/aarch32/lin/gcc-arm-none-eabi/bin/arm-none-eabi-

INC += -I.
INC += -I$(TOP)
INC += -I$(BUILD)
INC += -I$(BOARD_DIR)
INC += -Ibsp

BOOT_IMG = boot.bin
CFLAGS_CORTEX_A9 = -mthumb -mcpu=cortex-a9 -msoft-float -fsingle-precision-constant \
				   -Wdouble-promotion -Wfloat-conversion -DSDT -specs=Xilinx.spec
# Use C_ADD_FLAGS to pass flags when invoking make
CFLAGS += $(INC) -Wall -Werror -std=c99 $(CFLAGS_CORTEX_A9) $(C_ADD_FLAGS)
LDFLAGS += -nostdlib -T lscript.ld -Wl,-Map=$@.map -Wl,--cref -Wl,--gc-sections
ifeq ($(BUILD_VERBOSE),1)
LDFLAGS += -Wl,--trace
endif

CSUPEROPT = -Os # save some code space

LIBGCC_FILE_NAME = $(shell $(CC) $(CFLAGS) -print-libgcc-file-name)
LIBS += -L$(dir $(LIBGCC_FILE_NAME)) -lgcc

# Tune for Debugging or Optimization
CFLAGS += -g  # always include debug info in the ELF
ifeq ($(DEBUG), 1)
CFLAGS += -O0
else
CFLAGS += -Os -DNDEBUG
CFLAGS += -fdata-sections -ffunction-sections
endif

SHARED_SRC_C += $(addprefix shared/,\
	libc/string0.c \
	libc/printf.c \
	readline/readline.c \
	runtime/pyexec.c \
	runtime/stdout_helpers.c \
	)

SRC_C = \
	main.c \
	uart_core.c \
	mphalport.c \

BSP_SRC_C += $(addprefix bsp/,\
	xuartps.c \
	xuartps_hw.c \
	)

OBJ += $(PY_O)
OBJ += $(addprefix $(BUILD)/, $(SHARED_SRC_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(SRC_C:.c=.o))

# List of sources for qstr extraction
SRC_QSTR += $(SRC_C) $(SHARED_SRC_C)

# Custom functions
define WRITE_BOOT_DEV
	@sh -c '{ [ -n "$(DEV)" ] && [ -n "$(MNT_POINT)" ]; } || \
		{ echo "Cannot write to SD card: DEV or MNT_POINT not specified"; exit 1; }'
	@sh -c '[ -b $(DEV) ] || { echo "Cannot write to SD card: $(DEV) does not exist"; exit 1; }'
	$(ECHO) "Writing ${1}, ${2} to SD card..."
	@sudo sh -c "mount $(DEV) $(MNT_POINT) && $(CP) -v ${1} ${2} $(MNT_POINT) && umount $(MNT_POINT)"
endef

# Targets
all: $(BUILD)/firmware.elf cp-compilation-db

$(BUILD)/firmware.elf: $(OBJ)
	$(ECHO) "LINK $@"
	$(Q)$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)
	$(Q)$(SIZE) $@

$(BOOT_IMG): $(BUILD)/firmware.elf
	bootgen -arch zynq -image zturn.bif -w on -o $@

deploy: $(BOOT_IMG)
	$(call WRITE_BOOT_DEV,$(BOOT_IMG),boot.py)

cp-compilation-db:
	@sh -c "[ -f compile_commands.json ] && cp -u compile_commands.json $(TOP)"

include $(TOP)/py/mkrules.mk
