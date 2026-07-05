#pragma once

#include "lexer.h"
#include "ast.h"
#include "common.h"
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>





class ParseTimeError : public std::exception {
  private:
  std::string message;
  public:
  explicit ParseTimeError(const std::string &msg) : message(msg) {}

  const char * what() const noexcept override {
    return message.c_str();
  }
};

class parser {
    public:
    std::vector<token> src;
    u64 indx;
    u64 line=0;
    u64 column=0;
    bool factor_decl_mode = false;
    std::string filename;
    bool errors=false;
    bool is_module=false;
    explicit parser(const std::vector<token> &a, const std::string &f) {
        src = a;
        indx = 0;
        filename = f;
    }
    bool returning=false;

    astptr parse_expr();
    astptr parse_shift();
    astptr parse_unary();
    astptr parse_and_b();
    astptr parse_xor();
    astptr parse_or_b();
    astptr parse_term();
    astptr parse_factor();
    astptr parse_statement();
    astptr parse_if_statement();
    astptr parse_while_statement();
    astptr parse_for_statement();
    astptr parse_return();
    astptr parse_block(const std::string &func);
    astptr parse_func_statement(const std::string &struct_="");
    astptr parse_use();
    astptr parse_comparison();
    astptr parse_equality();
    astptr parse_and();
    astptr parse_or();
    astptr parse_break_continue();
    astptr parse_array(bool is_const=false, const std::string &struct_="");
    astptr parse_vector(bool is_const=false, const std::string &struct_="");
    void parse_comptime();
    astptr parse_method();
    astptr parse_module_call(const std::string &name="");
    astptr parse_struct();
    astptr parse_assignment(bool is_const=false, bool comptime=false, const std::string &struct_="");
    token consume() {
        if(indx >= src.size()) {
            throw ParseTimeError("\tUnexpected end of input\n");
        }
        return src[indx++];
    }
    token consume(token_type expected) {
        if(indx >= src.size()) {
            throw ParseTimeError("\tUnexpected end of input\n");
        }
        if(src[indx].type != expected) {
            line = src[indx].line;
            column = src[indx].column;
            throw ParseTimeError("\tUnexpected token " + disassemble_tok_type(src[indx].type) + " , " + "expected " + disassemble_tok_type(expected) + '\n');
        }
        return src[indx++];
    }
    inline token peek() {
        if(indx >= src.size()) {
            throw ParseTimeError("\tUnexpected end of input\n");
        }
        return src.at(indx);
    }
    inline token peek(u8 i) {
        if(indx+i >= src.size()) {
            throw ParseTimeError("\tUnexpected end of input\n");
        }
        return src.at(indx+i);
    }
    std::vector<token> peek_(int n) {
        std::vector<token> res;
        for(u64 i=indx;i<indx+n;i++) {
            res.push_back(src.at(i));
        }
        return res;
    }
    void sync() {
        while(true) {
            token_type c = peek().type;
            if(c==EOF_) return;
            if(c==SEMI||c==R_BRACES) {
                consume();
                return;
            }
            consume();
        }
    }
    std::vector<std::unique_ptr<ASTNode>>parse();
};
