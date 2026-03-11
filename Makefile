# CXX=g++
# CC=gcc

CXXFLAGS+=-Wall -Wextra -Werror
CXXFLAGS+=-O3
CXXFLAGS+=-fopenmp

CFLAGS+=$(CXXFLAGS)

C_FILES=$(wildcard *.c)
CPP_FILES=$(wildcard *.cpp)

TARGETS=$(C_FILES:%.c=bin/%) $(CPP_FILES:%.cpp=bin/%)

# Assignment1 specific config
OMPFLAGS = -fopenmp
TARGET = Assignment1

# Default source file (can be overridden: make SRC=multmatrix.cpp or make SRC=multmatrix_v2.cpp)
SRC ?= multmatrix_v2.cpp

all: $(TARGETS) $(TARGET)

bin/%: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $< -o $@

bin/%: %.c
	@mdkir -p $(dir $@)
	$(CC) $(CFLAGS) $< -o $@

asm/%.asm: %.cpp
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -S $< -o $@

# Rule to build Assignment1
$(TARGET): $(SRC)
	$(CXX) $(ASSIGNMENT_CXXFLAGS) $(OMPFLAGS) -o $(TARGET) $(SRC)

# Convenience targets
v1: SRC = multmatrix.cpp
v1: $(TARGET)

v2: SRC = multmatrix_v2.cpp
v2: $(TARGET)

clean:
	rm -rf bin >/dev/null 2>&1 || true
	rm -f $(TARGET)

.PHONY: clean v1 v2