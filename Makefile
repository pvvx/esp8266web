#############################################################
#
# Root Level Makefile
#
#############################################################

ESPOPTION ?= -p COM6 -b 460800

# SPI_SPEED = 40MHz or 80MHz
SPI_SPEED?=80
# SPI_MODE: QIO, DIO, QOUT, DOUT
SPI_MODE?=QIO
# SPI_SIZE: 512KB for all size Flash ! (512 kbytes .. 16 Mbytes Flash autodetect)
SPI_SIZE?=512
# 
ADDR_FW1 = 0x00000
ADDR_FW2 = 0x06000
# 
#USERFADDR = 0x3C000
USERFADDR = $(shell printf '0x%X\n' $$(( ($$(stat --printf="%s" $(OUTBIN2)) + 0xFFF + $(ADDR_FW2)) & (0xFFFFE000) )) )
USERFBIN = ./webbin/WEBFiles.bin
#
FIRMWAREDIR := bin
CLREEPBIN := ./$(FIRMWAREDIR)/clear_eep.bin
CLREEPADDR := 0x79000
DEFAULTBIN := ./$(FIRMWAREDIR)/esp_init_data_default.bin
DEFAULTADDR := 0x7C000
BLANKBIN := ./$(FIRMWAREDIR)/blank.bin
BLANKADDR := 0x7E000

# Base directory for the compiler
XTENSA_TOOLS_ROOT ?= c:/Espressif/xtensa-lx106-elf/bin

#PATH := $(XTENSA_TOOLS_ROOT);$(PATH)

GET_FILESIZE ?= 

# base directory of the ESP8266 SDK package, absolute
#SDK_BASE	?= c:/Espressif/ESP8266_SDK

# select which tools to use as compiler, librarian and linker
CC := $(XTENSA_TOOLS_ROOT)/xtensa-lx106-elf-gcc
AR := $(XTENSA_TOOLS_ROOT)/xtensa-lx106-elf-ar
LD := $(XTENSA_TOOLS_ROOT)/xtensa-lx106-elf-gcc
NM := $(XTENSA_TOOLS_ROOT)/xtensa-lx106-elf-nm
CPP = $(XTENSA_TOOLS_ROOT)/xtensa-lx106-elf-cpp
OBJCOPY = $(XTENSA_TOOLS_ROOT)/xtensa-lx106-elf-objcopy
OBJDUMP := $(XTENSA_TOOLS_ROOT)/xtensa-lx106-elf-objdump

SDK_TOOLS	?= c:/Espressif/utils
#ESPTOOL		?= $(SDK_TOOLS)/esptool
ESPTOOL		?= C:/Python27/python.exe $(CWD)esptool.py

CSRCS ?= $(wildcard *.c)
ASRCs ?= $(wildcard *.s)
ASRCS ?= $(wildcard *.S)
SUBDIRS ?= $(patsubst %/,%,$(dir $(wildcard */Makefile)))

ODIR := .output
OBJODIR := $(ODIR)/$(TARGET)/obj

OBJS := $(CSRCS:%.c=$(OBJODIR)/%.o) \
        $(ASRCs:%.s=$(OBJODIR)/%.o) \
        $(ASRCS:%.S=$(OBJODIR)/%.o)

DEPS := $(CSRCS:%.c=$(OBJODIR)/%.d) \
        $(ASRCs:%.s=$(OBJODIR)/%.d) \
        $(ASRCS:%.S=$(OBJODIR)/%.d)

LIBODIR := $(ODIR)/$(TARGET)/lib
OLIBS := $(GEN_LIBS:%=$(LIBODIR)/%)

IMAGEODIR := $(ODIR)/$(TARGET)/image
OIMAGES := $(GEN_IMAGES:%=$(IMAGEODIR)/%)

BINODIR := $(ODIR)/$(TARGET)/bin
OBINS := $(GEN_BINS:%=$(BINODIR)/%)

OUTBIN1 := ./$(FIRMWAREDIR)/$(ADDR_FW1).bin
OUTBIN2 := ./$(FIRMWAREDIR)/$(ADDR_FW2).bin

CCFLAGS += \
	-std=gnu90	\
	-Os	\
	-Wall	\
	-Werror	\
	-Wno-pointer-sign	\
	-mtarget-align	\
	-mlongcalls	\
	-mno-serialize-volatile	\
	-mtext-section-literals	\
	-fno-tree-ccp	\
	-foptimize-register-move	\
	-fno-inline-functions	\
	-Wl,-EL	\
	-nostdlib

ifeq ($(SPI_SPEED), 26.7)
    freqdiv = 1
	flashimageoptions = -ff 26m
else
    ifeq ($(SPI_SPEED), 20)
        freqdiv = 2
        flashimageoptions = -ff 20m
    else
        ifeq ($(SPI_SPEED), 80)
            freqdiv = 15
			flashimageoptions = -ff 80m
        else
            freqdiv = 0
			flashimageoptions = -ff 40m
        endif
    endif
endif

ifeq ($(SPI_MODE), QOUT)
    mode = 1
	flashimageoptions += -fm qout
else
    ifeq ($(SPI_MODE), DIO)
        mode = 2
		flashimageoptions += -fm dio
    else
        ifeq ($(SPI_MODE), DOUT)
            mode = 3
			flashimageoptions += -fm dout
        else
            mode = 0
			flashimageoptions += -fm qio
        endif
    endif
endif

# flash larger than 1024KB only use 1024KB to storage user1.bin and user2.bin
ifeq ($(SPI_SIZE), 256)
    size = 1
    flash = 256
	flashimageoptions += -fs 2m
else
    ifeq ($(SPI_SIZE), 1024)
        size = 2
        flash = 1024
		flashimageoptions += -fs 8m
    else
        ifeq ($(SPI_SIZE), 2048)
            size = 3
            flash = 1024
			flashimageoptions += -fs 16m
        else
            ifeq ($(SPI_SIZE), 4096)
                size = 4
                flash = 1024
				flashimageoptions += -fs 32m
            else
                size = 0
                flash = 512
				flashimageoptions += -fs 4m
            endif
        endif
    endif
endif

CCFLAGS += -DUSE_FIX_QSPI_FLASH=$(SPI_SPEED)

CFLAGS = $(CCFLAGS) $(DEFINES) $(INCLUDES)
DFLAGS = $(CCFLAGS) $(DDEFINES) $(INCLUDES)

define ShortcutRule
$(1): .subdirs $(2)/$(1)
endef

define MakeLibrary
DEP_LIBS_$(1) = $$(foreach lib,$$(filter %.a,$$(COMPONENTS_$(1))),$$(dir $$(lib))$$(LIBODIR)/$$(notdir $$(lib)))
DEP_OBJS_$(1) = $$(foreach obj,$$(filter %.o,$$(COMPONENTS_$(1))),$$(dir $$(obj))$$(OBJODIR)/$$(notdir $$(obj)))
$$(LIBODIR)/$(1).a: $$(OBJS) $$(DEP_OBJS_$(1)) $$(DEP_LIBS_$(1)) $$(DEPENDS_$(1))
	@mkdir -p $$(LIBODIR)
	@$$(if $$(filter %.a,$$?),mkdir -p $$(EXTRACT_DIR)_$(1))
	@$$(if $$(filter %.a,$$?),cd $$(EXTRACT_DIR)_$(1); $$(foreach lib,$$(filter %.a,$$?),$$(AR) xo $$(UP_EXTRACT_DIR)/$$(lib);))
	@$$(AR) ru $$@ $$(filter %.o,$$?) $$(if $$(filter %.a,$$?),$$(EXTRACT_DIR)_$(1)/*.o)
	@$$(if $$(filter %.a,$$?),$$(RM) -r $$(EXTRACT_DIR)_$(1))
endef

define MakeImage
DEP_LIBS_$(1) = $$(foreach lib,$$(filter %.a,$$(COMPONENTS_$(1))),$$(dir $$(lib))$$(LIBODIR)/$$(notdir $$(lib)))
DEP_OBJS_$(1) = $$(foreach obj,$$(filter %.o,$$(COMPONENTS_$(1))),$$(dir $$(obj))$$(OBJODIR)/$$(notdir $$(obj)))
$$(IMAGEODIR)/$(1).out: $$(OBJS) $$(DEP_OBJS_$(1)) $$(DEP_LIBS_$(1)) $$(DEPENDS_$(1))
	@mkdir -p $$(IMAGEODIR)
	$$(CC) $$(LDFLAGS) $$(if $$(LINKFLAGS_$(1)),$$(LINKFLAGS_$(1)),$$(LINKFLAGS_DEFAULT) $$(OBJS) $$(DEP_OBJS_$(1)) $$(DEP_LIBS_$(1))) -o $$@
endef

$(BINODIR)/%.bin: $(IMAGEODIR)/%.out
	@echo "------------------------------------------------------------------------------"
	@mkdir -p ../$(FIRMWAREDIR)
	@$(ESPTOOL) elf2image -o ../$(FIRMWAREDIR)/ $(flashimageoptions) $<
	@echo "------------------------------------------------------------------------------"
	@echo "Add rapid_loader..."
	@mv -f ../bin/$(ADDR_FW1).bin ../bin/0.bin 
ifeq ($(freqdiv), 15)	
	@dd if=../bin/rapid_loader.bin >../bin/$(ADDR_FW1).bin
else
	@dd if=../bin/rapid_loader_40m.bin >../bin/$(ADDR_FW1).bin
endif	
	@dd if=../bin/0.bin >>../bin/$(ADDR_FW1).bin

all: .subdirs $(OBJS) $(OLIBS) $(SPECIAL_MKTARGETS) $(OIMAGES) $(OBINS) 

$(SPECIAL_MKTARGETS): $(INPLIB) 
	@$(RM) -f $@
	@mkdir -p _temp
	cd _temp; $(AR) xo ../$<; $(foreach lib,$(ADDLIBS_libsdk),$(AR) xo ../$(ADDLIBDIR)$(lib);)
	@$(AR) ru $@ _temp/*.o
	@$(RM) -r _temp

clean:
	@$(foreach d, $(SUBDIRS), $(MAKE) -C $(d) clean;)
	@$(RM) -r $(ODIR)/$(TARGET)
	@$(RM) -f lib/libsdk.a

clobber: $(SPECIAL_CLOBBER)
	@$(foreach d, $(SUBDIRS), $(MAKE) -C $(d) clobber;)
	@$(RM) -r $(ODIR)
	@$(RM) -f lib/libsdk.a

FlashUserFiles: $(USERFBIN)
	$(ESPTOOL) $(ESPOPTION) write_flash $(flashimageoptions) $(USERFADDR) $(USERFBIN)

FlashAll: $(OUTBIN1)  $(USERFBIN) $(OUTBIN2) $(DEFAULTBIN) $(BLANKBIN) $(CLREEPBIN)
	$(ESPTOOL) $(ESPOPTION) write_flash $(flashimageoptions) $(ADDR_FW1) $(OUTBIN1) $(ADDR_FW2) $(OUTBIN2) $(USERFADDR) $(USERFBIN)  $(CLREEPADDR) $(CLREEPBIN) $(DEFAULTADDR) $(DEFAULTBIN) $(BLANKADDR) $(BLANKBIN)

FlashClearSetings: $(CLREEPBIN) $(DEFAULTBIN) $(BLANKBIN)
	$(ESPTOOL) $(ESPOPTION) write_flash $(flashimageoptions) $(CLREEPADDR) $(CLREEPBIN) $(DEFAULTADDR) $(DEFAULTBIN) $(BLANKADDR) $(BLANKBIN)

FlashCode: $(OUTBIN1) $(OUTBIN2)
	$(ESPTOOL) $(ESPOPTION) write_flash $(flashimageoptions) $(ADDR_FW1) $(OUTBIN1) $(ADDR_FW2) $(OUTBIN2)

$(USERFBIN):
	./WEBFS22.exe -h "*.htm, *.html, *.cgi, *.xml, *.bin, *.txt, *.wav" -z "*.inc, snmp.bib" ./WEBFiles ./webbin WEBFiles.bin

.subdirs:
	@set -e; $(foreach d, $(SUBDIRS), $(MAKE) -C $(d);)

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(MAKECMDGOALS),clobber)
ifdef DEPS
sinclude $(DEPS)
endif
endif
endif

$(OBJODIR)/%.o: %.c
	@mkdir -p $(OBJODIR);
	$(CC) $(if $(findstring $<,$(DSRCS)),$(DFLAGS),$(CFLAGS)) $(COPTS_$(*F)) -o $@ -c $< 

$(OBJODIR)/%.d: %.c
	@mkdir -p $(OBJODIR);
	@set -e; rm -f $@; \
	$(CC) -M $(CFLAGS) $< > $@.$$$$; \
	sed 's,\($*\.o\)[ :]*,$(OBJODIR)/\1 $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

$(OBJODIR)/%.o: %.s
	@mkdir -p $(OBJODIR);
	$(CC) $(CFLAGS) -o $@ -c $<

$(OBJODIR)/%.d: %.s
	@mkdir -p $(OBJODIR); \
	set -e; rm -f $@; \
	$(CC) -M $(CFLAGS) $< > $@.$$$$; \
	sed 's,\($*\.o\)[ :]*,$(OBJODIR)/\1 $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

$(OBJODIR)/%.o: %.S
	@mkdir -p $(OBJODIR);
	$(CC) $(CFLAGS) -D__ASSEMBLER__ -o $@ -c $<

$(OBJODIR)/%.d: %.S
	@mkdir -p $(OBJODIR); \
	set -e; rm -f $@; \
	$(CC) -M $(CFLAGS) $< > $@.$$$$; \
	sed 's,\($*\.o\)[ :]*,$(OBJODIR)/\1 $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

$(foreach lib,$(GEN_LIBS),$(eval $(call ShortcutRule,$(lib),$(LIBODIR))))

$(foreach image,$(GEN_IMAGES),$(eval $(call ShortcutRule,$(image),$(IMAGEODIR))))

$(foreach bin,$(GEN_BINS),$(eval $(call ShortcutRule,$(bin),$(BINODIR))))

$(foreach lib,$(GEN_LIBS),$(eval $(call MakeLibrary,$(basename $(lib)))))

$(foreach image,$(GEN_IMAGES),$(eval $(call MakeImage,$(basename $(image)))))

INCLUDES := $(INCLUDES) -I $(PDIR)include
#PDIR := ../$(PDIR)
#sinclude $(PDIR)Makefile
