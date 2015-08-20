include *.mk

SUBDIRS=src pal etc asm

all: $(SUBDIRS)
	make -C src all
	make -C pal all
	make -C asm all

dist: all $(SUBDIRS)
	mkdir -p dist/$(DIST)/bin
	mkdir -p dist/$(DIST)/etc
	make -C src dist
	make -C pal dist
	make -C asm dist
	make -C etc dist
	make -C dist dist

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
