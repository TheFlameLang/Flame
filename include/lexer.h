#pragma once

#include <variant>
#include <string>
#include <vector>
#include <iostream>

enum token_type : unsigned char {
    BYTE_TYPE,
    WORD_TYPE,
    INT_TYPE,
    LONG_TYPE,
    UNSIGNED_8_TYPE,
    UNSIGNED_16_TYPE,
    UNSIGNED_32_TYPE,
    UNSIGNED_64_TYPE,
    BOOL_TYPE,
    FLOAT_TYPE,
    DOUBLE_TYPE,
    STRING_TYPE,
    VOID_TYPE,
    AUTO_TYPE,
    BYTE,
    WORD,
    INT,
    LONG,
    UNSIGNED,
    FLOAT,
    DOUBLE,
    STRING,
    CHAR,
    TRUE,
    FALSE,
    NULL_,
    PLUS,
    MINUS,
    STAR,
    SLASH,
    MOD,
    EQ, // 29
    EQUAL,
    BIGGER,
    LESS,
    SHIFT_R,
    SHIFT_L,
    NOT,
    AND_B,
    AND,
    OR_B,
    OR,
    XOR,
    NEQUAL,
    BEQUAL,
    LEQUAL,
    R_BRACKET,
    L_BRACKET,
    R_SQ_BRACKET,
    L_SQ_BRACKET, // 47
    R_BRACES,
    L_BRACES,
    DOT,
    TWODOTS,
    SEMI,
    COMA,
    IF,
    ELIF,
    ELSE,
    WHILE,
    BREAK,
    CONTINUE,
    FOR,
    FUNC,
    USE,
    RETURN,
    CONST,
    COMPTIME,
    STRUCT,
    VEC,
    NAMESPACE,
    THIS,
    REF,
    MUT,
    ID,
    NEWLINE,
    EOF_
};

using nothing = std::monostate;
using token_value = std::variant<nothing, char, unsigned long long, long long, float, double, bool, std::string>;

struct token {
    token_type type;
    token_value value;
    unsigned long long line;
    unsigned long long column;
    std::string str_value="";
};

class lexer {
private:
  token create_token(const token_type &a,const token_value &b,const unsigned long long &line,const unsigned long long &column, const std::string &str="");
  bool is_int(char c);
  bool is_letter(char c);
  unsigned long long l;

public:
  std::vector<token> lex(std::string src);
};

std::string disassemble_tok_type(token_type tok);