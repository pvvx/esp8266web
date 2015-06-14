#############################################################
# Required variables for each makefile
# Discard this section from all parent makefiles
# Expected variables (with automatic defaults):
#   CSRCS (all "C" files in the dir)
#   SUBDIRS (all subdirs with a Makefile)
#   GEN_LIBS - list of libs to be generated ()
#   GEN_IMAGES - list of object file images to be generated ()
#   GEN_BINS - list of binaries to be generated ()
#   COMPONENTS_xxx - a list of libs/objs in the form
#     subdir/lib to be extracted and rolled up into
#     a generated lib/image xxx.a ()
#
TARGET = eagle

USE_OPEN_LWIP = 140


#EXTRA_CCFLAGS += -u

ifndef PDIR # {
GEN_IMAGES= eagle.app.v6.out
GEN_BINS = eagle.app.v6.bin

ifndef USE_OPEN_LWIP
SUBDIRS =	\
	main	\
	phy		\
	user	\
	driver	\
	web
else
SUBDIRS =	\
	main	\
	phy		\
	user	\
	driver	\
	lwip	\
	web
endif
	
endif # } PDIR

APPDIR = .
LDDIR = ../ld

LD_FILE = $(LDDIR)/eagle.app.v6.ld

ifeq ($(APP), 1)
	LD_FILE = $(LDDIR)/eagle.app.v6.app1.ld
endif

ifeq ($(APP), 2)
	LD_FILE = $(LDDIR)/eagle.app.v6.app2.ld
endif

ifndef USE_OPEN_LWIP

COMPONENTS_eagle.app.v6 = \
	main/libaddmmain.a	\
	phy/libaddmphy.a	\
	driver/libdriver.a \
	web/libweb.a	\
	user/libuser.a
	
LINKFLAGS_eagle.app.v6 = \
	-L../lib	\
	-nostdlib \
    -T$(LD_FILE) \
	-Wl,--no-check-sections	\
    -u call_user_start \
	-Wl,-static \
	-Wl,-Map -Wl,$(@:.out=.map) \
	-Wl,--start-group \
	-lmc	\
	-lmgcc	\
	-lmphy	\
	-lpp	\
	-lnet80211 \
	-lwpa	\
	-llwipif	\
	-lmlwip	\
	-lmmain	\
	$(DEP_LIBS_eagle.app.v6) \
	-Wl,--end-group

CONFIGURATION_DEFINES =	-D__ets__ \
			-DICACHE_FLASH	\
			-DPBUF_RSV_FOR_WLAN	\
			-DLWIP_OPEN_SRC	\
			-DEBUF_LWIP

else	

COMPONENTS_eagle.app.v6 = \
	main/libaddmmain.a	\
	phy/libaddmphy.a	\
	driver/libdriver.a \
	web/libweb.a	\
	lwip/liblwip.a \
	user/libuser.a
	
LINKFLAGS_eagle.app.v6 = \
	-L../lib	\
	-nostdlib \
    -T$(LD_FILE) \
	-Wl,--no-check-sections	\
    -u call_user_start \
	-Wl,-static \
	-Wl,-Map -Wl,$(@:.out=.map) \
	-Wl,--start-group \
	-lmc	\
	-lmgcc	\
	-lmphy	\
	-lpp	\
	-lnet80211 \
	-lwpa	\
	-lmmain	\
	$(DEP_LIBS_eagle.app.v6) \
	-Wl,--end-group

CONFIGURATION_DEFINES =	-D__ets__ \
			-DICACHE_FLASH	\
			-DPBUF_RSV_FOR_WLAN	\
			-DLWIP_OPEN_SRC	\
			-DEBUF_LWIP	\
			-DUSE_OPEN_LWIP

endif 

DEPENDS_eagle.app.v6 = \
                $(LD_FILE) \
                $(LDDIR)/eagle.rom.addr.v6.ld



DEFINES +=				\
	$(UNIVERSAL_TARGET_DEFINES)	\
	$(CONFIGURATION_DEFINES)

DDEFINES +=				\
	$(UNIVERSAL_TARGET_DEFINES)	\
	$(CONFIGURATION_DEFINES)


#############################################################
# Recursion Magic - Don't touch this!!
#
# Each subtree potentially has an include directory
#   corresponding to the common APIs applicable to modules
#   rooted at that subtree. Accordingly, the INCLUDE PATH
#   of a module can only contain the include directories up
#   its parent path, and not its siblings
#
# Required for each makefile to inherit from the parent
#

INCLUDES := $(INCLUDES) -I $(PDIR)include
INCLUDES += -I ./
PDIR := ../$(PDIR)
sinclude $(PDIR)Makefile


.PHONY: FORCE
FORCE:
