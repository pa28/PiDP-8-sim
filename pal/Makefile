BINS=DeepThought.bin  

all: $(BINS)

clean:
	$(RM) *.bin *.lst

%.bin: %.pal
	palbart -d $<
