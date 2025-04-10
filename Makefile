
CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -O2

SRCS := $(wildcard *.cpp)
OBJS := $(SRCS:.cpp=.o)

BIN = ipk25chat-client

all: $(BIN)

$(BIN): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

clean:
	rm -f $(OBJS) $(BIN)

.PHONY: all clean
