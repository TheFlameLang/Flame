#include "../include/ast.h"
#include "../include/generator.h"
#include "../include/lexer.h"
#include "../include/parser.h"
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>

bool error_exit = false;

int main(int argc, char *argv[]) {
  if (argc > 1) {
    if (strcmp(argv[1], "-v") == 0) {
      std::cout << "Flame compiler 0.10.0\nBy Naharashu\n";
      return 0;
    }
    if (strcmp(argv[1], "-h") == 0) {
      std::cout << "-o [filename] - output file name(executable), by default - out\n";
      std::cout << "-v - version of compiler\n";
      std::cout << "-lexer-debug - output lexed tokens\n";
      std::cout << "======Compile flags======\n";
      std::cout << "-O0 - generate C++ with -O0\n";
      std::cout << "-O1 - generate C++ with -O1\n";
      std::cout << "-O2 - generate C++ with -O2\n";
      std::cout << "-O3 - generate C++ with -O3\n";
      std::cout << "-O4 - generate C++ with -O3, -funroll-loops, -flto\n";
      std::cout << "======Backend======\n";
      std::cout << "-CXX - generate only C++ code\n";
      std::cout << "-C - use C backend(experimental)\n";
      std::cout << "-t - dont generate binary\n";
      std::cout << "-clang - use clang instead of gcc\n";
      return 0;
    }
  }

  bool compile_into_bin = true;
  std::string out_name = "out";
  bool lexer_output = false;

  uint8_t opt_level = 0; // by default -O0
  
  bool use_c=false;
  std::string filename = "";
  bool clang_ = false;

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-CXX") == 0)
      use_c = false;
    else if (strcmp(argv[i], "-o") == 0) {
      out_name = argc > i + 1 ? argv[i + 1] : "out";
      i++;
    }
    else if (strcmp(argv[i], "-lexer-debug") == 0)
      lexer_output = true;
    else if (strcmp(argv[i], "-O0") == 0)
      opt_level = 0;
    else if (strcmp(argv[i], "-O1") == 0)
      opt_level = 1;
    else if (strcmp(argv[i], "-O2") == 0)
      opt_level = 2;
    else if (strcmp(argv[i], "-O3") == 0)
      opt_level = 3;
    else if (strcmp(argv[i], "-O4") == 0)
      opt_level = 4;
    else if (strcmp(argv[i], "-t") == 0)
      compile_into_bin = false;
    else if (strcmp(argv[i], "-C")==0) {
      use_c = true;
      std::cerr << "Using experimental feature: C generator\n";
    }
    else if (strcmp(argv[i], "-clang") == 0)
      clang_ = true;
    else if(filename=="")
      filename = argv[i];
    else {
      std::cerr << "Unknown argument '" << argv[i] << "'\n";
      return 1;
    }
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
  std::ifstream file(filename);
  if(!file) {
    std::cerr << "Input file doenst exist: " << filename << '\n';
    return 1;
  }
  while (std::getline(file, line)) {
    code += line + '\n';
  }
  file.close();

  std::vector<token> toks = lex.lex(code);
  parser parser_(toks, filename);
  parser_.c_gen = use_c;
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
  gen.c_gen = use_c;
  std::string code_;

  try {
    code_ = gen.generate(res);
  } catch (TranspileTimeError& e) {
    std::cout << "\x1b[0;35m" << filename << ':' << gen.line << ':' << gen.column << ": \x1b[31m error\n" <<  e.what() << "\x1b[0m";
    return 1;
  }

  const std::string genname = out_name + (use_c ? "_flame.c" : "_flame.cpp");
  std::ofstream out(genname, std::ios::out | std::ios::binary);
  if (!out.is_open()) {
    std::cerr << "E: Cannot open temp file to generate cpp code\n";
    return 1;
  }
  out.write(code_.c_str(), (long)code_.size());
  out.close();

  std::string compiler = use_c ? "gcc" : "g++";
  if(clang_) compiler = use_c ? "clang" : "clang++";

  if (compile_into_bin) {
    std::string cleanup = "rm -f ";
    cleanup += out_name;
    system(cleanup.c_str());
    std::string output;
    if(opt_level==0) output = compiler + " -O0 " + genname +" -o " + out_name;
    else if(opt_level==1) output = compiler + " -O1 " + genname +" -o " + out_name;
    else if(opt_level==2) output = compiler + " -O2 " + genname +" -o " + out_name;
    else if(opt_level==3) output = compiler + " -O3 " + genname +" -o " + out_name;
    else if(opt_level==4) output = compiler + " -O3 -funroll-loops -flto " + genname +" -o " + out_name;
    system(output.c_str());
  }
  // std::cout << code_ << std::endl;
  return 0;
}
