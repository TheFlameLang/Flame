CXX = g++
CXX_FLAGS = -std=c++20 -g -O1 -Wall -Wextra
CXX_OBJ = build/lexer.o build/parser.o build/main.o build/ast.o build/table.o
HOME_ = $(HOME)

clang: CXX = clang++
clang: main

all: main

debug: CXX_FLAGS = -std=gnu++20 -g3 -O0 -Wall -Wextra
debug: main

main: $(CXX_OBJ)
	$(CXX) $(CXX_OBJ) -flto=thin -o flame

build/%.o: src/%.cpp
	$(CXX) $(CXX_FLAGS) -c $< -o $@

install:
	mv flame $(HOME_)/.local/bin/flame
	mkdir -p $(HOME_)/.local/bin/flame_
	cp -r stdlib/. $(HOME_)/.local/bin/flame_/

uninstall:
	rm -f $(HOME_)/.local/bin/flame
	rm -rf $(HOME_)/.local/bin/flame_/*

clean:
	rm -f flame
	rm -f build/*.o *.o
	rm -f temp_flame.cpp
