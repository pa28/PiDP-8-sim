CXX        := g++
LD         := $(CXX)
BINS       := pidp8
BINSDIR    := .
CFLAGS     := -g -Wall -DHAS_PSELECT -std=gnu++0x
pidp8_SRCS := Chassis.cpp Console.cpp CPU.cpp Device.cpp Memory.cpp Panel.cpp Thread.cpp Terminal.cpp VirtualPanel.cpp
pidp8_OBJS := $(patsubst %.cpp,%.o,$(pidp8_SRCS))
pidp8_LIBS := -lpthread -lncurses -lrt
pidp8_DEPS := $(pidp8_OBJS:.o=.d)

OBJS := $(pidp8_OBJS)
DEPS := $(pidp8_DEPS)

all: $(BINS)

clean:
	$(RM) $(BINS) $(OBJS) $(DEPS)

pidp8: $(pidp8_OBJS)
	$(CXX) $(CFLAGS) -o $@ $(pidp8_OBJS) $(pidp8_LIBS)

%.o: %.cpp
	$(CXX) -c -MMD -MP $< -o $@ $(CFLAGS)

-include $(DEPS)
