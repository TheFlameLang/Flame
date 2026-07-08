#include "include/ast.h"
#include "include/generator.h"
#include "include/lexer.h"
#include "include/parser.h"
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>

bool error_exit = false;

int main(int argc, char *argv[]) {
  if (argc > 1) {
    if (strcmp(argv[1], "-v") == 0) {
      std::cout << "Flame compiler 0.7\nBy Naharashu\n";
      return 0;
    }
    if (strcmp(argv[1], "-h") == 0) {
      std::cout << "-o [filename] - output file name(executable), by default - out\n";
      std::cout << "-v - version of compiler\n";
      std::cout << "-C - generate only C++ code\n";
      std::cout << "-lexer-debug - output lexed tokens\n";
      std::cout << "-O3 - generate C++ with -O3\n";
      std::cout << "-O0 - generate C++ with -O0\n";
      std::cout << "-O4 - generate C++ with -O3, -funroll-loops and -march=native\n";
      return 0;
    }
  }
  bool compile_into_bin = true;
  std::string out_name = "out";
  bool lexer_output = false;
  bool fast_code = false;
  bool super_fast_code = false;
  bool slow_code = false;
  for (int i = 0; i < argc; i++) {
    if (strcmp(argv[i], "-C") == 0)
      compile_into_bin = false;
    if (strcmp(argv[i], "-o") == 0)
      out_name = argc > i + 1 ? argv[i + 1] : "out";
    if (strcmp(argv[i], "-lexer-debug") == 0)
      lexer_output = true;
    if (strcmp(argv[i], "-O2") == 0)
      fast_code = true;
    if (strcmp(argv[i], "-O0") == 0)
      slow_code = true;
    if (strcmp(argv[i], "-O3") == 0)
      super_fast_code = true;
  }
  lexer lex;
  std::string code;
  code.clear();
  if (argc < 2) {
    std::cerr << "Usage: flame [file.flame] {args}\n";
    std::cerr << "Type ./flame -h for more details\n";
    return 1;
  }
  std::string line;
  const char* filename = argv[1];
  std::ifstream file(filename);
  while (std::getline(file, line)) {
    code += line + '\n';
  }
  file.close();
  std::vector<token> toks = lex.lex(code);
  parser parser_(toks, filename);
  u64 i = 0;
  if (lexer_output) {
    for (auto &tok : toks) {
      std::cout << '(' << i << ") " << disassemble_tok_type(tok.type) << '\n';
      i++;
    }
  }
  std::vector<astptr> res;
  res.reserve(toks.size());
  try {
    res = parser_.parse();
  } catch (ParseTimeError& e) {
    std::cout << "\x1b[0;35m" << filename << ':' << parser_.line << ':' << parser_.column << ": \x1b[31m error\n" <<  e.what() << "\x1b[0m";
    return 1;
  }
  if(parser_.errors) return 1;
  generator gen;
  std::string code_;
  try {
    code_ = gen.generate(res);
  } catch (TranspileTimeError& e) {
    std::cout << "\x1b[0;35m" << filename << ':' << gen.line << ':' << gen.column << ": \x1b[31m error\n" <<  e.what() << "\x1b[0m";
    return 1;
  }
  std::ofstream out("temp_flame.cpp", std::ios::out | std::ios::binary);
  if (!out.is_open()) {
    std::cerr << "E: Cannot open temp file to generate cpp code\n";
    return 1;
  }
  out.write(code_.c_str(), (long)code_.size());
  out.close();
  if (compile_into_bin) {
    std::string cleanup = "rm -f ";
    cleanup += out_name;
    system(cleanup.c_str());
    std::string output;
    if(slow_code) output = "g++ -O0 temp_flame.cpp -o " + out_name;
    else if(super_fast_code) output = "g++ -O3 -funroll-loops -march=native temp_flame.cpp -o " + out_name;
    else output = !fast_code ? "g++ temp_flame.cpp -o " + out_name : "g++ -O2 temp_flame.cpp -o " + out_name;
    system(output.c_str());
  }
  // std::cout << code_ << std::endl;
  return 0;
}
