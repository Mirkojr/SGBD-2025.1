CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2

SRCS = main.cpp 
OBJDIR = obj
OBJS = $(SRCS:%.cpp=$(OBJDIR)/%.o)
TARGET = main

all: $(OBJDIR) $(TARGET)

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(OBJDIR)/%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf $(OBJDIR) $(TARGET)

	#deletar arquivos de saida exceto in.txt
	find . -maxdepth 1 -type f -name '*.txt' ! -name 'in.txt' -exec rm -f {} + 