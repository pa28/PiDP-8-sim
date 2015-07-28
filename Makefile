SUBDIRS=src

all: $(SUBDIRS)
	make -C $@
	
.PHONY: all $(SUBDIRS)
