#pragma once

#include "lexer.h"
#include "ast.h"
#include "common.h"
#include <algorithm>
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

inline std::string replaceChar(std::string s, char oldChar, char newChar) {
    std::replace(s.begin(), s.end(), oldChar, newChar);
    std::size_t pos;
    while ((pos = s.find("__")) != std::string::npos) {
        s.replace(pos, 2, "_");
    }
    return s;
}


class parser {
    public:
    std::vector<token> src; // raw lexed tokens
    u64 indx; // index in src
    u64 line=0; // line for debug
    u64 column=0; // column for debug
    std::string filename; // filename
    bool errors=false;
    bool is_module=false;
    bool c_gen=false;
    explicit parser(const std::vector<token> &a, const std::string &f) {
        src = a;
        indx = 0;
        filename = f;
    }
    std::string get_name(const std::string& varname, const std::string& struct_n="", bool module=true ) {
        //std::string fname = replaceChar(filename, '.', '_');
        if(is_module&&module) {
            std::string temp = filename + "_" + struct_n + "_" + varname;
            return replaceChar(temp, '.', '_');
        }
        else {
            if(struct_n=="") return varname;
            std::string temp = struct_n + "_" + varname;
            return replaceChar(temp, '.', '_');
        }
    }
    bool returning=false; // internal: is parse_block seen return for function

    /**
     * @brief Prints error
     * 
     * @param error text for error msg
     * @param tok for line and column 
     */
    inline void ParserError(const std::string &error, const token &tok) {
        line = tok.line;
        column = tok.column;
        throw ParseTimeError(error);
    }

    /**
     * @brief Prints warning
     * 
     * @param warning Text you want to print as warning
     * @param tok for line and column
     * @param warntype additional text as warning type 
     */
    inline void ParserWarning(const std::string &warning, const token &tok, const std::string& warntype="BasicWarning") {
        std::cerr << "\x1b[0;33m" << filename << ":" << std::to_string(tok.line) << ":" + std::to_string(tok.column)
                          << ":  warning(" << warntype << ")\n" << warning << '\n' << "\x1b[0m\n";
    }

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
    astptr parse_return(const token_type& expectedType=NULL_, bool isptr_return=false);
    astptr parse_block(const token_type& expectedType=NULL_, bool isptr_return=false);
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

    /**
     * @brief Skip to the nearest ; or }
     */
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
