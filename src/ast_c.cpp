#include "../include/ast.h"
#include "../include/common.h"
#include "../include/generator.h"
#include "../include/table.h"
#include <map>
#include <memory>
#include <sstream>


std::string Node::gen_(generator &g)
{
    (void)g;
    if(tok.type==ID) {
        symbol var = search(tok.str_value);
        if(var.comptime&&(var.type<=LONG_TYPE||var.type==BOOL_TYPE)) return std::to_string(variant2int<long long>(var.value));
        if(var.comptime&&(var.type<=UNSIGNED_64_TYPE&&var.type>LONG_TYPE)) return std::to_string(variant2int<unsigned long long>(var.value));
        if(var.comptime&&var.type==STRING_TYPE) return variant2string(var.value);
        if(var.comptime&&(var.type==FLOAT_TYPE)) return std::to_string(variant2float(var.value));
        if(var.comptime&&(var.type==DOUBLE_TYPE)) return std::to_string(variant2double(var.value));
        if(isptr||is_ref_arg) return "*" + tok.str_value;
        else return tok.str_value;
    }
    return variant2value(tok);
}

std::string BinaryNode::gen_(generator &g)
{
    bool float_ = false;
    bool string_to_int_op = false;
    if (left->kind == ast_type::JUSTNODE)
    {
        auto n = static_cast<Node *>(left.get());
        auto n_ = static_cast<Node *>(right.get());
        if (is_it_float(n->tok.type))
        {
            float_ = true;
            g.line = n->tok.line;
            g.column = n->tok.column;
        }
        if (is_it_float(n_->tok.type))
        {
            float_ = true;
            g.line = n->tok.line;
            g.column = n->tok.column;
        }
        if (n->tok.type == STRING && (is_it_int(n_->tok.type)))
        {
            string_to_int_op = true;
            g.line = n->tok.line;
            g.column = n->tok.column;
        }
        if (n_->tok.type == STRING && is_it_int(n->tok.type))
        {
            string_to_int_op = true;
            g.line = n->tok.line;
            g.column = n->tok.column;
        }
    }
    std::string r = g.gencode(right);
    if(r=="0") {
        g.line = op.line;
        g.column = op.column;
        throw TranspileTimeError("\tDivision by zero\n");
    }
    std::string res = "(" + g.gencode(left) + op2string(op.type) + g.gencode(right) + ")";
    if (float_ && op.type == MOD)
    {
        std::string err = "Cannot do modulo for floats, use fmod from stdlib instead:\n\t";
        err += res + '\n';
        g.line = op.line;
        g.column = op.column;
        throw TranspileTimeError(err.c_str());
    }
    if (float_ && (op.type == SHIFT_R||op.type==SHIFT_L))
    {
        std::string err = "Cannot shift for floats, use fshiftl/fshiftr from stdlib instead:\n\t";
        err += res + '\n';
        g.line = op.line;
        g.column = op.column;
        throw TranspileTimeError(err.c_str());
    }
    if (float_ && (op.type == OR_B||op.type==AND_B||op.type==XOR))
    {
        std::string err = "Cannot do xor, bitwise or/and for floats, sorry:\n\t";
        err += res + '\n';
        g.line = op.line;
        g.column = op.column;
        throw TranspileTimeError(err.c_str());
    }
    else if (string_to_int_op)
    {
        std::string err = "Cannot do math with string and integer:\n\t";
        err += res + '\n';
        throw TranspileTimeError(err.c_str());
    }
    return res;
}

std::string AssignmentNode::gen_(generator &g)
{
    std::string value = val ? g.gencode(val) : "";
    if(ismoving) {
        if(isptr) return id + " = std::move(" + value.substr(1) + ")";
    }
    if(isptr) return "*" + id + (value.empty() ? "" : " = " + value);
    return id + (value.empty() ? "" : "=" + value);
}

std::string AssignmentNodeExpr::gen_(generator &g)
{
    std::string type;
    std::string const_="";
    if(is_const) const_ = "const ";
    std::string nullval = "0";
    if (type_ == BYTE_TYPE)
        type += "int8_t";
    if (type_ == WORD_TYPE)
        type += "int16_t";
    if (type_ == INT_TYPE)
        type += "int32_t";
    if (type_ == LONG_TYPE)
        type += "int64_t";
    if (type_ == FLOAT_TYPE)
        type += "float";
    if (type_ == DOUBLE_TYPE)
        type += "double";
    if (type_ == STRING_TYPE)
    {
        type += "string";
        nullval = "";
    }
    if (type_ == BOOL_TYPE)
        type += "bool";
    if (type_ == UNSIGNED_8_TYPE)
        type += "uint8_t";
    if (type_ == UNSIGNED_16_TYPE)
        type += "uint16_t";
    if (type_ == UNSIGNED_32_TYPE)
        type += "uint32_t";
    if (type_ == UNSIGNED_64_TYPE)
        type += "uint64_t";
    type += struct_id;
    if(is_ptr) {
        if(val) {
            if(is_mov) {
                std::ostringstream s;
                s << type << "* " << id << " = " << "oxygen_move((void**)&" << g.gencode(val).substr(1) << ")";
                return s.str();
            }
            else return type + "* " + id + " = malloc(sizeof(" + type + "));\n" + g.pad() + "*" + id + " = " + g.gencode(val);
        }
        else return type +  "* " + id;
    }
    if(struct_id!="") return type + " " + id + " = " + type + "_flame_def_init()";
    if(type_==STRING_TYPE) {
        std::string a = const_ + type + ' ' + id + ";\n";
        a += "    oxygen_new_string( &" + id + ", " + (val ?  g.gencode(val) : "") + ")";
        return a;
    }
    return const_ + type + ' ' + id + (val ? "=" + g.gencode(val) : "=" + nullval);
}

std::string UnaryNode::gen_(generator &g)
{
    if (sign == MINUS)
        return "-" + g.gencode(left);
    if (sign == NOT)
        return "!" + g.gencode(left);
    return g.gencode(left);
}

std::string FuncCallNode::gen_(generator &g)
{
    std::string args_;
    if (id == "print")
    {
        g.line = line;
        g.column = column;
        throw TranspileTimeError("\tprint() currently not supported in C generator");
        args_ = "print(";
        for (auto &x : args)
        {
            args_ += g.gencode(args.at(0));
            args_ += "";
        }
        args_.pop_back();
        args_.pop_back();
        args_.pop_back();
        args_.pop_back();
        return args_;
    }
    if (id == "input")
    {
        g.line = line;
        g.column = column;
        throw TranspileTimeError("\tinput() currently not supported in C generator");
        args_ = "std::cin ";
        args_ += " >> ";
        args_ += g.gencode(args.at(0));
        return args_;
    }

    if (id == "sizeof")
    {
        args_ = "sizeof(";
        args_ += g.gencode(args.at(0)) + ')';
        return args_;
    }
    for (u64 i = 0; i < args.size(); i++)
    {
        if(args[i]->kind==ast_type::JUSTNODE) {
            Node* n = dynamic_cast<Node*>(args[i].get());
            if(n->is_ref_arg) {
                args_ += "REF(" + n->tok.str_value + ")";
            }
            else args_ += g.gencode(args[i]);
        }
        else args_ += g.gencode(args[i]);
        if (i + 1 < args.size())
            args_ += ", ";
    }
    return id + '(' + args_ + ')';
}

std::string CondNode::gen_(generator &g)
{
    std::string op_;
    if (op == LESS)
        op_ = " < ";
    if (op == BIGGER)
        op_ = " > ";
    if (op == LEQUAL)
        op_ = " <= ";
    if (op == BEQUAL)
        op_ = " >= ";
    if (op == EQUAL)
        op_ = " == ";
    if (op == NEQUAL)
        op_ = " != ";
    if (op == OR)
        op_ = " || ";
    if (op == AND)
        op_ = " && ";
    return '(' + g.gencode(left) + op_ + g.gencode(right) + ')';
}

std::string BlockNode::gen_(generator &g)
{
    std::ostringstream code;
    code << " {\n";
    g.indent++;
    for (auto &x : stmts)
    {
        code << g.pad();
        code << g.gencode(x);
        code << ";\n";
    }
    g.indent--;
    code << g.pad() + "}";
    return code.str();
}

std::string FuncNode::gen_(generator &g)
{
    std::ostringstream code;
    if(is_return_type_array) {
        code << "std::array<" << type_in_c(type) << ',' << std::to_string(size) << ">";
    }
    else code << type_in_c(type);
    code << id;
    code << '(';
    for (u64 i = 0; i < args.size(); i++)
    {
        code << g.gencode(args[i]);
        if (i + 1 < args.size())
            code << ", ";
    }
    code << ") ";
    code << g.gencode(block);
    code << '\n';
    return code.str();
}

std::string ArgumentNode::gen_(generator &g)
{
    (void)g;
    std::string type_;
    if(isconst&&!ref) type_ += "const ";
    type_ += type_in_c(type);
    if(ismut) return type_ + "* " + id;
    if(ref) return "const " + type_ + "*const " + id;
    if(is_array) 
        return type_in_c(type)+'['+std::to_string(size_if_array)+"] "+id;
    return type_ + id;
}

std::string ReturnNode::gen_(generator &g)
{
    if(!value) return "return";
    return "return " + g.gencode(value);
}

std::string IfNode::gen_(generator &g)
{
    std::string code;
    if (cond)
    {
        code += (type == IF ? "if(" : "else if(") + g.gencode(cond) + ")" + g.gencode(block);
    }
    else
        code += "else " + g.gencode(block);
    if (next)
        code += " " + g.gencode(next);
    return code;
}

std::string LoopNode::gen_(generator &g)
{
    std::string code;
    code += "while(";
    code += g.gencode(cond);
    code += ") ";
    code += g.gencode(block);
    return code;
}

std::string BreakNode::gen_(generator &g)
{
    (void)g;
    return "break";
}

std::string ContinueNode::gen_(generator &g)
{
    (void)g;
    return "continue";
}

std::string IncDecVarNode::gen_(generator &g)
{
    (void)g;
    if (type == 0)
    {
        return id + "++";
    }
    else
        return id + "--";
}

std::string ForNode::gen_(generator &g)
{
    std::string code = " for(";
    code += g.gencode(var) + ';';
    code += g.gencode(cond) + ';';
    code += g.gencode(thing);
    code += ')';
    code += g.gencode(block);
    return code;
}

std::string ReAssignmentNodeExpr::gen_(generator &g)
{
    std::string op;
    switch (type_)
    {
    case PLUS:
    {
        op = " += ";
        break;
    }
    case MINUS:
    {
        op = " -= ";
        break;
    }
    case STAR:
    {
        op = " *= ";
        break;
    }
    case SLASH:
    {
        op = " /= ";
        break;
    }
    case MOD:
    {
        op = " %= ";
        break;
    }
    case XOR:
    {
        op = " ^= ";
        break;
    }
    case OR_B:
    {
        op = " |= ";
        break;
    }
    case AND_B:
    {
        op = " &= ";
        break;
    }
    default:
        break;
    }
    return id + op + g.gencode(val);
}


/*

ARRAY NODE LATER

*/

std::string ArrayNode::gen_(generator &g)
{
    if(is_vector) {
        std::string type_ = type_in_c(type);
        if(is_init) {
            return "std::vector<"+type_+">"+id + '=' + g.gencode(values.at(0));
        }
        std::string values_ = "{";
        if (!values.empty())
        {
        for (u64 i = 0; i < values.size(); i++)
        {
            values_ += g.gencode(values[i]);
            if (i + 1 < values.size())
                values_ += ", ";
            }
            values_ += "}";
        }
        else
        {
            values_ = "";
        }
        if(!values.empty()) {
            return "std::vector<"+type_+">"+id + '=' + values_;
        }
        return "std::vector<"+type_+">"+id;
    }
    std::string type_ = type_in_c(type);
    if(is_init) {
        return type_ + id + '[' + std::to_string(size)+">" + "] =" + g.gencode(values.at(0));
    }
    std::string values_ = "{";
    if (!values.empty())
    {
        for (u64 i = 0; i < values.size(); i++)
        {
            values_ += g.gencode(values[i]);
            if (i + 1 < values.size())
                values_ += ", ";
        }
        values_ += "}";
    }
    else
    {
        return type_ + id + '[' + std::to_string(size)+">" + "]";
    }
    return type_ + id + '[' + std::to_string(size) + "] = " + values_;
    //return "std::array<"+type_+','+std::to_string(size)+">"+id;

    /*
    if (!values.empty())
        return type_ + id + "[" + std::to_string(size) + "] = " + values_;
    return type_ + id + "[" + std::to_string(size) + "]";
    if (!values.empty())
        return type_ + id + "[] = " + values_;
    return type_ + id + "[]";
    */
}

std::string ArrayAccessNode::gen_(generator &g)
{
    if(isptr) return "(*" + id.str_value + ")[" + g.gencode(index) + "]";
    if(is_vector) return id.str_value + ".at(" + g.gencode(index) + ")";
    return id.str_value + "[" + g.gencode(index) + "]";
}

std::string ArrayChangeNode::gen_(generator &g)
{
    return id.str_value + "[" + g.gencode(index) + "] = " + g.gencode(value);
}

std::string ModuleNode::gen_(generator &g)
{
    std::ostringstream code;
    generator gen;
    gen.is_mod = true;
    gen.c_gen = g.c_gen;
    code << gen.generate(module);
    g.header += code.str();
    return "";
}

std::string MethodNode::gen_(generator &g) {
    std::string code = parent;
    unsigned long i = 0;
    std::string accessor = isptr ? "->" : ".";
    for(auto &x : children) {
        if(type==VEC) {
            if(x->kind==ast_type::FuncCall) {
                auto n = static_cast<FuncCallNode*>(x.get());
                if(n->id=="push") {
                    code += '.';
                    std::string args_;
                    args_ += g.gencode(n->args[0]);
                    code += "emplace_back(" + args_ + ')';
                    return code;
                } else if(n->id=="pop") {
                    code += '.';
                    code += "pop_back()";
                    return code;
                }
            }
        }
        code += accessor + g.gencode(x);
        accessor = (i < isptrs.size() && isptrs[i]) ? "->" : ".";
        i++;
    }
    return code;
}

std::string ModuleCallNode::gen_(generator &g) {
    std::string code = "";
    unsigned long i = 0;
    std::string accessor = "";
    for(auto &x : children) {
        code += accessor + g.gencode(x);
        accessor = (i < isptrs.size() && isptrs[i]) ? "->" : ".";
        i++;
    }
    return code;
}

std::string StructNode::gen_(generator &g) {
    std::ostringstream code;
    std::ostringstream funcs;
    std::ostringstream default_init;
    default_init << id << ' ' << id << "_flame_def_init() {\n";
    default_init << "    " << id << " Temp;\n";
    code << "typedef struct " << id;
    code << " {\n";
    g.indent++;
    for (auto &x : block)
    {
        code << g.pad();
        if(x->kind==ast_type::DEFINEVAR) {
            AssignmentNodeExpr* n = dynamic_cast<AssignmentNodeExpr*>(x.get());
            code << type_in_c(token{.type=n->type_}) << n->id << ";\n";
            default_init  << "    Temp." << n->id << " = " << g.gencode(n->val) << ";\n";
        } else {
            code << g.gencode(x);
            code << ";\n";
        }
    }
    g.indent--;
    code << g.pad() + "} " << id << ';';
    default_init << "    return Temp;\n}";
    code << '\n' << default_init.str();
    return code.str();
}