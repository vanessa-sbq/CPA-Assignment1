# CXX=g++
# CC=gcc

CXXFLAGS+=-Wall -Wextra -Werror
CXXFLAGS+=-O3
CXXFLAGS+=-fopenmp

CFLAGS+=$(CXXFLAGS)

C_FILES=$(wildcard *.c)
CPP_FILES=$(wildcard *.cpp)

TARGETS=$(C_FILES:%.c=bin/%) $(CPP_FILES:%.cpp=bin/%)

all: $(TARGETS)

bin/%: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $< -o $@

bin/%: %.c
	@mdkir -p $(dir $@)
	$(CC) $(CFLAGS) $< -o $@

asm/%.asm: %.cpp
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -S $< -o $@

clean:
	rm -rf bin >/dev/null 2>&1 || true
