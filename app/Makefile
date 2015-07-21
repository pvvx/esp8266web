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
USE_OPEN_DHCPS = 1


ifndef PDIR # {
GEN_IMAGES= eagle.app.v6.out
GEN_BINS = eagle.app.v6.bin

SUBDIRS =	\
	system	\
	phy		\
	wpa		\
	user	\
	driver	\
	mdbtcp	\
	web


ifdef USE_OPEN_LWIP
SUBDIRS += lwip
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

COMPONENTS_eagle.app.v6 = \
	system/libaddmmain.a	\
	phy/libaddmphy.a	\
	wpa/libaddwpa.a	\
	driver/libdriver.a \
	web/libweb.a	\
	mdbtcp/libmdbtcp.a \
	user/libuser.a

ifdef USE_OPEN_LWIP
COMPONENTS_eagle.app.v6 +=	lwip/liblwip.a 	
endif

LINK_LIBS = \
    -lmicroc \
	-lmgcc	\
	-lmphy	\
	-lpp	\
	-lnet80211 \
	-lmwpa	\
	-lmmain	

ifndef USE_OPEN_LWIP 
LINK_LIBS += \
	-llwipif	\
	-lmlwip	
endif

ifndef USE_OPEN_DHCPS
LINK_LIBS += -ldhcps
endif
	
LINKFLAGS_eagle.app.v6 = \
	-L../lib	\
	-nostdlib \
    -T$(LD_FILE) \
	-Wl,--no-check-sections	\
    -u call_user_start \
	-Wl,-static \
	-Wl,-Map -Wl,$(@:.out=.map) \
	-Wl,--start-group \
	$(LINK_LIBS)	\
	$(DEP_LIBS_eagle.app.v6) \
	-Wl,--end-group

CONFIGURATION_DEFINES =	-D__ets__ \
			-DICACHE_FLASH	\
			-DPBUF_RSV_FOR_WLAN	\
			-DLWIP_OPEN_SRC	\
			-DEBUF_LWIP

ifdef USE_OPEN_LWIP
CONFIGURATION_DEFINES +=-DUSE_OPEN_LWIP
endif 

ifdef USE_OPEN_DHCPS
CONFIGURATION_DEFINES +=-DUSE_OPEN_DHCPS
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
