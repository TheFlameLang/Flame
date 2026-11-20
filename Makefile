CXX = g++
CXX_FLAGS = -std=c++20 -g -O1 -Wall -Wextra
CXX_OBJ = build/lexer.o build/parser.o build/main.o build/ast.o build/table.o build/ast_c.o
HOME_ = $(HOME)

all: main

debug: CXX_FLAGS = -std=gnu++20 -g3 -O0 -Wall -Wextra
debug: main

main: $(CXX_OBJ)
	$(CXX) $(CXX_OBJ) -flto=thin -o flame

build/%.o: src/%.cpp
	mkdir -p build
	$(CXX) $(CXX_FLAGS) -c $< -o $@

install:
	cp flame /usr/bin/flame
	mkdir -p /usr/bin/flame_
	cp -r stdlib/. /usr/bin/flame_/
	sudo cp include/runtime/flame_rt.h /usr/include/flame_rt.h

uninstall:
	rm -f /usr/bin/flame
	rm -rf /usr/bin/flame_/*

clean:
	rm -f flame
	rm -f build/*.o *.o
	rm -f temp_flame.cpp
