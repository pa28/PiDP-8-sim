BINS=pidp8 
BINSDIR=.
pidp8_OBJS=Chassis.o Console.o CPU.o Device.o Memory.o Panel.o Thread.o
pidp8_LIBS=-lpthread -lncurses

all: $(BINS)

clean:
	rm -f $(BINS) $(pidp8_OBJS)

pidp8: $(pidp8_OBJS)
	g++ -g -Wall -o $@ $(pidp8_OBJS) $(pidp8_LIBS)

%.o: %.cpp
	g++ -g -Wall -c $<
