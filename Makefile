SUBDIRS=src

all: $(SUBDIRS)
	make -C $(SUBDIRS)

clean: $(SUBDIRS)
	make -C $(SUBDIRS)
	
.PHONY: all $(SUBDIRS)
