BINS=DeepThought.bin Test.bin SGDT.bin

all: $(BINS)

clean:
	$(RM) *.bin *.lst

%.bin: %.pal
	palbart -d $<
