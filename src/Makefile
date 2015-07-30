BINS=pidp8 
BINSDIR=.
pidp8_OBJS=Chassis.o Console.o CPU.o Device.o Memory.o Panel.o Thread.o
pidp8_LIBS=-lpthread

all: $(BINS)

clean:
	rm -f $(BINS) $(pidp8_OBJS)

install: install-pal install-bin

install-pal: palbart
	install --owner=bin --group=bin --mode=0755 palbart /usr/local/bin
	gzip -c palbart.1 > palbart.1.gz
	install --owner=root --group=root --mode=0644 palbart.1.gz /usr/share/man/man1
	rm -f palbart.1.gz

install-bin:
	install -d --mode=0755 $(BINSDIR)
	install --mode=0644 $(BINS) $(BINSDIR)

pidp8: $(pidp8_OBJS)
	g++ -g -Wall -o $@ $(pidp8_OBJS) $(pidp8_LIBS)

%.o: %.cpp
	g++ -g -Wall -c $<
