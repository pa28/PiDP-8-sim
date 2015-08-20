include *.mk

SUBDIRS=src pal etc asm

all: $(SUBDIRS)
	make -C src all
	make -C pal all
	make -C asm all

install: $(SUBDIRS)
	$(INSTALL) $(INSTALL_OPTS) $(INSTALL_DOPTS) -d $(BIN_DIR) $(ETC_DIR)
	make -C src install
	make -C pal install
	make -C etc install
	make -C asm install

clean: $(SUBDIRS)
	make -C src clean
	make -C pal clean
	make -C asm clean
	
.PHONY: all $(SUBDIRS)
