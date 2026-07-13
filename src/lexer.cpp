#include "../include/lexer.h"
#include "../include/common.h"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>

token lexer::create_token(const token_type &a,const token_value &b,const u64 &line,const u64 &column, const std::string &str) { return token{a, b, line, column, str}; }

bool lexer::is_int(char c) { return c >= '0' && c <= '9'; }

bool lexer::is_letter(char c) {
  return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

std::vector<token> lexer::lex(std::string src) {
  std::vector<token> lexed;
  size_t i = 0;
  l = 1;
  u64 col = 1;
  while (i < src.size()) {
    char c = src[i];
    col++;
    char next = (i + 1 < src.size()) ? src[i + 1] : '\0';
    if (c == ' ' || c == '\t' || c == '\r') {
      i++;
      continue;
    }
    if(c == '\n') {
      l++;
      col=0;
      i++;
      continue;
    }
    switch (c) {
    case '+': {
      lexed.push_back(create_token(PLUS, nothing{}, l, col));
      break;
    }
    case '-': {
      lexed.push_back(create_token(MINUS, nothing{}, l, col));
      break;
    }
    case '*': {
      lexed.push_back(create_token(STAR, nothing{}, l, col));
      break;
    }
    case '/': {
      if(next == '/') {
        while(i<src.size()&&src.at(i)!='\n') i++;
        l++;
        col=0;
        continue;
      }
      if(next == '*') {
        i+=2;
        while(!(src.at(i)=='*'&&src.at(i+1)=='/')) {
          if(src[i]=='\n') {
            l++;
            col=0;
          }
          i++;
        }
        i+=2;
        continue;
      }
      lexed.push_back(create_token(SLASH, nothing{}, l, col));
      break;
    }
    case '%': {
      lexed.push_back(create_token(MOD, nothing{}, l, col));
      break;
    }
    case '(': {
      lexed.push_back(create_token(L_BRACKET, nothing{}, l, col));
      break;
    }
    case ')': {
      lexed.push_back(create_token(R_BRACKET, nothing{}, l, col));
      break;
    }
    case '[': {
      lexed.push_back(create_token(L_SQ_BRACKET, nothing{}, l, col));
      break;
    }
    case ']': {
      lexed.push_back(create_token(R_SQ_BRACKET, nothing{}, l, col));
      break;
    }
    case '{': {
      lexed.push_back(create_token(L_BRACES, nothing{}, l, col));
      break;
    }
    case '}': {
      lexed.push_back(create_token(R_BRACES, nothing{}, l, col));
      break;
    }
    case '=': {
      if (next == '=') {
        lexed.push_back(create_token(EQUAL, nothing{}, l, col));
        i++;
        break;
      }
      lexed.push_back(create_token(EQ, nothing{}, l, col));
      break;
    }
    case '>': {
      if (next == '=') {
        lexed.push_back(create_token(BEQUAL, nothing{}, l, col));
        i++;
        break;
      } else if (next == '>') {
        lexed.push_back(create_token(SHIFT_R, nothing{}, l, col));
        i++;
        break;
      }
      lexed.push_back(create_token(BIGGER, nothing{}, l, col));
      break;
    }
    case '<': {
      if (next == '=') {
        lexed.push_back(create_token(LEQUAL, nothing{}, l, col));
        i++;
        break;
      } else if (next == '<') {
        lexed.push_back(create_token(SHIFT_L, nothing{}, l, col));
        i++;
        break;
      }
      lexed.push_back(create_token(LESS, nothing{}, l, col));
      break;
    }
    case '!': {
      if (next == '=') {
        lexed.push_back(create_token(NEQUAL, nothing{}, l, col));
        i++;
        break;
      }
      lexed.push_back(create_token(NOT, nothing{}, l, col));
      break;
    }
    case '&': {
      if (next == '&') {
        lexed.push_back(create_token(AND, nothing{}, l, col));
        i++;
        break;
      }
      lexed.push_back(create_token(AND_B, nothing{}, l, col));
      break;
    }
    case '|': {
      if (next == '|') {
        lexed.push_back(create_token(OR, nothing{}, l, col));
        i++;
        break;
      }
      lexed.push_back(create_token(OR_B, nothing{}, l, col));
      break;
    }
    case '^': {
      lexed.push_back(create_token(XOR, nothing{}, l, col));
      break;
    }
    case ';': {
      lexed.push_back(create_token(SEMI, nothing{}, l, col));
      break;
    }
    case ',': {
      lexed.push_back(create_token(COMA, nothing{}, l, col));
      break;
    }
    case '.': {
      lexed.push_back(create_token(DOT, nothing{}, l, col));
      break;
    }
    case ':': {
      lexed.push_back(create_token(TWODOTS, nothing{}, l, col));
      break;
    }
    default:
      break;
    }
    if (is_letter(c)) {
      std::string id;
      while (i < src.size() &&
             (is_letter(src[i]) || is_int(src[i]) || src[i] == '_')) {
        id.push_back(src[i]);
        i++;
      }
      if (id == "if")
        lexed.push_back(create_token(IF, nothing{}, l, col));
      else if (id == "else")
        lexed.push_back(create_token(ELSE, nothing{}, l, col));
      else if (id == "elif")
        lexed.push_back(create_token(ELIF, nothing{}, l, col));
      else if (id == "while")
        lexed.push_back(create_token(WHILE, nothing{}, l, col));
      else if (id == "break")
        lexed.push_back(create_token(BREAK, nothing{}, l, col));
      else if (id == "continue")
        lexed.push_back(create_token(CONTINUE, nothing{}, l, col));
      else if (id == "for")
        lexed.push_back(create_token(FOR, nothing{}, l, col));
      else if (id == "func")
        lexed.push_back(create_token(FUNC, nothing{}, l, col));
      else if (id == "true")
        lexed.push_back(create_token(TRUE, true, l, col));
      else if (id == "false")
        lexed.push_back(create_token(FALSE, false, l, col));
      else if (id == "i8")
        lexed.push_back(create_token(BYTE_TYPE, nothing{}, l, col));
      else if (id == "u8")
        lexed.push_back(create_token(UNSIGNED_8_TYPE, nothing{}, l, col));
      else if (id == "i16")
        lexed.push_back(create_token(WORD_TYPE, nothing{}, l, col));
      else if (id == "u16")
        lexed.push_back(create_token(UNSIGNED_16_TYPE, nothing{}, l, col));
      else if (id == "i32")
        lexed.push_back(create_token(INT_TYPE, nothing{}, l, col));
      else if (id == "u32")
        lexed.push_back(create_token(UNSIGNED_32_TYPE, nothing{}, l, col));
      else if (id == "i64")
        lexed.push_back(create_token(LONG_TYPE, nothing{}, l, col));
      else if (id == "u64")
        lexed.push_back(create_token(UNSIGNED_64_TYPE, nothing{}, l, col));
      else if (id == "f32")
        lexed.push_back(create_token(FLOAT_TYPE, nothing{}, l, col));
      else if (id == "f64")
        lexed.push_back(create_token(DOUBLE_TYPE, nothing{}, l, col));
      else if (id == "string")
        lexed.push_back(create_token(STRING_TYPE, nothing{}, l, col));
      else if (id == "bool")
        lexed.push_back(create_token(BOOL_TYPE, nothing{}, l, col));
      else if (id == "null")
        lexed.push_back(create_token(NULL_, std::string{"NULL"}, l, col));
      else if (id == "void")
        lexed.push_back(create_token(VOID_TYPE, nothing{}, l, col));
      else if (id == "const")
        lexed.push_back(create_token(CONST, nothing{}, l, col));
      else if (id == "comptime")
        lexed.push_back(create_token(COMPTIME, nothing{}, l, col));
      else if (id == "auto")
        lexed.push_back(create_token(AUTO_TYPE, nothing{}, l, col));
      else if (id == "use")
        lexed.push_back(create_token(USE, nothing{}, l, col));
      else if (id == "return")
        lexed.push_back(create_token(RETURN, nothing{}, l, col));
      else if (id == "vec")
        lexed.push_back(create_token(VEC, nothing{}, l, col));
      else if (id == "struct")
        lexed.push_back(create_token(STRUCT, nothing{}, l, col));
      else if (id == "this")
        lexed.push_back(create_token(THIS, nothing{}, l, col));
      else if (id == "ref")
        lexed.push_back(create_token(REF, nothing{}, l, col));
      else if (id == "mut")
        lexed.push_back(create_token(MUT, nothing{}, l, col));
      else if (id == "namespace")
        lexed.push_back(create_token(NAMESPACE, nothing{}, l, col));
      else
        lexed.push_back(create_token(ID, id, l,col, id));

      continue;
    }
    if (c == '"'|| c == '\'') {
      std::string str;
      bool char_mode = c == '\'';
      
      if(i+1 < src.size() && char_mode) {
        char charr = src[i+1];
        i+=3; // 'c'
        lexed.push_back(create_token(CHAR, charr, l, col));
        continue;
      }
      i++;
      while (i < src.size() && src[i] != '"') {
        /*
        if (src[i] == '\\' && i + 1 < src.size()) {
          i++;
          switch (src[i]) {
          case 'n':
            str += '\n';
            break;
          case 't':
            str += '\t';
            break;
          case '"':
            str += '"';
            break;
          case '\\':
            str += '\\';
            break;
          default:
            str += src[i];
            break;
          }
        } else {*/
          str += src[i];
        //}
        i++;
      }
      if (i == src.size()) {
        std::cerr << "Error: unterminated string literal\n";
        exit(1);
      }
      i++;
      lexed.push_back(create_token(STRING, str, l, col, str));
      continue;
    }
    if (is_int(c)) {
      std::string number;
      while ((i < src.size() && is_int(src[i])) || src[i] == '.') {
        number += src[i++];
      }
      char *endptr;
      if (number.find('.') != std::string::npos) {
        auto val_ = strtod(number.c_str(), &endptr);
        if(fits<float>(val_)) lexed.push_back(create_token(FLOAT, (float)val_ ,l, col));
        else lexed.push_back(create_token(DOUBLE, val_ ,l, col));
      } else {
        auto val_ = strtoull(number.c_str(), &endptr, 10);
        token_type type = INT;
        if (fits<uint8_t>(val_))
          type = BYTE;
        else if (fits<uint16_t>(val_))
          type = WORD;
        else if (fits<uint32_t>(val_))
          type = INT;
        else if (fits<uint64_t>(val_))
          type = UNSIGNED;
        else
          type = LONG;
        lexed.push_back(create_token(type, static_cast<token_value>(val_),l, col));
      }
      continue;
    }
    i++;
  }
  lexed.push_back(create_token(EOF_, nothing{}, l, col));
  return lexed;
}


std::string disassemble_tok_type(token_type type) {
  switch (type) {
    case BYTE_TYPE:
      return "i8";
    case WORD_TYPE:
      return "i16";
    case INT_TYPE:
      return "i32";
    case LONG_TYPE:
      return "i64";
    case AUTO_TYPE:
      return "auto";
    case UNSIGNED_8_TYPE: 
      return "u8";
    case UNSIGNED_16_TYPE:
      return "u16";
    case UNSIGNED_32_TYPE:
      return "u32";
    case UNSIGNED_64_TYPE:
      return "u64";
    case FLOAT_TYPE:
      return "f32";
    case DOUBLE_TYPE:
      return "f64";
    case BOOL_TYPE:
      return "bool";
    case STRING_TYPE:
      return "string";
    case SEMI:
      return ";";
    case COMA:
      return ",";
    case DOT:
      return ".";
    case ID:
      return "ID";
    case L_BRACKET:
      return "(";
    case R_BRACKET:
      return ")";
    case L_SQ_BRACKET:
      return "[";
    case R_SQ_BRACKET:
      return "]";
    case L_BRACES:
      return "{";
    case R_BRACES:
      return "}";
    case BYTE:
    case WORD:
    case INT:
    case LONG:
    case UNSIGNED:
      return "number";
    case FLOAT:
    case DOUBLE:
      return "floating point number";
    case FUNC:
      return "func";
    case RETURN:
      return "return";
    case WHILE:
      return "while";
    case FOR:
      return "for";
    case USE:
      return "use";
    case THIS:
      return "this";
    case REF:
      return "ref";
    case IF:
      return "if";
    case ELIF:
      return "elif";
    case ELSE:
      return "else";
    case TRUE:
      return "true";
    case FALSE:
      return "false";
    case PLUS:
      return "+";
    case MINUS:
      return "-";
    case STAR:
      return "*";
    case SLASH:
      return "/";
    case MOD:
      return "%";
    case EQ:
      return "=";
    case AND:
      return "and";
    case OR:
      return "or";
    case LESS:
      return "<";
    case BIGGER:
      return ">";
    case LEQUAL:
      return "<=";
    case BEQUAL:
      return ">=";
    case NEQUAL:
      return "!=";
    case EQUAL:
      return "==";
    case CONST:
      return "const";
    case XOR:
      return "xor";
    case AND_B:
      return "bitwise and";
    case OR_B:
      return "bitwise or";
    case EOF_:
      return "EOF";
    default:
      return std::to_string(type);
  }
}