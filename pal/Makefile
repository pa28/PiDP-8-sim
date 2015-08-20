include *.mk

BINS=DeepThought.bin Test.bin SGDT.bin

all: $(BINS)

install: $(BINS)
	$(INSTALL) $(INSTALL_OPTS) $(INSTALL_FOPTS) $(BINS) $(ETC_DIR)

dist: $(BINS)
	cp $(BINS) ../dist/$(DIST)/etc

clean:
	$(RM) *.bin *.lst

%.bin: %.pal
	palbart -d $<
