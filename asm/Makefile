include *.mk

all: palbart 


install: palbart
	$(INSTALL) --owner=bin --group=bin --mode=0755 palbart /usr/local/bin
	gzip -c palbart.1 > /tmp/palbart.1.gz
	$(INSTALL) --owner=root --group=root --mode=0644 /tmp/palbart.1.gz /usr/share/man/man1
	$(RM) /tmp/palbart.1.gz

clean:
	$(RM) palbart

palbart: palbart.c
	gcc -O -Wall -o $@ $<
