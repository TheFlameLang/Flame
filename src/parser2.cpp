#include "../include/parser2.h"
#include "../include/ast.h"
#include "../include/common.h"
#include "../include/eval_ast.h"
#include "../include/lexer.h"
#include "../include/table.h"
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <memory>
#include <ranges>
#include <sstream>
#include <string>
#include <unistd.h>
#include <utility>
#include <vector>

astptr parser::parse_unary()
{
    token op = peek();
    if (op.type == token_type::PLUS || op.type == token_type::MINUS || op.type == token_type::NOT)
    {
        consume();
        astptr unary = parse_unary();
        return std::make_unique<UnaryNode>(std::move(unary), op.type);
    }
    return parse_factor();
}

static int prec(token_type t) {
    switch (t) {
    case OR:                                  return 1;
    case AND:                                 return 2;
    case OR_B:                                return 3;
    case XOR:                                 return 4;
    case AND_B:                               return 5;
    case EQUAL: case NEQUAL:                  return 6;
    case LESS: case BIGGER: case LEQUAL: case BEQUAL: return 7;
    case SHIFT_L: case SHIFT_R:               return 8;
    case PLUS: case MINUS:                    return 9;
    case STAR: case SLASH: case MOD:          return 10;
    default:                                  return -1;
    }
}



astptr parser::parse_expr(int min_prec) {
    astptr lhs = parse_unary();
    for (;;) {
        token op = peek();
        int p = prec(op.type);
        if (p < min_prec) break;         
        consume();
        astptr rhs = parse_expr(p + 1);
        if (is_it_logical(op.type))
            lhs = std::make_unique<CondNode>(std::move(lhs), std::move(rhs), op.type);
        else
            lhs = std::make_unique<BinaryNode>(std::move(lhs), std::move(rhs), op);
    }
    return lhs;
}

astptr parser::parse_or() { return parse_expr(1); }  