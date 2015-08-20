include *.mk

SUBDIRS=src pal

all: $(SUBDIRS)
	make -C src all
	make -C pal all

install: $(SUBDIRS)
	$(INSTALL) $(INSTALL_OPTS) $(INSTALL_DOPTS) -d $(BIN_DIR) $(ETC_DIR)
	make -C src install
	make -C pal install
	make -C etc install

clean: $(SUBDIRS)
	make -C src clean
	make -C pal clean
	
.PHONY: all $(SUBDIRS)
