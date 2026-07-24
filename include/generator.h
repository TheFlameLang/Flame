#pragma once

#include "ast.h"
#include "common.h"
#include "lexer.h"
#include "parser.h"
#include <cstddef>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

class TranspileTimeError : public std::exception {
  private:
  std::string message;
  public:
  explicit TranspileTimeError(const std::string &msg) : message(msg) {}

  const char * what() const noexcept override {
    return message.c_str();
  }
};

class generator {
public:
  int indent = 0;
  u64 line=1;
  u64 column=0;
  bool c_gen=false;
  bool is_mod = false;
  generator* root = nullptr;
  std::string pad() { return std::string(static_cast<int>(indent * 4), ' '); }

  std::ostringstream gen_code;
  std::string header;
  std::string gencode(astptr &node) {
    if(c_gen) return node->gen_(*this);
    return node->gen(*this);
  }

  inline void cpp_headers() {
    if(is_mod) return;
    if(header.find("<iostream>\n")==std::string::npos) { 
      header += "#include <iostream>\n";
    }
    if(header.find("<cstdint>")==std::string::npos) {
      header += "#include <cstdint>\n";
    }
    if(header.find("<array>")==std::string::npos) {
      header += "#include <array>\n";
    }
    if(header.find("<vector>")==std::string::npos) {
      header += "#include <vector>\n";
    }
    if(header.find("<memory>")==std::string::npos) {
      header += "#include <memory>\n";
    }
  }

  inline void c_headers() {
    if(is_mod) return;
    if(header.find("<iostream>\n")==std::string::npos) { 
      header += "#include <stdio.h>\n";
    }
    if(header.find("<cstdint>")==std::string::npos) {
      header += "#include <stdint.h>\n";
    }
    if(header.find("<oxygen_runtime.h>")==std::string::npos) {
      header += "#include <oxygen_runtime.h>\n";
      std::ostringstream check;
      header += "#define OXYGEN_REQ_MAJ_V 0\n";
      header += "#define OXYGEN_REQ_MIN_V 2\n";
      header += "#define OXYGEN_REQ_PAT_V 0\n";
      check << "#if OXYGEN_MAJOR_VER < OXYGEN_REQ_MAJ_V\n"
      << "    #error \"Using very out-dated version of Oxygen Runtime!\"\n"
      << "#elif OXYGEN_MINOR_VER < OXYGEN_REQ_MIN_V\n"
      << "    #warning \"Using old version of Oxygen Runtime!\"\n"
      << "#elif OXYGEN_PATCH_VER < OXYGEN_REQ_PAT_V\n"
      << "    #warning \"Using older release of Oxygen Runtime!\"\n"
      << "#endif\n";
      header += check.str();
    }
  }

  std::string generate(std::vector<astptr> &nodes) {
    if(c_gen) c_headers();
    else cpp_headers();
    
    for (auto &x : nodes) {
      if(!x) continue;
      std::string c = gencode(x);
      if (c.empty())
        continue;

      if (x->kind != ast_type::MODULE && x->kind != ast_type::BLOCK &&
          x->kind != ast_type::IF&&x->kind!=ast_type::FUNC)
        gen_code << c + ';';

      else
        gen_code << c;
      gen_code << '\n';
    }
    return header + gen_code.str();
  }
};
