# CXX=g++
# CC=gcc

CXXFLAGS+=-Wall -Wextra -Werror
CXXFLAGS+=-O3
CXXFLAGS+=-fopenmp

SRC=multmatrix_v2.cpp
TARGET=Assignment1

$(TARGET): $(SRC) Makefile
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET)

clean:
	rm -f $(TARGET)
