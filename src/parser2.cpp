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

astptr parser::parse_factor()
{
    // 0.10.0 -> math.add()
    // 0.11.0>= -> math::add()
    if (peek().type == ID && peek(1).type == TWODOTS) {
        token module = peek();
        for (auto &x : loadedModules) {
            if (module.str_value == x) {
                consume(ID);
                return parse_module_call(module.str_value);
            }
        }
        return parse_method();
    }
    token tok = consume();
    if (tok.type == token_type::BYTE || tok.type == token_type::WORD || tok.type == token_type::INT ||
        tok.type == token_type::LONG || tok.type == token_type::UNSIGNED || tok.type == token_type::NULL_ ||
        tok.type == token_type::FLOAT || tok.type == token_type::DOUBLE || tok.type == token_type::TRUE ||
        tok.type == token_type::FALSE || tok.type == token_type::VOID_TYPE || tok.type == token_type::STRING || tok.type == token_type::CHAR)
    {
        return std::make_unique<Node>(tok);
    }
    else if (tok.type == token_type::L_BRACKET)
    {

        astptr node = parse_or();

        token close = consume(token_type::R_BRACKET);

        return node;
    }
    else
    {
        err("Unexpected token '" + disassemble_tok_type(tok.type) + "'\n", "UnexpectedToken",tok);
        
    }
}


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

astptr parser::parse_assignment(bool is_const, bool comptime, const std::string &struct_) {
    if (is_const)
        consume(CONST);
    if (is_struct(peek().str_value))
    {
        token struct_id = consume(ID);
        bool isptr = false;
        if(peek().type==STAR) {
            isptr = true;
            consume(STAR);
        }
        token id = consume(ID);
        consume(SEMI);
        if(is_module) insert(id.str_value, STRUCT, struct_id.str_value, false, 1, false, false, false, isptr, filename, false,false, struct_id.str_value);
        else insert(id.str_value, STRUCT, struct_id.str_value, false, 1, false, false, false, isptr, "", false, false, struct_id.str_value);
        return std::make_unique<AssignmentNodeExpr>(STRUCT, get_name(id.str_value), astptr{}, is_const, struct_id.str_value, isptr);
    }
    if (peek().type == VEC)
        return parse_vector(is_const, struct_ + '.');
    if (peek().type == ID && (peek(1).type == L_BRACKET || peek(1).type == L_SQ_BRACKET))
    {
        astptr node = parse_factor();
        consume(SEMI);
        return node;
    }
    else if ((peek().type == ID && peek(1).type == PLUS && peek(2).type == PLUS) ||
             (peek().type == ID && peek(1).type == MINUS && peek(2).type == MINUS))
    {
        token id = consume(ID);
        token a = consume();
        consume();
        if (!exist(id.str_value))
        {
            ParserError("\tUse undeclared variable '" + id.str_value + "'\n", id);
        }
        if (a.type == PLUS)
        {
            if (peek().type == SEMI)
                consume(SEMI);
            return std::make_unique<IncDecVarNode>(0, get_name(id.str_value));
        }
        else
        {
            if (peek().type == SEMI)
                consume(SEMI);
            return std::make_unique<IncDecVarNode>(1, get_name(id.str_value));
        }
    }
    if (is_it_type(peek()) && peek(1).type == L_SQ_BRACKET)
        return parse_array(is_const, struct_);
    token type = consume();
    if (type.type == ID)
    {
        if (peek().type == PLUS || peek().type == MINUS || peek().type == STAR || peek().type == SLASH ||
            peek().type == MOD || peek().type == XOR || peek().type == AND_B || peek().type == OR_B ||
            peek().type == SHIFT_L || peek().type == SHIFT_R)
        {
            token op = consume();
            if (!exist(type.str_value))
            {
                ParserError("\tUse undeclared variable '" + type.str_value + "'\n", type);
            }
            consume(EQ);

            u64 rollback = indx;
            symbol s = search(type.str_value);
            while(peek().type!=SEMI) {
                token t = consume();
                if(is_it_int_value(t)) {
                    if(!is_it_int_type(s.type)) {
                        ParserError("\tTrying modify non-int variable '"
                        + type.str_value + "'\n", t);
                    }
                } else if(t.type==ID) {
                    symbol s_ = search(t.str_value);
                    if(s.type!=s_.type) {
                        if(!is_it_int_type(s.type)&&!is_it_int_type(s_.type)) {
                            ParserError("\tTrying modify variable '"
                            + type.str_value + "' with value of '" + s_.name + "', but types dont match\n", t);
                            }
                    }
                }
            }
            indx = rollback;

            if(s.type==STRING_TYPE&&op.type!=PLUS) {
                ParserError("\tString only support '+=' for concatenation\n", op);
            } 

            astptr value = parse_or();
            if (peek().type == SEMI)
                consume(SEMI);
            return std::make_unique<ReAssignmentNodeExpr>(op.type, get_name(type.str_value), std::move(value), is_const, struct_);
        }
        if (!exist(variant2string(type.str_value)) && !is_struct(type.str_value))
        {
            ParserError("\tUse undeclared variable '" + variant2string(type.str_value) + "'\n", type);
        }
        if (search(variant2string(type.str_value)).is_const || is_const)
        {
            ParserError("\tVariable '" + variant2string(type.str_value) + "' is constant'\n", type);
        }
        
        std::string mov_from = "";
        const u64 rollback = indx;
        while(peek().type!=SEMI) {
            token t = consume();
            if(t.type==ID) {
                symbol* s = searchptr(t.str_value);
                if(!s || !s->is_ptr) continue;
                if(s->is_moved) {
                    ParserError("\t'"+t.str_value+"' is already moved\n", t);
                }
                if(s->type!=search(type.str_value).type) {
                    ParserError("\tCannot move pointer '"+t.str_value+"' to '"
                        + type.str_value + "', because types dont match\n"
                        , t);
                }
                mov_from = s->name;
            }
        }
        indx = rollback;
        consume(EQ);
        astptr value = parse_or();
        if(mov_from!="") {
            symbol* s = searchptr(mov_from);
            if(s) s->is_moved=true;
        }
        const bool ismov = mov_from!="";
        consume(SEMI);
        return std::make_unique<AssignmentNode>(get_name(type.str_value), std::move(value), is_const, search(type.str_value).is_ptr, ismov, struct_);
    }
    if (!is_it_type(type) || is_struct(type.str_value))
    {
        ParserError("\tUnexpected type " + std::to_string(type.type) + '\n', type);
    }
    if (peek().type == STAR)
    {
        consume(STAR);
        token id = consume(token_type::ID);
        std::string id_value = id.str_value;


        if (exist_in_scope(id_value, table.size() - 1) && !exist_in_scope(id_value, 0))
        {
            ParserError("\tRedefinition of '" + id.str_value + "'\n", id);
        }
        else if (search_type(id_value) == FUNC)
        {
            ParserError("\tRedefinition function '" + id.str_value + "' as variable\n", id);
        }
        else if (search(id_value).is_const)
        {
            ParserError("\tVariable '" + id_value + "' is constant'\n", id);
        }
        else if (search(id_value).comptime)
        {
            ParserError("\tVariable '" + id_value + "' is compile time value'\n", id);
        }
        if(peek().type==SEMI) {
            consume(SEMI);
            if(is_module) insert(struct_ + id.str_value, type.type, nothing{}, is_const, 1, false, false,false,true, filename);
            else insert(struct_ + id.str_value, type.type, nothing{}, is_const, 1, false, false,false,true);
            return std::make_unique<AssignmentNodeExpr>(type.type, get_name(id.str_value), astptr{}, is_const, "", true);
        }
        consume(EQ);
        std::string mov_from = "";
        u64 size = 0;
        astptr value = parse_factor();
        consume(SEMI);
        return std::make_unique<AssignmentNodeExpr>(type.type, get_name(id.str_value), std::move(value), is_const, struct_, true, ismov);
    }
    token id = consume(token_type::ID);
    std::string id_value = id.str_value;
    if (peek().type == SEMI)
    {
        consume();
        if(is_module) insert(struct_ + id.str_value, type.type, nothing{}, is_const, false, false, false, false, false, filename, false, false, struct_);
        else insert(struct_ + id.str_value, type.type, nothing{}, is_const,1, false, false, false, false, "", false, false, struct_);
        return std::make_unique<AssignmentNodeExpr>(type.type, get_name(id.str_value), astptr{}, is_const, struct_);
    }
    consume(token_type::EQ);
    astptr value = parse_or();
    consume(SEMI);
    return std::make_unique<AssignmentNodeExpr>(type.type, get_name(id.str_value), std::move(value), is_const, struct_);
}


astptr parser::parse_statement()
{
    token tok = peek();
    if (tok.type == ID && peek(1).type == DOT)
        return parse_method();
    switch (tok.type)
    {
    case token_type::IF:
        return parse_if_statement();
    case token_type::WHILE:
        return parse_while_statement();
    case token_type::FOR:
        return parse_for_statement();
    case token_type::L_BRACES:
        return parse_block();
    case token_type::FUNC:
        return parse_func_statement();
    case token_type::USE:
        return parse_use();
    case token_type::RETURN:
        return parse_return();
    case token_type::BREAK:
    case token_type::CONTINUE:
        return parse_break_continue();
    case token_type::CONST:
        return parse_assignment(true);
    case token_type::VEC:
        return parse_vector();
    case token_type::COMPTIME:
        parse_comptime();
        return parse_statement();
    case token_type::STRUCT:
        return parse_struct();
    case token_type::BYTE_TYPE:
    case token_type::WORD_TYPE:
    case token_type::INT_TYPE:
    case token_type::LONG_TYPE:
    case token_type::BOOL_TYPE:
    case token_type::FLOAT_TYPE:
    case token_type::DOUBLE_TYPE:
    case token_type::STRING_TYPE:
    case token_type::UNSIGNED_8_TYPE:
    case token_type::UNSIGNED_16_TYPE:
    case token_type::UNSIGNED_32_TYPE:
    case token_type::UNSIGNED_64_TYPE:
    case token_type::AUTO_TYPE:
    case token_type::ID:
        return parse_assignment();
    default:
        astptr node = parse_or();
        consume(SEMI);
        return node;
    }
    
}

std::vector<std::unique_ptr<ASTNode>> parser::parse()
{
    std::vector<std::unique_ptr<ASTNode>> parsed;
    while (peek().type != token_type::EOF_)
    {
        parsed.push_back(parse_statement());
    }
    return parsed;
}
