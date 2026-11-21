// AST -> C99/C11 (GNU extensions: __typeof__, __auto_type) code generator.
// Generated programs must include "flame_rt.h" (see flame_rt.h).
#include "../include/ast.h"
#include "../include/common.h"
#include "../include/generator.h"
#include "../include/lexer.h"
#include "../include/table.h"
#include <algorithm>
#include <cctype>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace
{
    std::string g_prefix;                              // current module prefix, e.g. "math__"
    std::set<std::string> g_structs;                   // qualified struct names seen so far
    std::map<std::string, std::vector<char>> g_sigs;   // C func name -> per-arg mode (0 value, 1 mut ptr, 2 ref const ptr)
    std::set<std::string> g_byref;                     // params of the function being generated (deref on use)

    std::string replace_all(std::string s, const std::string &from, const std::string &to)
    {
        for (size_t p = 0; (p = s.find(from, p)) != std::string::npos; p += to.size())
            s.replace(p, from.size(), to);
        return s;
    }
    std::string mangle(const std::string &s) { return replace_all(s, "::", "__"); }

    std::string rtrim(std::string s)
    {
        while (!s.empty() && s.back() == ' ') s.pop_back();
        return s;
    }

    std::string strip_deref(const std::string &s) { return (!s.empty() && s[0] == '*') ? s.substr(1) : s; }

    std::string var_name(const std::string &id) { return g_byref.count(id) ? "(*" + id + ")" : id; }

    std::string qualify_struct(const std::string &id)
    {
        if (!g_prefix.empty() && g_structs.count(g_prefix + id)) return g_prefix + id;
        return id;
    }

    bool is_ident_text(const std::string &s)
    {
        if (s.empty()) return false;
        for (unsigned char c : s)
            if (!std::isalnum(c) && c != '_') return false;
        return !std::isdigit((unsigned char)s[0]);
    }
    // "x" or "(*x)" -> we can take its address directly
    bool is_lvalue_text(const std::string &s)
    {
        if (is_ident_text(s)) return true;
        return s.size() > 3 && s.compare(0, 2, "(*") == 0 && s.back() == ')' && is_ident_text(s.substr(2, s.size() - 3));
    }

    // Flame type -> C type name (no trailing space). Empty = unknown.
    template <class T> std::string c_type_name(T t)
    {
        if (t == BYTE_TYPE) return "int8_t";
        if (t == WORD_TYPE) return "int16_t";
        if (t == INT_TYPE) return "int32_t";
        if (t == LONG_TYPE) return "int64_t";
        if (t == FLOAT_TYPE) return "float";
        if (t == DOUBLE_TYPE) return "double";
        if (t == STRING_TYPE) return "flame_str";
        if (t == BOOL_TYPE) return "bool";
        if (t == UNSIGNED_8_TYPE) return "uint8_t";
        if (t == UNSIGNED_16_TYPE) return "uint16_t";
        if (t == UNSIGNED_32_TYPE) return "uint32_t";
        if (t == UNSIGNED_64_TYPE) return "uint64_t";
        if (t == AUTO_TYPE) return "__auto_type";
        return "";
    }
    // Same contract as type_in_cpp (trailing space). Falls back to it for types
    // not listed above (void, struct types...) -- port those inside type_in_cpp if they print C++-only names.
    template <class T> std::string type_in_c(T t)
    {
        std::string s = c_type_name(t);
        return s.empty() ? type_in_cpp(t) : s + " ";
    }

    template <class P> bool is_string_expr(const P &p)
    {
        if (!p || p->kind != ast_type::JUSTNODE) return false;
        auto n = static_cast<Node *>(p.get());
        if (n->tok.type == STRING) return true;
        if (n->tok.type == ID)
        {
            symbol s = search(n->tok.str_value);
            return s.type == STRING_TYPE;
        }
        return false;
    }
} // namespace

std::string Node::gen_c(generator &g)
{
    (void)g;
    if (tok.type == ID)
    {
        symbol var = search(tok.str_value);
        if (var.comptime && (var.type <= LONG_TYPE || var.type == BOOL_TYPE)) return std::to_string(variant2int<long long>(var.value));
        if (var.comptime && (var.type <= UNSIGNED_64_TYPE && var.type > LONG_TYPE)) return std::to_string(variant2int<unsigned long long>(var.value));
        if (var.comptime && var.type == STRING_TYPE) return variant2string(var.value);
        if (var.comptime && (var.type == FLOAT_TYPE)) return std::to_string(variant2float(var.value));
        if (var.comptime && (var.type == DOUBLE_TYPE)) return std::to_string(variant2double(var.value));
        std::string name = mangle(tok.str_value);
        if (!var.module_name.empty() && var.module_name != g_prefix) {
            name = mangle(var.module_name) + "__" + name;
        }
        if (isptr) return "*" + name;
        return var_name(name);
    }
    return variant2value(tok);
}

std::string BinaryNode::gen_c(generator &g)
{
    bool float_ = false;
    bool string_to_int_op = false;
    if (left->kind == ast_type::JUSTNODE && right->kind == ast_type::JUSTNODE)
    {
        auto n = static_cast<Node *>(left.get());
        auto n_ = static_cast<Node *>(right.get());
        if (is_it_float(n->tok.type) || is_it_float(n_->tok.type))
        {
            float_ = true;
            g.line = n->tok.line;
            g.column = n->tok.column;
        }
        if ((n->tok.type == STRING && is_it_int(n_->tok.type)) || (n_->tok.type == STRING && is_it_int(n->tok.type)))
        {
            string_to_int_op = true;
            g.line = n->tok.line;
            g.column = n->tok.column;
        }
    }
    std::string l = g.gencode(left);
    std::string r = g.gencode(right);
    if (r == "0" && (op.type == SLASH || op.type == MOD))
    {
        g.line = op.line;
        g.column = op.column;
        throw TranspileTimeError("\tDivision by zero\n");
    }
    bool ls = is_string_expr(left), rs = is_string_expr(right);
    if (ls && rs && op.type == PLUS) return "flame_strcat(" + l + ", " + r + ")"; // allocates, see flame_rt.h

    std::string res = "(" + l + op2string(op.type) + r + ")";
    if (float_ && op.type == MOD)
    {
        std::string err = "Cannot do modulo for floats, use fmod from stdlib instead:\n\t";
        err += res + '\n';
        g.line = op.line;
        g.column = op.column;
        throw TranspileTimeError(err.c_str());
    }
    if (float_ && (op.type == SHIFT_R || op.type == SHIFT_L))
    {
        std::string err = "Cannot shift for floats, use fshiftl/fshiftr from stdlib instead:\n\t";
        err += res + '\n';
        g.line = op.line;
        g.column = op.column;
        throw TranspileTimeError(err.c_str());
    }
    if (float_ && (op.type == OR_B || op.type == AND_B || op.type == XOR))
    {
        std::string err = "Cannot do xor, bitwise or/and for floats, sorry:\n\t";
        err += res + '\n';
        g.line = op.line;
        g.column = op.column;
        throw TranspileTimeError(err.c_str());
    }
    if (string_to_int_op || ls || rs)
    {
        std::string err = "Cannot do math with strings (only string + string is supported):\n\t";
        err += res + '\n';
        g.line = op.line;
        g.column = op.column;
        throw TranspileTimeError(err.c_str());
    }
    return res;
}

std::string AssignmentNode::gen_c(generator &g)
{
    std::string value = val ? g.gencode(val) : "";
    if (ismoving && isptr)
    {
        // C has no std::move: copy the pointer, null the source
        std::string src = strip_deref(value);
        return "(" + id + " = " + src + ", " + src + " = NULL)";
    }
    std::string name = g.in_function ? id : g_prefix + id;
    if (!g.in_function)
    {
        symbol var = search(id);
        if (!var.module_name.empty()) name = mangle(var.module_name) + "__" + id;
    }
    if (isptr) return "*" + name + (value.empty() ? "" : " = " + value);
    return var_name(name) + (value.empty() ? "" : "=" + value);
}

std::string AssignmentNodeExpr::gen_c(generator &g)
{
    std::string type = c_type_name(type_);
    std::string const_ = is_const ? "const " : "";
    std::string nullval = (type_ == STRING_TYPE) ? "\"\"" : "0";
    std::string sid = qualify_struct(struct_id);
    bool known_struct = type.empty() && g_structs.count(sid);

    if (is_ptr)
    {
        std::string base = type.empty() ? sid : type;
        std::string decl = "smart " + const_ + base + "* " + id + " = ";
        if (val)
        {
            if (is_mov)
            {
                std::string src = strip_deref(g.gencode(val));
                return decl + src + "; " + src + " = NULL";
            }
            return decl + "(" + base + " *)malloc(sizeof(" + base + ")); *" + id + " = " + g.gencode(val);
        }
        std::string s = decl + "(" + base + " *)calloc(1, sizeof(" + base + "))";
        if (known_struct) s += "; *" + id + " = " + sid + "__default()";
        return s;
    }
    std::string name = g.in_function ? id : g_prefix + id;
    if (!struct_id.empty() && type.empty())
        return const_ + sid + " " + name + " = " + (known_struct ? sid + "__default()" : "{0}");
    return const_ + type + ' ' + name + "=" + (val ? g.gencode(val) : nullval);
}

std::string UnaryNode::gen_c(generator &g)
{
    if (sign == MINUS) return "-(" + g.gencode(left) + ")";
    if (sign == NOT) return "!(" + g.gencode(left) + ")";
    return g.gencode(left);
}

std::string FuncCallNode::gen_c(generator &g)
{
    if (id == "print")
    {
        if (args.empty()) return "(void)0";
        std::string out;
        for (u64 i = 0; i < args.size(); i++)
        {
            out += "flame_print(" + g.gencode(args[i]) + ")";
            if (i + 1 < args.size()) out += ", ";
        }
        return args.size() > 1 ? "(" + out + ")" : out; // comma expression
    }
    if (id == "input") return "flame_input(&" + g.gencode(args.at(0)) + ")";
    if (id == "sizeof") return "sizeof(" + g.gencode(args.at(0)) + ')';

    std::string fid = mangle(id);
    std::string cname = g_sigs.count(g_prefix + fid) ? g_prefix + fid : fid;
    auto sig = g_sigs.find(cname);
    std::string args_;
    for (u64 i = 0; i < args.size(); i++)
    {
        std::string a = g.gencode(args[i]);
        char mode = (sig != g_sigs.end() && i < sig->second.size()) ? sig->second[i] : 0;
        if (mode == 1) a = "&" + a;
        else if (mode == 2) a = is_lvalue_text(a) ? "&" + a : "&(__typeof__(" + a + ")){" + a + "}";
        args_ += a;
        if (i + 1 < args.size()) args_ += ", ";
    }
    return cname + '(' + args_ + ')';
}

std::string CondNode::gen_c(generator &g)
{
    std::string op_;
    if (op == LESS) op_ = " < ";
    if (op == BIGGER) op_ = " > ";
    if (op == LEQUAL) op_ = " <= ";
    if (op == BEQUAL) op_ = " >= ";
    if (op == EQUAL) op_ = " == ";
    if (op == NEQUAL) op_ = " != ";
    if (op == OR) op_ = " || ";
    if (op == AND) op_ = " && ";
    std::string l = g.gencode(left), r = g.gencode(right);
    bool is_cmp = (op != OR && op != AND);
    if (is_cmp && (is_string_expr(left) || is_string_expr(right)))
        return "(strcmp(" + l + ", " + r + ")" + op_ + "0)"; // == on char* compares addresses in C
    return '(' + l + op_ + r + ')';
}

std::string BlockNode::gen_c(generator &g)
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

std::string FuncNode::gen_c(generator &g)
{
    if (is_return_type_array)
        throw TranspileTimeError("\tC backend cannot return arrays by value; take an array argument or return a pointer\n");

    std::string cname = g_prefix + id;
    std::set<std::string> saved = g_byref;
    g_byref.clear();

    std::vector<char> sig;
    for (auto &a : args)
    {
        auto arg = static_cast<ArgumentNode *>(a.get());
        char mode = 0;
        if (!arg->is_array) mode = arg->ref ? 2 : (arg->ismut ? 1 : 0);
        sig.push_back(mode);
        if (mode) g_byref.insert(arg->id);
    }
    g_sigs[cname] = sig; // registered before the body so recursion works

    std::ostringstream code;
    code << type_in_c(type) << cname << '(';
    if (args.empty()) code << "void";
    for (u64 i = 0; i < args.size(); i++)
    {
        code << g.gencode(args[i]);
        if (i + 1 < args.size()) code << ", ";
    }
    code << ") ";
    std::string saved_prefix = g_prefix;
    std::string saved_in_function = g.in_function ? "1" : "0";
    g.in_function = true;
    code << g.gencode(block);
    g.in_function = (saved_in_function == "1");
    g_prefix = saved_prefix;
    code << '\n';
    g_byref = saved;
    return code.str();
}

std::string ArgumentNode::gen_c(generator &g)
{
    (void)g;
    std::string t = rtrim(type_in_c(type));
    if (is_array) return (isconst ? "const " : "") + t + " " + id + "[" + std::to_string(size_if_array) + "]"; // decays to pointer
    if (ref) return "smart const " + t + "* " + id;
    if (ismut) return "smart " + t + "* " + id;
    return (isconst ? "const " : "") + t + " " + id;
}

std::string ReturnNode::gen_c(generator &g)
{
    if (!value) return "return";
    return "return " + g.gencode(value);
}

std::string IfNode::gen_c(generator &g)
{
    std::string code;
    if (cond)
        code += std::string(type == IF ? "if" : "else if") + " (" + g.gencode(cond) + ")" + g.gencode(block);
    else
        code += "else " + g.gencode(block);
    if (next) code += " " + g.gencode(next);
    return code;
}

std::string LoopNode::gen_c(generator &g)
{
    return "while(" + g.gencode(cond) + ") " + g.gencode(block);
}

std::string BreakNode::gen_c(generator &g)
{
    (void)g;
    return "break";
}

std::string ContinueNode::gen_c(generator &g)
{
    (void)g;
    return "continue";
}

std::string IncDecVarNode::gen_c(generator &g)
{
    (void)g;
    return var_name(id) + (type == 0 ? "++" : "--");
}

std::string ForNode::gen_c(generator &g)
{
    std::string code = " for(";
    code += g.gencode(var) + ';';
    code += g.gencode(cond) + ';';
    code += g.gencode(thing);
    code += ')';
    code += g.gencode(block);
    return code;
}

std::string ReAssignmentNodeExpr::gen_c(generator &g)
{
    std::string op;
    switch (type_)
    {
    case PLUS:  op = " += "; break;
    case MINUS: op = " -= "; break;
    case STAR:  op = " *= "; break;
    case SLASH: op = " /= "; break;
    case MOD:   op = " %= "; break;
    case XOR:   op = " ^= "; break;
    case OR_B:  op = " |= "; break;
    case AND_B: op = " &= "; break;
    default: break;
    }
    return (g.in_function ? var_name(id) : var_name(g_prefix + id)) + op + g.gencode(val);
}

std::string ArrayNode::gen_c(generator &g)
{
    std::string elem = rtrim(type_in_c(type));
    std::string vals;
    for (u64 i = 0; i < values.size(); i++)
    {
        vals += g.gencode(values[i]);
        if (i + 1 < values.size()) vals += ", ";
    }
    if (is_vector)
    {
        // vectors are `T *` with a hidden header (see flame_rt.h)
        if (is_init) return "smart " + elem + "* " + id + " = " + g.gencode(values.at(0));
        if (values.empty()) return "smart " + elem + "* " + id + " = NULL";
        return "smart " + elem + "* " + id + " = flame_vec_from(" + elem + ", " + std::to_string(values.size()) + ", " + vals + ")";
    }
    std::string decl = elem + " " + id + "[" + std::to_string(size) + "]";
    if (is_init) return decl + "; memcpy(" + id + ", " + g.gencode(values.at(0)) + ", sizeof(" + id + "))";
    if (values.empty()) return decl + " = {0}";
    return decl + " = {" + vals + "}";
}

std::string ArrayAccessNode::gen_c(generator &g)
{
    std::string idx = g.gencode(index);
    std::string name = isptr ? "(*" + id.str_value + ")" : var_name(id.str_value);
    if (is_vector) return "flame_vec_at(" + name + ", " + idx + ")"; // bounds-checked, like .at()
    return name + "[" + idx + "]";
}

std::string ArrayChangeNode::gen_c(generator &g)
{
    // works for both fixed arrays and vectors (vector is a plain T*), no bounds check
    return var_name(id.str_value) + "[" + g.gencode(index) + "] = " + g.gencode(value);
}

std::string ModuleNode::gen_c(generator &g)
{
    // C has no namespaces: members get a "<module>__" prefix
    generator gen;
    generator *root = g.root ? g.root : &g;
    gen.is_mod = true;
    gen.c_gen = true;
    gen.root = root;
    std::string saved = g_prefix;
    g_prefix = mname + "__";
    gen.generate(module);
    g_prefix = saved;
    root->header += gen.header;
    root->header += gen.gen_code.str();
    return "";
}

std::string MethodNode::gen_c(generator &g)
{
    std::string code = isptr ? parent : var_name(parent);
    unsigned long i = 0;
    std::string accessor = isptr ? "->" : ".";
    for (auto &x : children)
    {
        if (type == VEC && x->kind == ast_type::FuncCall)
        {
            auto n = static_cast<FuncCallNode *>(x.get());
            if (n->id == "push") return "flame_vec_push(" + code + ", " + g.gencode(n->args[0]) + ')';
            if (n->id == "pop") return "flame_vec_pop(" + code + ')';
        }
        code += accessor + g.gencode(x);
        accessor = (i < isptrs.size() && isptrs[i]) ? "->" : ".";
        i++;
    }
    return code;
}

std::string ModuleCallNode::gen_c(generator &g)
{
    std::string code = "";
    unsigned long i = 0;
    std::string accessor = "";
    for (auto &x : children)
    {
        code += accessor + g.gencode(x);
        accessor = (i < isptrs.size() && isptrs[i]) ? "->" : ".";
        i++;
    }
    return mangle(code); // "mod::f" -> "mod__f"
}

std::string StructNode::gen_c(generator &g)
{
    std::string cname = g_prefix + id;
    g_structs.insert(cname);

    std::ostringstream code;
    std::string defaults;
    code << "typedef struct " << cname << " " << cname << ";\n";
    code << "struct " << cname << " {\n";
    g.indent++;
    for (auto &x : block)
    {
        std::string decl, def, fname;
        if (auto m = dynamic_cast<AssignmentNodeExpr *>(x.get()))
        {
            // C forbids member initializers: emit a bare declaration, keep the default for <S>__default()
            std::string base = c_type_name(m->type_);
            std::string sid = qualify_struct(m->struct_id);
            bool is_struct_t = base.empty();
            if (is_struct_t) base = sid;
            decl = std::string(m->is_ptr ? "smart " : "") + (m->is_const ? "const " : "") + base + (m->is_ptr ? "* " : " ") + m->id;
            fname = m->id;
            if (!m->is_ptr)
            {
                if (m->val) def = g.gencode(m->val);
                else if (m->type_ == STRING_TYPE) def = "\"\"";
                else if (is_struct_t && g_structs.count(sid)) def = sid + "__default()";
            }
        }
        else if (auto a = dynamic_cast<ArrayNode *>(x.get()))
        {
            std::string elem = rtrim(type_in_c(a->type));
            fname = a->id;
            decl = a->is_vector ? "smart " + elem + "* " + a->id : elem + " " + a->id + "[" + std::to_string(a->size) + "]";
            if (!a->values.empty() && !a->is_init)
            {
                std::string vals;
                for (u64 i = 0; i < a->values.size(); i++)
                {
                    vals += g.gencode(a->values[i]);
                    if (i + 1 < a->values.size()) vals += ", ";
                }
                def = a->is_vector ? "flame_vec_from(" + elem + ", " + std::to_string(a->values.size()) + ", " + vals + ")"
                                   : "{" + vals + "}";
            }
        }
        else
        {
            if (x->kind == ast_type::FUNC)
                continue;
            decl = g.gencode(x);
        }

        code << g.pad() << decl << ";\n";
        if (!def.empty()) defaults += (defaults.empty() ? "" : ", ") + std::string(".") + fname + " = " + def;
    }
    g.indent--;
    code << g.pad() + "}";
    // default-constructed value (replaces C++ default member initializers)
    code << ";\nstatic inline " << cname << " " << cname << "__default(void) { return (" << cname << "){"
         << (defaults.empty() ? "0" : defaults) << "}; }";
    return code.str();
}