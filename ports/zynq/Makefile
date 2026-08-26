include ../../py/mkenv.mk

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

BOOT_IMG = boot.bin
CFLAGS_CORTEX_A9 = -mthumb -mcpu=cortex-a9 -msoft-float -fsingle-precision-constant -Wdouble-promotion -Wfloat-conversion
COPT += -DSDT -specs=Xilinx.spec
CFLAGS += $(INC) -Wall -Werror -std=c99 $(CFLAGS_CORTEX_A9) $(COPT)
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

SRC_C = \
	main.c \
	xuartps.c \
	xuartps_hw.c \
	uart_core.c \
	mphalport.c \
	shared/libc/string0.c \
	shared/libc/printf.c \
	shared/readline/readline.c \
	shared/runtime/pyexec.c \
	shared/runtime/stdout_helpers.c \

SRC_QSTR += shared/readline/readline.c shared/runtime/pyexec.c

OBJ += $(PY_O)
OBJ += $(addprefix $(BUILD)/, $(SRC_C:.c=.o))

# Custom functions
define WRITE_BOOT_DEV
	@sh -c '{ [ -n "$(DEV)" ] && [ -n "$(MNT_POINT)" ]; } || \
		{ echo "Cannot write to SD card: DEV or MNT_POINT not specified"; exit 1; }'
	@sh -c '[ -b $(DEV) ] || { echo "Cannot write to SD card: $(DEV) does not exist"; exit 1; }'
	$(ECHO) "Writing $< to SD card..."
	@sudo sh -c "mount $(DEV) $(MNT_POINT) && $(CP) -v $< $(MNT_POINT) && umount $(MNT_POINT)"
endef

# Targets
all: $(BUILD)/firmware.elf

$(BUILD)/firmware.elf: $(OBJ)
	$(ECHO) "LINK $@"
	$(Q)$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)
	$(Q)$(SIZE) $@

$(BOOT_IMG): $(BUILD)/firmware.elf
	bootgen -arch zynq -image zturn.bif -w on -o $@

deploy: $(BOOT_IMG)
	$(call WRITE_BOOT_DEV)

include $(TOP)/py/mkrules.mk
