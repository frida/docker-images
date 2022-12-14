ifndef QCONFIG
QCONFIG=qconfig.mk
endif
include $(QCONFIG)
INSTALLDIR=usr/photon/bin

define PINFO
PINFO DESCRIPTION=
endef

USEFILE=$(PROJECT_ROOT)/$(SECTION)/devi-$(SECTION).use

EXTRA_SILENT_VARIANTS = $(subst -, ,$(SECTION))
NAME=$(PROJECT)-$(SECTION)

LIBPREF_ph = -Bstatic
LIBPOST_ph = -Bdynamic
LIBS += input keymap ph gf

PUBLIC_INCVPATH+=$(PROJECT_ROOT)/private $(PROJECT_ROOT)/public

SERVICES_ROOT=$(PRODUCT_ROOT)/../services
LIB_VARIANT=$(subst dll,o,$(COMPOUND_VARIANT))

# Local modifications of make environment
-include ../../../private.mk

include $(MKFILES_ROOT)/qmacros.mk
include $(PROJECT_ROOT)/$(SECTION)/pinfo.mk

-include $(PROJECT_ROOT)/roots.mk
ifndef DEVILIB_ROOT
DEVILIB_ROOT=$(PROJECT_ROOT)/lib
endif
ifndef KEYMAP_ROOT
KEYMAP_ROOT=$(PROJECT_ROOT)/keymap
endif

EXTRA_LIBVPATH+=$(SERVICES_ROOT)/usb/usbdi/$(subst x86/so.,x86/so,$(CPU)/so.$(filter le be,$(subst ., ,$(VARIANTS))))
EXTRA_LIBVPATH+=$(SERVICES_ROOT)/hid/hiddi/$(subst x86/so.,x86/so,$(CPU)/so.$(filter le be,$(subst ., ,$(VARIANTS))))
EXTRA_INCVPATH+=$(PROJECT_ROOT)/include $(PROJECT_ROOT)/../../lib/ph
EXTRA_INCVPATH+=$(INSTALL_ROOT_nto)/usr/include/xilinx
EXTRA_LIBVPATH+=$(INSTALL_ROOT_nto)/usr/lib/xilinx

EXTRA_LIBVPATH+=$(DEVILIB_ROOT)/$(OS)/$(CPU)/a.$(COMPOUND_VARIANT) $(KEYMAP_ROOT)/$(OS)/$(CPU)/a.$(COMPOUND_VARIANT)
EXTRA_LIBVPATH+=$(DEVILIB_ROOT)/$(OS)/$(CPU)/a.$(COMPOUND_VARIANT).shared $(KEYMAP_ROOT)/$(OS)/$(CPU)/a.$(COMPOUND_VARIANT).shared



#####AUTO-GENERATED by packaging script... do not checkin#####
   INSTALL_ROOT_nto = $(PROJECT_ROOT)/../../../install
   USE_INSTALL_ROOT=1
##############################################################

include $(MKFILES_ROOT)/qtargets.mk



