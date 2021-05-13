#
# Project name (also used for output file name)
#
PROJECT = bluetank

#
# Source files and search directories
#
CSRC   += uart.c chip.c main.c
CSRC   += ntlibc.c ntshell.c text_editor.c text_history.c vtrecv.c vtsend.c
ASRC   +=
VPATH   = ../../../lib/core

#
# Optimization level (0, 1, 2, 3, 4 or s)
#
OPTIMIZE = s

#
# C Standard level (c89, gnu89, c99 or gnu99)
#
CSTD = gnu89

#
# Processor type for CSRC/ASRC files.
#
CPU = bf592-any

#
# Linker script for the target MCU
#
LINKSCRIPT =

#
# Output file format (ihex, bin or both) and debugger type
#
OUTPUT  = ihex
DEBUG   = dwarf-2

#
# Include dirs, library dirs and definitions
#
LIBS    =
LIBDIRS =
INCDIRS = ../../../lib/core
DEFS    =
ADEFS   =

#
# Warning contorls
#
WARNINGS = all extra

#
# Object output directory
#
OBJDIR = obj

#
# Programs to build porject
#
CC      = bfin-elf-gcc
OBJCOPY = bfin-elf-objcopy
OBJDUMP = bfin-elf-objdump
SIZE    = bfin-elf-size
NM      = bfin-elf-nm

#
# Define all object files
#
COBJ      = $(CSRC:.c=.o)
AOBJ      = $(ASRC:.S=.o)
COBJ      := $(addprefix $(OBJDIR)/,$(COBJ))
AOBJ      := $(addprefix $(OBJDIR)/,$(AOBJ))
PROJECT   := $(OBJDIR)/$(PROJECT)

#
# Flags for C files
#
CFLAGS += -std=$(CSTD)
CFLAGS += -g$(DEBUG)
CFLAGS += -O$(OPTIMIZE)
CFLAGS += -fmessage-length=0
CFLAGS += $(addprefix -W,$(WARNINGS))
CFLAGS += $(addprefix -I,$(INCDIRS))
CFLAGS += $(addprefix -D,$(DEFS))
CFLAGS += -Wp,-M,-MP,-MT,$(OBJDIR)/$(*F).o,-MF,$(OBJDIR)/$(*F).d

#
# Assembler flags
#
ASFLAGS += $(addprefix -D,$(ADEFS)) -Wa,-g$(DEBUG)

#
# Linker flags
#
LDFLAGS += -Wl,-Map=$(PROJECT).map,--cref,--gc-sections
LDFLAGS += -lc -lgcc
LDFLAGS += $(patsubst %,-L%,$(LIBDIRS)) $(patsubst %,-l%,$(LIBS))
#LDFLAGS += -T$(LINKSCRIPT)
#LDFLAGS += -nostartfiles
#LDFLAGS += -nodefaultlibs
#LDFLAGS += -nostdlib
#LDFLAGS += -fno-builtin

# Combine all necessary flags and optional flags.
# Add target processor to flags.
ALL_CFLAGS  = -mcpu=$(CPU) -I. $(CFLAGS)
ALL_ASFLAGS = -mcpu=$(CPU) -I. -x assembler-with-cpp $(ASFLAGS)

#
# Default target.
#
all: version build size

ifeq ($(OUTPUT),ihex)
build: elf hex lst sym
hex: $(PROJECT).hex
else
ifeq ($(OUTPUT),binary)
build: elf bin lst sym
bin: $(PROJECT).bin
else
ifeq ($(OUTPUT),both)
build: elf hex bin lst sym
hex: $(PROJECT).hex
bin: $(PROJECT).bin
else
$(error "Invalid format: $(OUTPUT)")
endif
endif
endif

elf: $(PROJECT).elf
lst: $(PROJECT).lst
sym: $(PROJECT).sym

#
# Display compiler version information.
#
version :
	@$(CC) --version

#
# Create final output file (.hex or .bin) from ELF output file.
#
%.hex: %.elf
	@echo
	$(OBJCOPY) -O ihex $< $@

%.bin: %.elf
	@echo
	$(OBJCOPY) -O binary $< $@

#
# Create extended listing file from ELF output file.
#
%.lst: %.elf
	@echo
	$(OBJDUMP) -h -S -C $< > $@

#
# Create a symbol table from ELF output file.
#
%.sym: %.elf
	@echo
	$(NM) -n $< > $@

#
# Display size of file.
#
size:
	@echo
	$(SIZE) -A $(PROJECT).elf

#
# Link: create ELF output file from object files.
#
%.elf:  $(AOBJ) $(COBJ)
	@echo
	@echo Linking...
	$(CC) $(ALL_CFLAGS) $(AOBJ) $(COBJ) --output $@ $(LDFLAGS)

#
# Compile: create object files from C source files.
#
$(COBJ) : $(OBJDIR)/%.o : %.c
	@echo
	@echo $< :
	$(CC) -c $(ALL_CFLAGS) $< -o $@

#
# Assemble: create object files from assembler source files.
#
$(AOBJ) : $(OBJDIR)/%.o : %.S
	@echo
	@echo $< :
	$(CC) -c $(ALL_ASFLAGS) $< -o $@

#
# Target: clean project.
#
clean:
	@echo
	rm -f -r $(OBJDIR) | exit 0

#
# Load: Load ldr for Blackfin
#
load:
	bfin-elf-ldr -T BF592 -c $(PROJECT).ldr $(PROJECT).elf
	bfin-elf-ldr -l $(PROJECT).ldr /dev/ttyUSB0 -b 57600 -C

#
# Console: Console
#
console:
	screen /dev/ttyUSB0 57600

#
# Include the dependency files.
#
-include $(shell mkdir $(OBJDIR) 2>/dev/null) $(wildcard $(OBJDIR)/*.d)

