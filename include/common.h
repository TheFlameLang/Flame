#pragma once

#include "lexer.h"
#include <limits>
#include <string>

typedef unsigned long long u64;
typedef long long i64;
typedef double f64;
typedef unsigned char u8;

inline std::string variant2string(const token_value &val) {
  if (const std::string *s = std::get_if<std::string>(&val)) {
    return *s;
  }
  return "error";
}

template <typename T> inline long long variant2int(const token_value &val) {
  return std::get<T>(val);
}

inline double variant2float(const token_value &val) {
  return std::get<float>(val);
}

inline std::string variant2char(const token_value &val) {
  return std::to_string((std::get<char>(val)));
}

inline double variant2double(const token_value &val) {
  if(auto* res = std::get_if<double>(&val)) {
    return *res;
  } else if(auto* res = std::get_if<float>(&val)) {
    return static_cast<double>(*res);
  } else if(auto* res = std::get_if<i64>(&val)) {
    return static_cast<double>(*res);
  } else if(auto* res = std::get_if<u64>(&val)) {
    return static_cast<double>(*res);
  } else if(auto* res = std::get_if<bool>(&val)) {
    return *res ? 1.0 : 0.0;
  } else {
    return 0.0;
  }
}

inline bool variant2bool(token_value &val) { return std::get<bool>(val); }

template <typename T> bool fits(auto v) {
  return v >= std::numeric_limits<T>::min() &&
         v <= std::numeric_limits<T>::max();
}

inline bool is_it_value(const token &a) {
  if (a.type >= BYTE && a.type <= NULL_)
    return true;
  return false;
}

inline bool is_it_int_value(const token &a) {
  if (a.type >= BYTE && a.type <= UNSIGNED)
    return true;
  return false;
}

inline bool is_it_type(const token &a) {
  switch (a.type) {
  case BYTE_TYPE:
  case WORD_TYPE:
  case INT_TYPE:
  case LONG_TYPE:
  case FLOAT_TYPE:
  case DOUBLE_TYPE:
  case STRING_TYPE:
  case BOOL_TYPE:
  case UNSIGNED_8_TYPE:
  case UNSIGNED_16_TYPE:
  case UNSIGNED_32_TYPE:
  case UNSIGNED_64_TYPE:
  case STRUCT:
  case VEC:
  case AUTO_TYPE:
    return true;
  default:
    return false;
  }
}

inline bool is_it_int_type(const token_type &t) {
  switch (t) {
  case BYTE_TYPE:
  case WORD_TYPE:
  case INT_TYPE:
  case LONG_TYPE:
  case FLOAT_TYPE:
  case DOUBLE_TYPE:
  case UNSIGNED_8_TYPE:
  case UNSIGNED_16_TYPE:
  case UNSIGNED_32_TYPE:
  case UNSIGNED_64_TYPE:
    return true;
  default:
    return false;
  }
}

inline bool is_it_logical(token_type a) {
  if (a != BIGGER && a != BEQUAL && a != LEQUAL && a != LESS && a != EQUAL &&
      a != NEQUAL && a != AND && a != OR)
    return false;
  return true;
}

inline bool is_it_op(token_type a) {
  if(is_it_logical(a)) return true;
  switch(a) {
    case PLUS:
    case MINUS:
    case SLASH:
    case STAR:
    case MOD:
    case XOR:
    case SHIFT_L:
    case SHIFT_R:
    case OR_B:
    case AND_B:
      return true;
    default:
      return false;
  }
}

inline bool is_literal(token_type t) {
  switch (t) {
  case BYTE:
  case WORD:
  case INT:
  case LONG:
  case UNSIGNED:
  case FLOAT:
  case DOUBLE:
  case TRUE:
  case FALSE:
  case STRING:
  case CHAR:
    return true;
  default:
    return false;
  }
}

inline std::string type_in_cpp(const token &a) {
  std::string type;
  if (a.type == BYTE_TYPE)
    type = "int8_t ";
  if (a.type == WORD_TYPE)
    type = "int16_t ";
  if (a.type == INT_TYPE)
    type = "int32_t ";
  if (a.type == LONG_TYPE)
    type = "int64_t ";
  if (a.type == FLOAT_TYPE)
    type = "float ";
  if (a.type == DOUBLE_TYPE)
    type = "double ";
  if (a.type == STRING_TYPE) {
    type = "std::string ";
  }
  if (a.type == BOOL_TYPE)
    type = "bool ";
  if (a.type == UNSIGNED_8_TYPE)
    type = "uint8_t ";
  if (a.type == UNSIGNED_16_TYPE)
    type = "uint16_t ";
  if (a.type == UNSIGNED_32_TYPE)
    type = "uint32_t ";
  if (a.type == UNSIGNED_64_TYPE)
    type = "uint64_t ";
  if (a.type == VOID_TYPE)
    type = "void ";
  if (a.type == AUTO_TYPE)
    type = "auto ";
  if (a.type == VEC)
    type = "std::vector<";
  if (a.type == STRUCT)
    type = "struct ";
  return type;
}

inline std::string type_in_c(const token &a) {
  std::string type;
  if (a.type == BYTE_TYPE)
    type = "int8_t ";
  if (a.type == WORD_TYPE)
    type = "int16_t ";
  if (a.type == INT_TYPE)
    type = "int32_t ";
  if (a.type == LONG_TYPE)
    type = "int64_t ";
  if (a.type == FLOAT_TYPE)
    type = "float ";
  if (a.type == DOUBLE_TYPE)
    type = "double ";
  if (a.type == STRING_TYPE) {
    type = "string* ";
  }
  if (a.type == BOOL_TYPE)
    type = "bool ";
  if (a.type == UNSIGNED_8_TYPE)
    type = "uint8_t ";
  if (a.type == UNSIGNED_16_TYPE)
    type = "uint16_t ";
  if (a.type == UNSIGNED_32_TYPE)
    type = "uint32_t ";
  if (a.type == UNSIGNED_64_TYPE)
    type = "uint64_t ";
  if (a.type == VOID_TYPE)
    type = "void ";
  if (a.type == AUTO_TYPE)
    type = "exit(1)";
  if (a.type == VEC)
    type = "exit(1)";
  if (a.type == STRUCT)
    type = "struct ";
  return type;
}

inline std::string op2string(token_type a) {
  std::string op;
  if (a == PLUS)
    op = " + ";
  if (a == MINUS)
    op = " - ";
  if (a == STAR)
    op = " * ";
  if (a == SLASH)
    op = " / ";
  if (a == MOD)
    op = " % ";
  if (a == AND)
    op = " && ";
  if (a == OR)
    op = " || ";
  if (a == XOR)
    op = " ^ ";
  if (a == OR_B)
    op = " | ";
  if (a == AND_B)
    op = " & ";
  if (a == SHIFT_L)
    op = " << ";
  if (a == SHIFT_R)
    op = " >> ";
  return op;
}


inline std::string variant2value(token tok) {
  if (tok.type == BYTE)
    return std::to_string((unsigned char)variant2int<unsigned long long>(tok.value));
  if (tok.type == WORD)
    return std::to_string((unsigned short)variant2int<unsigned long long>(tok.value));
  if (tok.type == INT)
    return std::to_string((unsigned int)variant2int<unsigned long long>(tok.value));
  if (tok.type == LONG)
    return std::to_string(variant2int<long long>(tok.value));
  if (tok.type == UNSIGNED)
    return std::to_string(variant2int<unsigned long long>(tok.value));
  if (tok.type == FLOAT)
    return std::to_string(variant2float(tok.value));
  if (tok.type == DOUBLE)
    return std::to_string(variant2double(tok.value));
  if (tok.type == STRING)
    return "\"" + variant2string(tok.value) + "\"";
  if (tok.type == CHAR) {
    return "\'" + variant2char(tok.value) + "\'";
  }
  if (tok.type == TRUE || tok.type == FALSE)
    return std::to_string(variant2bool(tok.value));
  if (tok.type == NULL_)
    return variant2string(tok.value);
  if (tok.type == ID)
    return variant2string(tok.value);
  return "";
}

inline void removeSubstring(std::string& a, const std::string& b) {
  u64 pos = a.find(b);
  while(pos != std::string::npos) {
    a.erase(pos, b.length());
    pos = a.find(b);
  }
}

inline bool is_it_int(token_type a) {
  switch(a) {
    case BYTE:
    case WORD:
    case INT:
    case LONG:
    case UNSIGNED:
      return true;
    default:
      return false;
  }
}

inline bool is_it_float(token_type a) {
  switch (a) {
    case FLOAT:
    case DOUBLE:
      return true;
    default:
      return false;
  }
}
