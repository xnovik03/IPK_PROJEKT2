CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -O2

# Add debug print macro definition
CXXFLAGS += -DDEBUG_PRINT

SRCS := $(wildcard *.cpp)
OBJS := $(SRCS:.cpp=.o)

BIN = ipk25chat-client

all: $(BIN)
# Link object files into the final binary
$(BIN): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

clean:
	rm -f $(OBJS) $(BIN)

.PHONY: all clean
