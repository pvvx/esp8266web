#############################################################
# Required variables for each makefile
# Discard this section from all parent makefiles
# Expected variables (with automatic defaults):
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

UP_EXTRACT_DIR = ..

COMPONENTS_libsdk = \
	system/libaddmmain.a	\
	phy/libaddmphy.a	\
	pp/libaddpp.a	\
	wpa/libaddwpa.a
		
ifdef USE_OPEN_LWIP
COMPONENTS_libsdk += lwip/liblwip.a	
CONFIGURATION_DEFINES += -DUSE_OPEN_LWIP
endif

ifdef USE_OPEN_DHCPS
CONFIGURATION_DEFINES += -DUSE_OPEN_DHCPS
endif 

ifndef PDIR # {
GEN_LIBS = libsdk.a
SUBDIRS =	\
	system	\
	phy	\
	pp	\
	wpa
		
ifdef USE_OPEN_LWIP
SUBDIRS += lwip
endif

ADDLIBDIR = ../../lib/
SPECIAL_MKTARGETS = $(ADDLIBDIR)libsdk.a
INPLIB = .output/eagle/lib/libsdk.a

ADDLIBS_libsdk = \
	libmgcc.a	\
	libmmain.a	\
	libmphy.a	\
	libpp.a	\
	libmwpa.a	\
	libcrypto.a	\
	libnet80211.a

ifndef USE_OPEN_LWIP 
ADDLIBS_libsdk += \
	liblwipif.a	\
	libmlwip.a	
endif

ifndef USE_OPEN_DHCPS
ADDLIBS_libsdk += libdhcps.a
endif

endif # } PDIR

APPDIR = .

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

# INCLUDES := $(INCLUDES) -I $(PDIR)include
PDIR := ../$(PDIR)
sinclude $(PDIR)Makefile


.PHONY: FORCE
FORCE:
