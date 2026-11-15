CXX = g++
CXXFLAGS = -O2 -march=x86-64 -pipe
HOME_ = $(HOME)

all: main

main:
	$(CXX) $(CXXFLAGS) main.cpp -o shiver

install:
	cp shiver $(HOME_)/.local/bin/shiver
