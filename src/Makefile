BINS=pidp8
BINSDIR=.
CFLAGS=-g -Wall -DHAS_PSELECT
pidp8_OBJS=Chassis.o Console.o CPU.o Device.o Memory.o Panel.o Thread.o Terminal.o
pidp8_LIBS=-lpthread -lncurses -lrt

all: $(BINS)

clean:
	rm -f $(BINS) $(pidp8_OBJS)

pidp8: $(pidp8_OBJS)
	g++ $(CFLAGS) -o $@ $(pidp8_OBJS) $(pidp8_LIBS)

nc: nc.cpp
	g++  $(CFLAGS) -o $@ $< $(pidp8_LIBS)

%.o: %.cpp
	g++ $(CFLAGS) -c $<
