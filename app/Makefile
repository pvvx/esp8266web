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

ifndef PDIR # {
GEN_IMAGES= eagle.app.v6.out
GEN_BINS = eagle.app.v6.bin

SUBDIRS =	\
	gdbstub	\
	sdklib	\
	user	\
	driver	\
	mdbtcp	\
	web
	
endif # } PDIR

APPDIR = .
LDDIR = ../ld

LD_FILE = $(LDDIR)/eagle.app.v6.ld

DEPENDS_eagle.app.v6 = \
                $(LD_FILE) \
                $(LDDIR)/eagle.rom.addr.v6.ld

COMPONENTS_eagle.app.v6 = \
	gdbstub/libgdbstub.a	\
	driver/libdriver.a \
	web/libweb.a	\
	mdbtcp/libmdbtcp.a \
	user/libuser.a \
	sdklib/libsdk.a

LINK_LIBS = \
    -lsdk
    
#    -lmicroc \

LINKFLAGS_eagle.app.v6 = \
	-nostartfiles	\
	-nodefaultlibs	\
	-nostdlib	\
	-L../lib	\
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
#INCLUDES += -I ./
PDIR := ../$(PDIR)
sinclude $(PDIR)Makefile


.PHONY: FORCE
FORCE:
