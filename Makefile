SUBDIRS=src pal

all: $(SUBDIRS)
	make -C src all
	make -C pal all

clean: $(SUBDIRS)
	make -C src clean
	make -C pal clean
	
.PHONY: all $(SUBDIRS)
