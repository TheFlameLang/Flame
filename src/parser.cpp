#include "../include/parser.h"
#include "../include/ast.h"
#include "../include/common.h"
#include "../include/eval_ast.h"
#include "../include/lexer.h"
#include "../include/table.h"
#include <cstddef>
#include <cstdlib>
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
    if (peek().type == ID && peek(1).type == DOT) {
        for(auto &x : loadedModules) {
            if(peek().str_value==x) return parse_module_call(x);
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
    else if (tok.type == token_type::ID)
    {
        std::string id = tok.str_value;
        if(search(id).is_moved) {
            ParserError("\tCannot use moved pointer '"+id+"'\n", tok);
        }
        if (peek().type == L_BRACKET)
        {
            if (!exist(tok.str_value))
            {
                parser::line = tok.line;
                parser::column = tok.column;
                throw ParseTimeError("\tUse undeclared function '" + id + "'\n");
            }

            bool builtin = search(tok.str_value).is_const == true;
            consume();
            std::vector<astptr> args_;
            u64 arg_i = 0;
            fsymbol *f = fsearch(tok.str_value);
            std::vector<symbol> mut_vars;
            while (peek().type != R_BRACKET)
            {
                token c = peek();
                token n = peek(1);
                if ((c.type == ID && (n.type == COMA || n.type == R_BRACKET) && f))
                {
                    symbol* s = searchptr(c.str_value);
                    if (arg_i >= f->args.size())
                    {
                        parser::line = c.line;
                        parser::column = c.column;
                        throw ParseTimeError("\tExpected " + std::to_string(f->args.size()) + " arguments, got " +
                                             std::to_string(arg_i) + "\n");
                    }
                    if (s->type != f->args[arg_i].type)
                    {
                        parser::line = c.line;
                        parser::column = c.column;
                        throw ParseTimeError("\tExpected argument of type '" +
                                             disassemble_tok_type(f->args[arg_i].type) + "', but got '" +
                                             disassemble_tok_type(search_type(c.str_value)) + "'\n");
                    }
                    if (s->is_array && search(c.str_value).size != f->args[arg_i].size)
                    {
                        ParserError("\tExpected array of size '" + std::to_string(f->args[arg_i].size) +
                                             "'\n", c);
                    }
                    if(f->args[arg_i].is_ref_arg) {
                        for(auto &x : mut_vars) {
                            if(s->name==x.name) {
                                ParserError("\tVariable can only have one mutable reference\n", c);
                            }
                        }
                        mut_vars.emplace_back(*s);
                        s->is_mut_now=false;
                    }
                }
                if(f && f->args[arg_i].is_ref_arg && f->args[arg_i].type!=c.type) {
                    ParserError("\tCannot pass temporarily value as mutable reference\n", c);
                }
                arg_i++;
                if(f && f->args[arg_i].is_ref_arg) {
                    token tok = consume(ID);
                    args_.push_back(std::make_unique<Node>(tok, search(tok.str_value).is_ptr, true));
                }
                else args_.push_back(parse_or());
                if (peek().type == COMA)
                    consume();
            }
            consume(R_BRACKET);
            if (builtin)
                return std::make_unique<FuncCallNode>(tok.str_value, std::move(args_), "");
            else if (f->struct_ != "")
                return std::make_unique<FuncCallNode>(get_name(tok.str_value, f->struct_), std::move(args_), "");
            return std::make_unique<FuncCallNode>(tok.str_value, std::move(args_), "");
        }
        if (peek().type == L_SQ_BRACKET)
        {
            if (!exist(variant2string(tok.str_value)))
            {
                ParserError("\tUse undeclared array '" + id + "'\n", tok);
            }
            consume();
            bool have_id = false;
            std::vector<std::string> ids;
            for (size_t i = 0; i <= src.size(); i++)
            {
                if (peek(i).type == EQ || peek(i).type == R_SQ_BRACKET)
                    break;
                if (peek(i).type == ID && !have_id)
                {
                    ids.push_back(variant2string(peek(i).str_value));
                    for (auto &x : ids)
                    {
                        if (!search(x).comptime)
                            have_id = true;
                    }
                }
            }
            astptr i = parse_expr();
            symbol array = search(id);
            if(!array.is_array&&array.type!=STRING_TYPE) {
                ParserError("\tTrying access variable '"+array.name+"' as array\n", peek());
            }
            if (!have_id)
            {
                eval_ast e;
                u64 index = e.eval<u64>(i);
                auto *val = std::get_if<bool>(&array.value);
                if (val && *val == 1)
                {
                    parser::line = tok.line;
                    parser::column = tok.column;
                    throw ParseTimeError("\tAccesing uninitilyzed area of '" + id + "'\n");
                }
                if (index >= array.size)
                {
                    parser::line = tok.line;
                    parser::column = tok.column;
                    throw ParseTimeError("\tOverflowed index of array '" + id + "', size of array is '" +
                                         std::to_string(array.size) + "'\n");
                }
                if (index < 0)
                {
                    parser::line = tok.line;
                    parser::column = tok.column;
                    throw ParseTimeError("\tUnderflowed index of array '" + id + "'\n");
                }
            }
            if (have_id) 
                ParserWarning("\tAccessing array '" + id + "' with not compile time index, it may lead to errors", tok, "NonCompileTimeIndex");
            auto *val = get_if<bool>(&array.value);
            if (val && *val == 1)
            {
                parser::line = tok.line;
                parser::column = tok.column;
                throw ParseTimeError("\tAccesing uninitilyzed area of '" + id + "'\n");
            }
            consume(R_SQ_BRACKET);
            if (peek().type != EQ) {
                return std::make_unique<ArrayAccessNode>(tok, std::move(i), array.is_vector, array.is_ptr, array.type==STRING_TYPE);
            }
            consume(EQ);
            if (array.is_const)
            {
                parser::line = tok.line;
                parser::column = tok.column;
                throw ParseTimeError("\tArray '" + id + "' is constant'\n");
            }
            if (!have_id)
            {
                eval_ast e;
                u64 index = e.eval<u64>(i);
                if (index >= array.size)
                {
                    parser::line = tok.line;
                    parser::column = tok.column;
                    throw ParseTimeError("\tOverflowed index of array '" + id + "', size of array is '" +
                                         std::to_string(array.size) + "'\n");
                }
                if (index < 0)
                {
                    parser::line = tok.line;
                    parser::column = tok.column;
                    throw ParseTimeError("\tUnderflowed index of array '" + id + "'\n");
                }
            }
            ParserWarning("\tAccessing array '" + id + "' with not compile time index, it may lead to errors", tok, "NonCompileTimeIndex");
            astptr value = parse_expr();
            return std::make_unique<ArrayChangeNode>(tok, std::move(i), std::move(value), false, array.type==STRING_TYPE);
        }
        if (!exist(variant2string(tok.str_value)))
        {
            parser::line = tok.line;
            parser::column = tok.column;
            throw ParseTimeError("\tUse undeclared variable '" + variant2string(tok.str_value) + "'\n");
        }
        if (search(variant2string(tok.str_value)).comptime)
        {
            symbol var = search(variant2string(tok.str_value));
            switch (var.type)
            {
            case BYTE_TYPE ... LONG_TYPE:
                return std::make_unique<Node>(
                    token{.type = LONG, .value = var.value, .line = tok.line, .column = tok.column});
            case UNSIGNED_8_TYPE ... UNSIGNED_64_TYPE:
                return std::make_unique<Node>(
                    token{.type = UNSIGNED, .value = var.value, .line = tok.line, .column = tok.column});
            case FLOAT_TYPE:
                return std::make_unique<Node>(
                    token{.type = FLOAT, .value = var.value, .line = tok.line, .column = tok.column});
            case DOUBLE_TYPE:
                return std::make_unique<Node>(
                    token{.type = DOUBLE, .value = var.value, .line = tok.line, .column = tok.column});
            case STRING_TYPE:
                return std::make_unique<Node>(
                    token{.type = STRING, .value = var.value, .line = tok.line, .column = tok.column});
            default:
                return std::make_unique<Node>(
                    token{.type = LONG, .value = var.value, .line = tok.line, .column = tok.column});
            }
        }
        tok.str_value = get_name(tok.str_value);
        return std::make_unique<Node>(tok, search(tok.str_value).is_ptr, search(tok.str_value).is_ref_arg, search(tok.str_value).struct_);
    }
    else if (tok.type == token_type::L_BRACKET)
    {

        astptr node = parse_expr();

        token close = consume(token_type::R_BRACKET);

        return node;
    }
    else
    {
        parser::line = tok.line;
        parser::column = tok.column;
        throw ParseTimeError("\tUnexpected token '" + disassemble_tok_type(tok.type) + "'\n");
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

astptr parser::parse_and_b()
{
    astptr node = parse_unary();

    while (true)
    {
        token op = peek();
        if (op.type != token_type::AND_B)
            break;

        consume();
        astptr rhs = parse_unary();
        node = std::make_unique<BinaryNode>(std::move(node), std::move(rhs), op);
    }

    return node;
}

astptr parser::parse_xor()
{
    astptr node = parse_and_b();

    while (true)
    {
        token op = peek();
        if (op.type != token_type::XOR)
            break;

        consume();
        astptr rhs = parse_and_b();
        node = std::make_unique<BinaryNode>(std::move(node), std::move(rhs), op);
    }

    return node;
}

astptr parser::parse_or_b()
{
    astptr node = parse_xor();

    while (true)
    {
        token op = peek();
        if (op.type != token_type::OR_B)
            break;

        consume();
        astptr rhs = parse_xor();
        node = std::make_unique<BinaryNode>(std::move(node), std::move(rhs), op);
    }

    return node;
}

astptr parser::parse_term()
{
    astptr node = parse_or_b();

    while (true)
    {
        token op = peek();
        if (op.type != token_type::STAR && op.type != token_type::SLASH && op.type != token_type::MOD)
            break;

        consume();
        astptr rhs = parse_or_b();
        node = std::make_unique<BinaryNode>(std::move(node), std::move(rhs), op);
    }

    return node;
}

astptr parser::parse_expr()
{
    astptr node = parse_term();

    while (true)
    {
        token op = peek();
        if (op.type != token_type::PLUS && op.type != token_type::MINUS)
            break;
        consume();
        astptr rhs = parse_term();
        node = std::make_unique<BinaryNode>(std::move(node), std::move(rhs), op);
    }
    return node;
}

astptr parser::parse_use()
{
    token m = consume(USE);
    const std::string mname = consume().str_value;
    std::string name = mname + ".flame";
    for(auto &x : loadedModules) {
        if(x==name||mname=="std") {
            if(mname=="std") {
                std::cout << "std\n";
                loadedModules.emplace_back("std.flame");
                insert("fmt", FUNC, nothing{}, false,1, false, false, false, false, "std.flame");
            }
            consume(SEMI);
            return {};
        }
    }
    loadedModules.emplace_back(name);
    consume(SEMI);
    std::ifstream file(name);
    std::ostringstream oss;
    if (!file.is_open())
    {
        std::string home = std::getenv("HOME");
        file.open((home + std::string{"/.local/bin/flame_/"} + name));
        oss << file.rdbuf();
        std::string code = oss.str();
        file.close();
        lexer l;
        std::vector<token> toks = l.lex(code);
        parser p(toks, name);
        p.c_gen = c_gen;
        p.is_module = true;
        std::vector<astptr> module;
        try
        {
            while (p.peek().type != token_type::EOF_)
            {
                module.push_back(p.parse_statement());
            }
        }
        catch (ParseTimeError &e)
        {
            std::cerr << "\x1b[0;35m" << name << ':' << p.line << ':' << p.column << ": \x1b[31m error\n"
                      << e.what() << "\x1b[0m";
            parser::line = m.line;
            parser::column = m.column;
            throw ParseTimeError("\tError in module '" + name + "'\n");
        }
        return std::make_unique<ModuleNode>(std::move(module), mname);
    }
    if (!file)
    {
        std::cerr << "Cannot open file " << name << " or maybe it doesnt exits\n";
        file.close();
        exit(1);
    }
    oss << file.rdbuf();
    std::string code = oss.str();
    file.close();
    lexer l;
    std::vector<token> toks = l.lex(code);
    parser p(toks, name);
    p.c_gen = c_gen;
    p.is_module = true; // tells later to table that we parsing module
    std::vector<astptr> module;
    while (p.peek().type != token_type::EOF_)
    {
        try
        {
            module.push_back(p.parse_statement());
        }
        catch (ParseTimeError &e)
        {
            std::cerr << "\x1b[0;35m" << filename << ':' << line << ':' << column << ": \x1b[31m error\n"
                      << e.what() << "\x1b[0m";
            sync();
        }
    }
    return std::make_unique<ModuleNode>(std::move(module), mname);
}

astptr parser::parse_shift()
{
    astptr node = parse_expr();
    while (true)
    {
        token op = peek();
        if (op.type != SHIFT_R && op.type != SHIFT_L)
            break;
        consume();
        astptr rhs = parse_expr();
        node = std::make_unique<CondNode>(std::move(node), std::move(rhs), op.type);
    }
    return node;
}

astptr parser::parse_comparison()
{
    astptr node = parse_shift();
    while (true)
    {
        token op = peek();
        if (op.type != LESS && op.type != BIGGER && op.type != LEQUAL && op.type != BEQUAL)
            break;
        consume();
        astptr rhs = parse_shift();
        node = std::make_unique<CondNode>(std::move(node), std::move(rhs), op.type);
    }
    return node;
}

astptr parser::parse_equality()
{
    astptr node = parse_comparison();
    while (true)
    {
        token op = peek();
        if (op.type != EQUAL && op.type != NEQUAL)
            break;
        consume();
        astptr rhs = parse_comparison();
        node = std::make_unique<CondNode>(std::move(node), std::move(rhs), op.type);
    }
    return node;
}

astptr parser::parse_and()
{
    astptr node = parse_equality();
    while (peek().type == AND)
    {
        consume();
        astptr rhs = parse_equality();
        node = std::make_unique<CondNode>(std::move(node), std::move(rhs), AND);
    }
    return node;
}

astptr parser::parse_or()
{
    astptr node = parse_and();
    while (peek().type == OR)
    {
        consume();
        astptr rhs = parse_and();
        node = std::make_unique<CondNode>(std::move(node), std::move(rhs), OR);
    }
    return node;
}

astptr parser::parse_return(const token_type& expectedType, bool isptr_return)
{
    consume(RETURN);
    u64 rollback = indx; // rollback index
    // code analyse
    while(peek().type!=SEMI&&!(expectedType==NULL_)) {
        token t = consume();
        if(t.type==ID) {
            // if return ID or similar
            symbol s = search(t.str_value);
            if(s.type==FUNC) s.type = fsearch(s.name)->type;
            // if return x is i32 but function returning string
            if(s.type!=expectedType) {
                // integer type promotion
                if(s.type<expectedType&&expectedType<STRING_TYPE) {
                    ParserWarning("\tHidden int casting from "
                        + disassemble_tok_type(s.type) + " to "
                        + disassemble_tok_type(expectedType)
                        , t, "HiddenCast");
                }
                else {
                    ParserError("\tCannot return '" + disassemble_tok_type(s.type) +
                 "', expected '" + disassemble_tok_type(expectedType) + "'\n" 
                    , t);
                }
            }
            // if func dummy() i32* {} but we returning i32 
            if(!s.is_ptr&&isptr_return) {
                ParserError("\tExpected returning pointer\n", t);
            }
        }
    }
    indx = rollback;
    if (peek().type == SEMI)
    {
        consume(SEMI);
        return std::make_unique<ReturnNode>(nullptr);
    }
    astptr node = parse_or();
    consume(SEMI);
    return std::make_unique<ReturnNode>(std::move(node));
}

astptr parser::parse_block(const token_type& expectedType, bool isptr_return)
{
    table.emplace_back();
    consume(L_BRACES);
    std::vector<astptr> stmts;
    bool seen_return = false;
    while (peek().type != R_BRACES && indx < src.size())
    {
        if (peek().type == RETURN) {
            for(auto &x : std::ranges::reverse_view(table)[0]) {
                if(x.second.type==STRING_TYPE&&c_gen) {
                    token tok = token{.type=ID, .value=nothing{}, .line=0, .column=0, .str_value = "&" + x.second.name};
                    std::vector<astptr> args;
                    args.emplace_back(std::make_unique<Node>(tok));
                    stmts.emplace_back(
                        std::make_unique<FuncCallNode>(
                            "oxygen_string_destroy",
                            std::move(args),
                            ""
                        )
                    );
                }
                if(x.second.is_ptr&&c_gen) {
                    token tok = token{.type=ID, .value=nothing{}, .line=0, .column=0, .str_value = x.second.name};
                    std::vector<astptr> args;
                    args.emplace_back(std::make_unique<Node>(tok));
                    stmts.emplace_back(
                        std::make_unique<FuncCallNode>(
                            "free",
                            std::move(args),
                            ""
                        )
                    );
                }
            }
            seen_return = true;
            if(expectedType !=NULL_) {
                stmts.emplace_back(parse_return(expectedType, isptr_return));
                continue;
            }
        }
        try
        {
            stmts.push_back(parse_statement());
        }
        catch (ParseTimeError &e)
        {
            std::cerr << "\x1b[0;35m" << filename << ':' << line << ':' << column << ": \x1b[31m error\n"
                      << e.what() << "\x1b[0m";
            sync();
            errors = true;
        }
    }
    if (seen_return)
        returning = true;
    consume(R_BRACES);
    table.pop_back();
    return std::make_unique<BlockNode>(std::move(stmts));
}

astptr parser::parse_func_statement(const std::string &struct_)
{
    consume(FUNC);
    token id = consume(ID);
    consume(L_BRACKET);
    finsert(id.str_value, FUNC, {}, struct_ + '.');
    std::vector<astptr> args_;
    table.emplace_back();
    while (peek().type != R_BRACKET)
    {
        bool is_const = false;
        bool is_ref = false;
        bool is_mut = false;
        if (peek().type == CONST)
        {
            consume(CONST);
            is_const = true;
        }
        if(peek().type == MUT) {
            consume(MUT);
            is_mut = true;
            is_const = false;
        }
        if (peek().type==REF)
        {
            consume(REF);
            if(!is_mut) is_const = true;
            is_ref = true;
        }
        token type = consume();
        bool is_array = false;
        u64 i = 1;
        if (peek().type == L_SQ_BRACKET)
        {
            consume(L_SQ_BRACKET);
            token size = peek();
            if (!is_it_int_value(size) || (size.str_value != "" && !search(size.str_value).comptime))
            {
                parser::line = type.line;
                parser::column = type.column;
                throw ParseTimeError("\tExpected compile time size\n");
            }
            eval_ast e;
            i = e.eval<u64>(parse_factor());
            consume(R_SQ_BRACKET);
            is_array = true;
        }
        token arg_id = consume(ID);
        if (!is_it_type(type))
        {
            parser::line = type.line;
            parser::column = type.column;
            throw ParseTimeError("\tUnknown type of argument '" + variant2string(arg_id.str_value) +
                                 "', expected i8..i64, u8..u64, bool, string, f32, f64, auto or Type[]\n");
        }
        insert(arg_id.str_value, type.type, nothing{}, false, i, is_array, false, false, false, "", false, is_mut);
        astptr argument = std::make_unique<ArgumentNode>(type, get_name(arg_id.str_value), is_array, i, is_ref, is_const, is_mut);
        args_.push_back(std::move(argument));
        finsert_arg(id.str_value,
                    symbol{.type = type.type,
                     .value = nothing{},
                     .is_const = is_const,
                     .size = i,
                     .is_array = is_array,
                     .name = arg_id.str_value,
                     .is_ref_arg=is_mut
                    });
        if (peek().type == COMA)
            consume();
    }
    consume(R_BRACKET);
    token return_type = consume();
    const bool is_ptr = peek().type == STAR;
    bool is_array = false;
    u64 size = 1;
    if (peek().type == L_SQ_BRACKET)
    {
        is_array = true;
        consume(L_SQ_BRACKET);
        astptr s = parse_expr();
        eval_ast e;
        size = e.eval<u64>(s);
        consume(R_SQ_BRACKET);
    }
    if (!is_it_type(return_type) && return_type.type != VOID_TYPE)
    {
        parser::line = return_type.line;
        parser::column = return_type.column;
        throw ParseTimeError("\tUnknown return type in " + id.str_value +
                             ", expected i8..64, u8..64, bool, string, void, f32, f64, auto\n");
    }
    astptr block = parse_block(return_type.type);
    if (!returning && return_type.type != VOID_TYPE)
    {
        parser::line = id.line;
        parser::column = id.column;
        throw ParseTimeError("\tFunction " + id.str_value + " doesnt have returning\n");
    }
    table.pop_back();
    returning = false;
    fsearch(id.str_value)->type=return_type.type;
    if(is_module) insert(id.str_value, FUNC, nothing{}, false,1, false, false, false, false, filename, false, false, struct_);
    else insert(id.str_value, FUNC, nothing{}, false, 1, false, false, false, false, "", false, false, struct_);
    return std::make_unique<FuncNode>(get_name(id.str_value, struct_), return_type, std::move(args_), std::move(block), is_array, size);
}

astptr parser::parse_if_statement()
{
    consume(IF);
    consume(L_BRACKET);
    token next = peek();
    if (next.type == R_BRACKET)
    {
        parser::line = next.line;
        parser::column = next.column;
        throw ParseTimeError("\tExpected condition in if, like if(a<10)\n");
    }
    astptr cond = parse_or();
    consume(R_BRACKET);
    astptr block = parse_block();
    astptr node = std::make_unique<IfNode>(std::move(cond), std::move(block), astptr{}, IF);
    IfNode *current = static_cast<IfNode *>(node.get());
    while (peek().type == ELIF || peek().type == ELSE)
    {
        token a = consume(); // ELIF or ELSE
        if (a.type == ELIF)
        {
            consume(L_BRACKET);
            astptr cond_elif = parse_or();
            consume(R_BRACKET);
            astptr block_elif = parse_block();
            current->next = std::make_unique<IfNode>(std::move(cond_elif), std::move(block_elif), astptr{}, ELIF);
            current = static_cast<IfNode *>(current->next.get());
        }
        if (a.type == ELSE)
        {
            astptr block_else = parse_block();
            current->next = std::make_unique<IfNode>(astptr{}, std::move(block_else), astptr{}, ELSE);
            break;
        }
    }
    return node;
}

astptr parser::parse_while_statement()
{
    consume(WHILE);
    consume(L_BRACKET);
    astptr cond = parse_or();
    consume(R_BRACKET);
    astptr block = parse_block();
    return std::make_unique<LoopNode>(std::move(cond), std::move(block), WHILE);
}

astptr parser::parse_for_statement()
{
    consume(FOR);
    consume(L_BRACKET);
    astptr var = parse_assignment();
    astptr cond = parse_or();
    consume(SEMI);
    astptr thing = parse_assignment();
    consume(R_BRACKET);
    astptr block = parse_block();
    return std::make_unique<ForNode>(std::move(cond), std::move(block), std::move(var), std::move(thing));
}

astptr parser::parse_break_continue()
{
    token a = consume();
    if (a.type == BREAK)
    {
        consume(SEMI);
        return std::make_unique<BreakNode>();
    }
    else if (a.type == CONTINUE)
    {
        consume(SEMI);
        return std::make_unique<ContinueNode>();
    }
    else
    {
        parser::line = a.line;
        parser::column = a.column;
        throw ParseTimeError("\tError in parsing break or continue, expected break or continue\n");
    }
}

astptr parser::parse_array(bool is_const, const std::string &struct_)
{
    if (peek().type == ID && peek(1).type == L_SQ_BRACKET)
        return parse_factor();
    if (!is_it_type(peek()) || !(peek(1).type == L_SQ_BRACKET))
    {
        parser::line = peek().line;
        parser::column = peek().column;
        throw ParseTimeError("\tError in parsing array, expected syntax like this: Type[]\n");
    }
    token type = consume();
    consume(L_SQ_BRACKET);
    token size;
    size.value = 0ULL;
    bool size_defined = false;
    if (peek().type != R_SQ_BRACKET)
    {
        size = consume();
        size_defined = true;
    }
    consume(R_SQ_BRACKET);
    std::string id = variant2string(consume(ID).str_value);
    if (peek().type == SEMI)
    {
        consume(SEMI);
        if(is_module) insert(struct_ + id, type.type, true, is_const, std::get<u64>(size.value), true, false, false, false, filename);
        else  insert(struct_ + id, type.type, nothing{}, is_const, std::get<u64>(size.value), true);
        return std::make_unique<ArrayNode>(type,
                                           std::vector<astptr>{},
                                           id,
                                           variant2int<unsigned long long>(size.value),
                                           false);
        /*
        token semi = consume(SEMI);
        parser::line = semi.line;
        parser::column = semi.column;
        throw ParseTimeError("\tDeclaring array '" + id + "' without initializing values\n");
        */
    }
    consume(EQ);
    if (peek().type == ID && !size_defined)
    {
        parser::line = peek().line;
        parser::column = peek().column;
        throw ParseTimeError("\tValue of array '" + id + "' must be known at compile time\n");
    }
    else if (peek().type == ID)
    {
        astptr value = parse_factor();
        consume(SEMI);
        std::vector<astptr> values;
        values.emplace_back(std::move(value));
        if(is_module) insert(struct_ + id, type.type, nothing{}, is_const, std::get<u64>(size.value), true, false, false, false, filename);
        else insert(struct_ + id, type.type, nothing{}, is_const, std::get<u64>(size.value), true);
        return std::make_unique<ArrayNode>(type,
                                           std::move(values),
                                           id,
                                           variant2int<unsigned long long>(size.value),
                                           true);
    }
    consume(L_SQ_BRACKET);
    std::vector<astptr> values;
    u64 els = 0;
    while (peek().type != R_SQ_BRACKET)
    {
        astptr value = parse_or();
        if (peek().type == COMA)
            consume();
        values.push_back(std::move(value));
        els++;
    }
    consume(R_SQ_BRACKET);
    consume(SEMI);
    if(is_module) insert(struct_ + id, type.type, nothing{}, is_const, els, true, false, false, false, filename);
    else insert(struct_ + id, type.type, nothing{}, is_const, els, true);
    if (size_defined)
        return std::make_unique<ArrayNode>(type, std::move(values), id, variant2int<unsigned long long>(size.value));
    else
        return std::make_unique<ArrayNode>(type, std::move(values), id, els);
}

void parser::parse_comptime()
{
    consume(COMPTIME);
    token type = consume();
    if (!is_it_type(type))
    {
        parser::line = type.line;
        parser::column = type.column;
        throw ParseTimeError("\tUnexpected type " + std::to_string(type.type) + '\n');
    }
    std::string id = variant2string(consume(ID).str_value);
    consume(EQ);
    u64 i = 0;
    while (i < src.size())
    {
        if (peek(i).type == ID && !search(peek(i).str_value).comptime)
        {
            parser::line = peek(i).line;
            parser::column = peek(i).column;
            throw ParseTimeError("\tValue cannot be evaluated in compile time\n");
        }
        if (peek(i).type == SEMI)
            break;
        i++;
    }
    astptr val_ = parse_expr();
    eval_ast e;
    if (type.type <= LONG_TYPE || type.type == BOOL_TYPE)
    {
        i64 val = e.eval<i64>(val_);
        insert(id, type.type, val, false, false, false, true);
    }
    else if (type.type > LONG_TYPE && type.type <= UNSIGNED_64_TYPE)
    {
        u64 val = e.eval<u64>(val_);
        insert(id, type.type, val, false, false, false, true);
    }
    else if (type.type == STRING_TYPE)
    {
        std::string val = e.eval_string(val_);
        insert(id, type.type, val, false, false, false, true);
    }
    else if (type.type == DOUBLE_TYPE || type.type == FLOAT_TYPE)
    {
        auto val = e.eval_double(val_);
        insert(id, type.type, val, false, false, false, true);
    }
    consume(SEMI);
    return;
}

astptr parser::parse_assignment(bool is_const, bool comptime, const std::string &struct_)
{
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
            parser::line = id.line;
            parser::column = id.column;
            throw ParseTimeError("\tUse undeclared variable '" + id.str_value + "'\n");
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
                parser::line = type.line;
                parser::column = type.column;
                throw ParseTimeError("\tUse undeclared variable '" + type.str_value + "'\n");
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
            parser::line = type.line;
            parser::column = type.column;
            throw ParseTimeError("\tUse undeclared variable '" + variant2string(type.str_value) + "'\n");
        }
        if (search(variant2string(type.str_value)).is_const || is_const)
        {
            parser::line = type.line;
            parser::column = type.column;
            throw ParseTimeError("\tVariable '" + variant2string(type.str_value) + "' is constant'\n");
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
        parser::line = type.line;
        parser::column = type.column;
        throw ParseTimeError("\tUnexpected type " + std::to_string(type.type) + '\n');
    }
    if (peek().type == STAR)
    {
        consume(STAR);
        token id = consume(token_type::ID);
        std::string id_value = id.str_value;


        if (exist_in_scope(id_value, table.size() - 1) && !exist_in_scope(id_value, 0))
        {
            parser::line = id.line;
            parser::column = id.column;
            throw ParseTimeError("\tRedefinition of '" + id.str_value + "'\n");
        }
        else if (search_type(id_value) == FUNC)
        {
            parser::line = id.line;
            parser::column = id.column;
            throw ParseTimeError("\tRedefinition function '" + id.str_value + "' as variable\n");
        }
        else if (search(id_value).is_const)
        {
            parser::line = id.line;
            parser::column = id.column;
            throw ParseTimeError("\tVariable '" + id_value + "' is constant'\n");
        }
        else if (search(id_value).comptime)
        {
            parser::line = id.line;
            parser::column = id.column;
            throw ParseTimeError("\tVariable '" + id_value + "' is compile time value'\n");
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
        const u64 rollback = indx;
        while(peek().type!=SEMI) {
            token t = consume();
            if(t.type==ID) {
                symbol* s = searchptr(t.str_value);
                if(!s && !s->is_ptr) continue;
                if(s->is_moved) {
                    ParserError("\t'"+t.str_value+"' is already moved\n", t);
                }
                if(s->type!=type.type) {
                    ParserError("\tCannot move pointer '"+t.str_value+"' to '"
                        + id.str_value + "', because types dont match\n"
                        , t);
                }
                symbol str = search(type.str_value);
                if(str.type==STRING_TYPE&&s->type!=STRING_TYPE) {
                    ParserError("\tCannot define string variable with non string value\n", t);
                }
                if(str.type==STRING_TYPE) size = str.size;
                mov_from = s->name;
            }
            if(t.type==STRING) {
                size = t.str_value.size();
            }
        }
        indx = rollback;
        astptr value = parse_factor();
        consume(SEMI);
        if(mov_from!="") {
            symbol* s = searchptr(mov_from);
            if(s) s->is_moved=true;
        }
        const bool ismov = mov_from!="";
        if(is_module) insert(struct_ + id.str_value, type.type, nothing{}, is_const, size, false, false,false,true, filename, false, false, struct_);
        else insert(struct_ + id.str_value, type.type, nothing{}, is_const, size, false, false,false,true, "", false, false, struct_);
        return std::make_unique<AssignmentNodeExpr>(type.type, get_name(id.str_value), std::move(value), is_const, struct_, true, ismov);
    }
    token id = consume(token_type::ID);
    std::string id_value = id.str_value;
    if (exist_in_scope(id_value, table.size() - 1) && !exist_in_scope(id_value, 0))
    {
        parser::line = id.line;
        parser::column = id.column;
        throw ParseTimeError("\tRedefinition of '" + id.str_value + "'\n");
    }
    else if (search_type(id_value) == FUNC)
    {
        parser::line = id.line;
        parser::column = id.column;
        throw ParseTimeError("\tRedefinition function '" + id.str_value + "' as variable\n");
    }
    else if (search(id_value).is_const)
    {
        parser::line = id.line;
        parser::column = id.column;
        throw ParseTimeError("\tVariable '" + id_value + "' is constant'\n");
    }
    else if (search(id_value).comptime)
    {
        parser::line = id.line;
        parser::column = id.column;
        throw ParseTimeError("\tVariable '" + id_value + "' is compile time value'\n");
    }
    if (peek().type == SEMI)
    {
        consume();
        if(is_module) insert(struct_ + id.str_value, type.type, nothing{}, is_const, false, false, false, false, false, filename, false, false, struct_);
        else insert(struct_ + id.str_value, type.type, nothing{}, is_const,1, false, false, false, false, "", false, false, struct_);
        return std::make_unique<AssignmentNodeExpr>(type.type, get_name(id.str_value), astptr{}, is_const, struct_);
    }
    consume(token_type::EQ);
    u64 size = 0;
    if(type.type==STRING_TYPE) {
        token t = peek();
        if(t.type!=STRING) {
            if(t.type!=ID) {
                ParserError("\tCannot define string variable with non string value\n", t);
            }
            symbol s = search(t.str_value);
            if(s.type!=STRING_TYPE) {
                ParserError("\tCannot define string variable with non string value\n", t);
            }
            size = s.size;
        } else {
            size = t.str_value.size();
        }
    }
    astptr value = parse_or();
    if(is_module&&!comptime) insert(id.str_value, type.type, nothing{}, is_const, size, false, false, false, false, filename, false, false, struct_);
    else insert(id.str_value, type.type, nothing{}, is_const, size,  false, comptime, false, false, "", false, false, struct_);
    consume(SEMI);
    return std::make_unique<AssignmentNodeExpr>(type.type, get_name(id.str_value), std::move(value), is_const, struct_);
}



astptr parser::parse_method()
{
    token parent = consume(ID);

    for(auto &x : loadedModules) {
        if(x==parent.str_value+".flame") {
            return parse_module_call(parent.str_value+".flame");
        }
    }


    if (!exist(parent.str_value))
    {
        parser::line = parent.line;
        parser::column = parent.column;
        throw ParseTimeError("\tUse undeclared variable '" + parent.str_value + "'\n");
    }
    symbol var = search(parent.str_value);
    std::string type = var.struct_;
    bool isptr = false;
    if(var.is_ptr) isptr = true;
    std::vector<astptr> children;
    std::vector<bool> isptrs;

    while (peek().type == DOT)
    {
        consume(DOT);
        token child = consume(ID);
        if (search(child.str_value).struct_!=search(parent.str_value).struct_)
        {
            parser::line = child.line;
            parser::column = child.column;
            throw ParseTimeError("\tUse undeclared variable '" + child.str_value + "'\n");
        }
        isptrs.push_back(search(variant2string(search(parent.str_value).value) + child.str_value).is_ptr);
        if (peek().type == L_BRACKET)
        {
            consume(L_BRACKET);
            std::vector<astptr> args_;
            u64 arg_i = 0;
            fsymbol *f = fsearch(child.str_value);
            if (child.str_value == "push" && search(parent.str_value).is_vector)
            {
                symbol *a = searchptr(parent.str_value);
                if (a)
                    a->size++;
            }
            if (child.str_value == "pop" && search(parent.str_value).is_vector)
            {
                symbol *a = searchptr(parent.str_value);
                if (a)
                    a->size--;
            }
            std::vector<symbol> mut_vars;
            while (peek().type != R_BRACKET)
            {
                token c = peek();
                symbol* s = searchptr(c.str_value);
                token n = peek(1);
                if ((c.type == ID && (n.type == COMA || n.type == R_BRACKET) && f))
                {
                    if (arg_i >= f->args.size())
                    {
                        parser::line = c.line;
                        parser::column = c.column;
                        throw ParseTimeError("\tExpected " + std::to_string(f->args.size()) + " arguments, got " +
                                             std::to_string(arg_i) + "\n");
                    }
                    if (search(c.str_value).type != f->args[arg_i].type)
                    {
                        parser::line = c.line;
                        parser::column = c.column;
                        throw ParseTimeError("\tExpected argument of type '" +
                                             disassemble_tok_type(f->args[arg_i].type) + "', but got '" +
                                             disassemble_tok_type(search_type(c.str_value)) + "'\n");
                    }
                    if (search(c.str_value).is_array && search(c.str_value).size != f->args[arg_i].size)
                    {
                        parser::line = c.line;
                        parser::column = c.column;
                        throw ParseTimeError("\tExpected array of size '" + std::to_string(f->args[arg_i].size) +
                                             "'\n");
                    }
                    if (s->is_array && search(c.str_value).size != f->args[arg_i].size)
                    {
                        ParserError("\tExpected array of size '" + std::to_string(f->args[arg_i].size) +
                                             "'\n", c);
                    }
                    if(f->args[arg_i].is_ref_arg) {
                        for(auto &x : mut_vars) {
                            if(s->name==x.name) {
                                ParserError("\tVariable can only have one mutable reference\n", c);
                            }
                        }
                        mut_vars.emplace_back(*s);
                        s->is_mut_now=false;
                    }
                }
                if(f && f->args[arg_i].is_ref_arg && f->args[arg_i].type!=c.type) {
                    ParserError("\tCannot pass temporarily value as mutable reference\n", c);
                }
                arg_i++;
                args_.push_back(parse_or());
                if (peek().type == COMA)
                    consume();
            }
            consume(R_BRACKET);
            children.emplace_back(std::make_unique<FuncCallNode>(child.str_value, std::move(args_), ""));
            if (peek().type == SEMI)
                consume(SEMI);
        }
        else
        {
            if (peek().type == L_SQ_BRACKET)
            {
                consume(L_SQ_BRACKET);
                astptr index = parse_expr();
                consume(R_SQ_BRACKET);
                symbol s = search(variant2string(search(parent.str_value).value) + child.str_value);
                if(!s.is_array&&s.type!=STRING_TYPE) {
                    ParserError("\tTrying access variable '"+s.name+"' as array\n", peek());
                }
                children.emplace_back(std::make_unique<ArrayAccessNode>(child, std::move(index), s.is_vector, s.type==STRING_TYPE));
            }
            else
            {
                if (peek().type == PLUS || peek().type == MINUS || peek().type == STAR || peek().type == SLASH ||
                    peek().type == MOD || peek().type == XOR || peek().type == AND_B || peek().type == OR_B ||
                    peek().type == SHIFT_L || peek().type == SHIFT_R) {
                    token op = consume();
                    if (!exist_in_scope(child.str_value, LAST_SCOPE_N)||search(child.str_value).struct_!=parent.str_value)
                    {
                        ParserError("\tUse undeclared field '"+child.str_value+"'\n", child);
                    }
                    consume(EQ);

                    u64 rollback = indx;
                    symbol s = search(child.str_value);
                    while(peek().type!=SEMI) {
                        token t = consume();
                        if(is_it_int_value(t)) {
                            if(!is_it_int_type(s.type)) {
                                ParserError("\tTrying modify non-int variable '"
                                + child.str_value + "'\n", t);
                            }
                        } else if(t.type==ID) {
                            symbol s_ = search(t.str_value);
                            if(s.type!=s_.type) {
                                if(!is_it_int_type(s.type)&&!is_it_int_type(s_.type)) {
                                    ParserError("\tTrying modify variable '"
                                    + child.str_value + "' with value of '" + s_.name + "', but types dont match\n", t);
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
                    return std::make_unique<ReAssignmentNodeExpr>(op.type, child.str_value, std::move(value), false, parent.str_value, search(parent.str_value).is_ptr);
                }
                if(peek().type==EQ) {
                    // reasingning -> p.x = 32;
                    if(search(child.str_value).is_const) {
                        parser::line = child.line;
                        parser::column = child.column;
                        throw ParseTimeError("\tVariable '" + child.str_value +
                                             "' is constant\n");
                    }
                    consume(EQ);
                    astptr value = parse_or();
                    consume(SEMI);
                    children.emplace_back(std::make_unique<AssignmentNode>(child.str_value, std::move(value), false, search(child.str_value).is_ptr, false, parent.str_value));
                }
                else
                {
                    children.emplace_back(std::make_unique<Node>(child, false, false, parent.str_value)); // access -> p.x
                }
            }
        }
    }
    if (var.is_vector)
        return std::make_unique<MethodNode>(std::move(children), isptrs, parent.str_value, VEC, isptr, type);
    if (peek().type == SEMI)
        consume(SEMI);
    return std::make_unique<MethodNode>(std::move(children), isptrs, parent.str_value, search_type(parent.str_value), isptr, type);
}

astptr parser::parse_std(const std::string& name) {
    if(name=="fmt") {
        consume(L_BRACKET);
        std::vector<astptr> args_;
        u64 arg_i = 0;
        std::string fmt = "";
        while (peek().type != R_BRACKET) {
            token c = peek();
            token n = peek(1);
            if(arg_i==0&&c.type!=STRING) {
                ParserError("\tstd.fmt requires first argument to be string\n", c);
            }
            if(arg_i!=0) {
                if ((c.type == ID && (n.type == COMA || n.type == R_BRACKET))) {
                    symbol s = search(c.str_value);
                    if(is_it_int_type(s.type)&&s.type!=UNSIGNED_8_TYPE&&s.type!=UNSIGNED_16_TYPE&&s.type!=UNSIGNED_32_TYPE&&s.type!=UNSIGNED_64_TYPE) {
                        fmt += "i";
                    } else if(is_it_int_type(s.type)) {
                        fmt += "u"; // unsigned
                    } else if(s.type==STRING_TYPE) {
                        fmt += "s";
                    } else if(s.type==FLOAT_TYPE||s.type==DOUBLE_TYPE) {
                        fmt += "f";
                    } else if(s.type==BOOL_TYPE) {
                        fmt += "b";
                    }
                } else {
                    if(c.type==MINUS&&is_it_int_value(n)) {
                        fmt += "i";
                    } else if(is_it_int_value(c)) {
                        fmt += "u"; // unsigned
                    } else if(c.type==STRING) {
                        fmt += "s";
                    } else if(c.type==FLOAT||c.type==DOUBLE) {
                        fmt += "f";
                    } else if(c.type==TRUE||c.type==FALSE) {
                        fmt += "b";
                    }
                }
            }
            arg_i++;
            args_.push_back(parse_or());
            if (peek().type == COMA) consume();
        }
        consume(R_BRACKET);
        return std::make_unique<FuncCallNode>("fmt", std::move(args_), fmt);
    }
    return nullptr;
}

astptr parser::parse_module_call(const std::string &name) {
    std::vector<astptr> children;
    std::vector<bool> isptrs;

    size_t s = name.find_last_of('.');
    std::string cname = (s == std::string::npos) ? name : name.substr(0, s);

    bool consume_semi = true;
    while (peek().type == DOT)
    {
        consume(DOT);
        token child = consume(ID);
        if (!exist_module(child.str_value, name))
        {
            parser::line = child.line;
            parser::column = child.column;
            throw ParseTimeError("\tUse undeclared variable '" + child.str_value + "' in module " + name + "\n");
        }
        isptrs.push_back(search_module(child.str_value, name).is_ptr);
        if (peek().type == L_BRACKET)
        {
            if(child.str_value=="fmt"&&cname=="std") {
                astptr fmt = parse_std("fmt");
                children.emplace_back(std::move(fmt));
            } else {
                consume(L_BRACKET);
                std::vector<astptr> args_;
                u64 arg_i = 0;
                fsymbol *f = fsearch_module(child.str_value, name);
                while (peek().type != R_BRACKET)
                {
                    token c = peek();
                    token n = peek(1);
                    if ((c.type == ID && (n.type == COMA || n.type == R_BRACKET) && f))
                    {
                        //if(search_module(c.str_value, name).type!)
                        if (arg_i >= f->args.size())
                        {
                            parser::line = c.line;
                            parser::column = c.column;
                            throw ParseTimeError("\tExpected " + std::to_string(f->args.size()) + " arguments, got " +
                                                std::to_string(arg_i) + "\n");
                        }
                        if (search_module(c.str_value, name).type != f->args[arg_i].type)
                        {
                            parser::line = c.line;
                            parser::column = c.column;
                            throw ParseTimeError("\tExpected argument of type '" +
                                                disassemble_tok_type(f->args[arg_i].type) + "', but got '" +
                                                disassemble_tok_type(search_type(c.str_value)) + "'\n");
                        }
                        if (search_module(c.str_value, name).is_array && search_module(c.str_value, name).size != f->args[arg_i].size)
                        {
                            parser::line = c.line;
                            parser::column = c.column;
                            throw ParseTimeError("\tExpected array of size '" + std::to_string(f->args[arg_i].size) +
                                                "'\n");
                        }
                    }
                    arg_i++;
                    args_.push_back(parse_or());
                    if (peek().type == COMA)
                        consume();
                }
                consume(R_BRACKET);
                if(!c_gen)  child.str_value = cname + "::" + child.str_value;
                children.emplace_back(std::make_unique<FuncCallNode>(get_name(child.str_value, cname), std::move(args_), ""));
            }
        }
        else
        {
            if (peek().type == L_SQ_BRACKET)
            {
                consume(L_SQ_BRACKET);
                astptr index = parse_expr();
                consume(R_SQ_BRACKET);
                symbol s = search_module(child.str_value, name);
                if(!s.is_array&&s.type!=STRING_TYPE) {
                    ParserError("\tTrying access variable '"+s.name+"' as array\n", peek());
                }
                child.str_value = search_module(child.str_value, name).name;
                if(!c_gen) child.str_value = cname + "::" + child.str_value;
                children.emplace_back(std::make_unique<ArrayAccessNode>(child, std::move(index), s.is_vector, s.type==STRING_TYPE));
            }
            else
            {
                
                if(peek().type==EQ) {
                    // reasingning -> p.x = 32;
                    if(search(child.str_value).is_const) {
                        ParserError("\tVariable '" + child.str_value +
                                             "' is constant\n", child);
                    }
                    consume(EQ);
                    astptr value = parse_or();
                    if(!c_gen) child.str_value = cname + "::" + child.str_value;
                    children.emplace_back(std::make_unique<AssignmentNode>(get_name(child.str_value, cname), std::move(value)));
                }
                else
                {   
                    auto s = search_module(child.str_value, name);

                    if(c_gen) child.str_value = get_name(s.name, name, false);
                    else child.str_value = cname + "::" + child.str_value;
                    children.emplace_back(std::make_unique<Node>(child)); // access -> p.x
                    consume_semi = false;
                }
            }
        }
    }
    if (peek().type == SEMI&&consume_semi)
        consume(SEMI);
    return std::make_unique<ModuleCallNode>(std::move(children), isptrs, name);
}

astptr parser::parse_vector(bool is_const, const std::string &struct_)
{
    consume(VEC);
    token type = consume();
    if (!is_it_type(type))
    {
        ParserError("\tUnknow type of vector\n", type);
    }
    if (type.type == AUTO_TYPE)
    {
        ParserError("\tVector cannot be defined with 'auto' type\n", type);
    }
    token id = consume(ID);
    if (peek().type == SEMI)
    {
        consume(SEMI);
        if(is_module) insert(struct_ + id.str_value, type.type, true, is_const, 0, true, false, true, false, filename);
        else insert(struct_ + id.str_value, type.type, true, is_const, 0, true, false, true);
        return std::make_unique<ArrayNode>(type, std::vector<astptr>{}, id.str_value, 0, false, true);
    }
    consume(EQ);
    if (peek().type == ID)
    {
        astptr value = parse_factor();
        consume(SEMI);
        std::vector<astptr> values;
        symbol var = search(peek().str_value);
        u64 size = var.type != FUNC ? var.size : 0;
        values.emplace_back(std::move(value));
        if(is_module) insert(struct_ + id.str_value, type.type, nothing{}, is_const, size, true, false, true, false, filename);
        else insert(struct_ + id.str_value, type.type, nothing{}, is_const, size, true, false, true);
        return std::make_unique<ArrayNode>(type, std::move(values), id.str_value, 0, true, true);
    }
    consume(L_SQ_BRACKET);
    std::vector<astptr> values;
    u64 els = 0;
    while (peek().type != R_SQ_BRACKET)
    {
        astptr value = parse_or();
        if (peek().type == COMA)
            consume();
        values.push_back(std::move(value));
        els++;
    }
    consume(R_SQ_BRACKET);
    consume(SEMI);
    if(is_module) insert(struct_ + id.str_value, type.type, nothing{}, is_const, els, true, false, true, false, filename);
    else insert(struct_ + id.str_value, type.type, nothing{}, is_const, els, true, false, true);
    return std::make_unique<ArrayNode>(type, std::move(values), id.str_value, els, false, true);
}

astptr parser::parse_struct()
{
    consume(STRUCT);
    token id = consume(ID);
    consume(L_BRACES);
    std::vector<astptr> block;
    while (peek().type != R_BRACES)
    {
        if (peek().type != FUNC) {
            bool is_const = peek().type == CONST ? true : false;
            block.push_back(parse_assignment(is_const, false, id.str_value));
        }
        else {
            block.push_back(parse_func_statement(id.str_value));
        }
    }
    consume(R_BRACES);
    consume(SEMI);
    struct_list.push_back(id.str_value);
    return std::make_unique<StructNode>(id.str_value, std::move(block));
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
        astptr node = parse_expr();
        consume(SEMI);
        return node;
    }
    return parse_expr();
}

std::vector<std::unique_ptr<ASTNode>> parser::parse()
{
    table.emplace_back();
    insert("print", FUNC, nothing{}, true);
    insert("input", FUNC, nothing{}, true);
    insert("sizeof", FUNC, nothing{}, true);
    insert("free", FUNC, nothing{}, true);
    std::vector<std::unique_ptr<ASTNode>> parsed;
    while (peek().type != token_type::EOF_)
    {
        try
        {
            parsed.push_back(parse_statement());
        }
        catch (ParseTimeError &e)
        {
            std::cerr << "\x1b[0;35m" << filename << ':' << line << ':' << column << ": \x1b[31m error\n"
                      << e.what() << "\x1b[0m";
            sync();
            errors = true;
        }
    }
    table.pop_back();
    return parsed;
}
