CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2

# Lista de arquivos fonte
SRCS = main.cpp bplustree.cpp 
OBJS = $(SRCS:.cpp=.o)
OBJS := $(OBJS:.c=.o)
TARGET = main

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $<

%.o: %.c
	$(CXX) $(CXXFLAGS) -c $<

clean:
	rm -f $(OBJS) $(TARGET)