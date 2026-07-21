# Include the core environment definitions; this will set $(TOP).
include ../../py/mkenv.mk

# Include py core make definitions.
include $(TOP)/py/py.mk

# Define used paths
ROOTDIR 		 = $(realpath .)
SPECS_FILE 		:= $(ROOTDIR)/bsp/Xilinx.spec
# BSP_SRC 		:= $(ROOTDIR)/bsp/libsrc
BSP_HEADERS 	:= $(ROOTDIR)/bsp/include
BSP_LIBS_PATH 	:= $(ROOTDIR)/bsp/lib

# Set makefile-level MicroPython feature configurations.
MICROPY_ROM_TEXT_COMPRESSION ?= 0

# Define toolchain and other tools.
CROSS_COMPILE ?= $${XILINX_VITIS}/gnu/aarch32/lin/gcc-arm-none-eabi/bin/arm-none-eabi-
DFU ?= $(TOP)/tools/dfu.py
PYDFU ?= $(TOP)/tools/pydfu.py

# Set CFLAGS.
CFLAGS += -I. -I$(TOP) -I$(BUILD) -I$(BSP_HEADERS)
CFLAGS += -Wall -Werror -std=c99
CFLAGS += -DSDT -mthumb -mcpu=cortex-a9 -mfpu=vfpv3 -mfloat-abi=softfp -MMD -MP -specs=$(SPECS_FILE)
CSUPEROPT = -Os # save some code space for performance-critical code

# CFLAGS		 := -DSDT -mcpu=cortex-a9 -mfpu=vfpv3 -mfloat-abi=hard -MMD -MP \
# 				-specs=$(SPECS_FILE) -I$(BSP_HEADERS)
# CFLAGS_EXTRA := -O2 -g -Wall -Wextra -fno-tree-loop-distribute-patterns -DNDEBUG

# Select debugging or optimisation build.
ifeq ($(DEBUG), 1)
CFLAGS += -Og
else
CFLAGS += -Os -DNDEBUG
CFLAGS += -fdata-sections -ffunction-sections
endif

# Set linker flags.
# For some reason linker doesn't search for libraries in sysroot dirs.
# Pass the paths explicitly for now.
LDFLAGS += -T lscript.ld \
		   -specs=$(SPECS_FILE) \
		   -L"$(BSP_LIBS_PATH)" -L"/" \
		   -L"$${XILINX_VITIS}/gnu/aarch32/lin/gcc-arm-none-eabi/aarch32-xilinx-eabi/usr/lib" \
		   -L"$${XILINX_VITIS}/gnu/aarch32/lin/gcc-arm-none-eabi/aarch32-xilinx-eabi/usr/lib/arm-xilinx-eabi/13.3.0" \
		   -Wl,--start-group -Wl,$(LIBS) -Wl,--end-group

# Define the required source files.
# SRC_C += lib.c main.c system.c
SRC_C += lib.c main.c

# Define the required object files.
OBJ += $(PY_CORE_O)
OBJ += $(addprefix $(BUILD)/, $(SRC_C:.c=.o))

# Define static libs
comma:= ,
empty:=
space:= $(empty) $(empty)
LIBS := xil xilstandalone gcc c
LIBS := $(addprefix -l,$(LIBS))
LIBS := $(subst $(space),$(comma),$(LIBS))

# Define the top-level target, the main firmware.
all: $(BUILD)/firmware.dfu

$(BUILD)/firmware.elf: $(OBJ)
	$(ECHO) "LINK $@"
	$(Q)$(CC) $^ -o $@ $(LDFLAGS)
	$(Q)$(SIZE) $@

$(BUILD)/firmware.bin: $(BUILD)/firmware.elf
	$(ECHO) "Create $@"
	$(Q)$(OBJCOPY) -O binary -j .isr_vector -j .text -j .data $^ $@

$(BUILD)/firmware.dfu: $(BUILD)/firmware.bin
	$(ECHO) "Create $@"
	$(Q)$(PYTHON) $(DFU) -b 0x08000000:$^ $@

deploy: $(BUILD)/firmware.dfu
	$(Q)$(PYTHON) $(PYDFU) -u $^

# Include remaining core make rules.
include $(TOP)/py/mkrules.mk
