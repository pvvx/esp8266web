OUTNAME = iointr.ovl
TARGET = eagle

ifndef PDIR # {
GEN_IMAGES= eagle.app.v6.out
GEN_BINS = eagle.app.v6.bin

SUBDIRS = main
	
endif # } PDIR

APPDIR = .
LDDIR = $(WEB_BASE)/ld

LD_FILE = $(LDDIR)/overlay.ld
DEPENDS_eagle.app.v6 = \
                $(LD_FILE) \
                $(LDDIR)/overlay.ld

COMPONENTS_eagle.app.v6 = \
	main/libmain.a
	
LINK_LIBS = \
    -lmgcc
    

LINKFLAGS_eagle.app.v6 = \
	-nostartfiles	\
	-nodefaultlibs	\
	-nostdlib	\
	-L$(WEB_BASE)/lib	\
    -T$(LD_FILE) \
	-Wl,--no-check-sections	\
    -u call_user_start \
	-Wl,-static \
	-Wl,-Map -Wl,$(@:.out=.map) \
	-Wl,--start-group \
	$(LINK_LIBS)	\
	$(DEP_LIBS_eagle.app.v6) \
	-Wl,--end-group

CONFIGURATION_DEFINES +=	\
			-DICACHE_FLASH	\
			-DPBUF_RSV_FOR_WLAN	\
			-DLWIP_OPEN_SRC	\
			-DEBUF_LWIP


DEFINES +=	$(CONFIGURATION_DEFINES)

DDEFINES +=	$(CONFIGURATION_DEFINES)

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
PDIR := ../$(PDIR)
sinclude $(PDIR)Makefile


.PHONY: FORCE
FORCE:
