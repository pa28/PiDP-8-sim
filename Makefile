SUBDIRS=src

all: $(SUBDIRS)
	make -C $(SUBDIRS) all

clean: $(SUBDIRS)
	make -C $(SUBDIRS) clean
	
.PHONY: all $(SUBDIRS)
