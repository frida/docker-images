ROOT_DIR := $(notdir $(CURDIR))
ifndef QCONFIG
QCONFIG=qconfig.mk
endif
include $(QCONFIG)

HOST_MKIFS := mkifs

SUFFIXES := .build .ifs .elf

.PHONY: all clean

all: ifs-mx6q-sabrelite.elf

clean: 
	$(RM_HOST) ifs-mx6q-sabrelite.elf ipl-mx6q-sabrelite.bin

ifs-mx6q-sabrelite.elf: sabrelite.build
	$(HOST_MKIFS) -vvv -r../install $(MKIFSFLAGS) $^ $@
