#pragma once
#include <vector>
#include <unordered_map>
#include <string>
#include "common.h"
#include "lexer.h"

using symbol = struct symbol {
    token_type type=EOF_;
    token_value value;
    bool is_const=false;
    u64 size=0;
    bool is_array=false;
    bool comptime=false;
    std::string name="";
    bool is_vector=false;
    bool is_ptr=false;
    std::string module_name = "";
    bool is_moved=false;
    bool is_mut_now=true;
    bool is_ref_arg=false;
    std::string struct_ = "";
};

using fsymbol = struct fsymbol {
    token_type type=EOF_;
    std::vector<symbol> args;
    bool comptime=false;
    std::string struct_="";
    std::string module_name = "";
};

extern std::vector<std::string> struct_list;
extern std::vector<std::string> freed_list;
extern std::vector<std::string> notfreed_list;
extern std::vector<std::unordered_map<std::string, symbol>> table;
extern std::unordered_map<std::string, fsymbol> ftable;
extern std::vector<std::string> loadedModules;

#define CURRENT_SCOPE std::ranges::reverse_view(table)[0]
#define LAST_SCOPE_N (table.size()-1)

token_value search_value(const std::string &name);

/**
 * Searches variable or function name in scope and returns {symbol} if its found else empty object
 */
symbol search(const std::string &name);
symbol* searchptr(const std::string &name);

token_type search_type(const std::string &name);


/**
 * @brief Searching in scope
 * 
 * @param name name of object
 * @param lvl scope level
 * @return token_type returns type of object, EOF_ if not found
 */
token_type search_type_scope(const std::string &name, unsigned int lvl);


void insert(const std::string &name,token_type type, token_value val, bool is_const=false, u64 size=1, bool is_array=false, bool comptime=false, bool is_vector=false, bool isptr=false, const std::string &modname="", bool ismov=false, bool isref=false, const std::string& struct_="");
void insert_top(const std::string &name,token_type type, token_value val,bool is_const=false, u64 size=1, bool is_array=false, bool comptime=false, bool is_vector=false, bool isptr=false);

bool exist(const std::string &name);

/**
 * @brief Check if exist in scope
 * 
 * @param name name of object
 * @param lvl scope level
 * @return bool returns true if exist, else false
 */
bool exist_in_scope(const std::string &name, unsigned int lvl);

inline fsymbol* fsearch(const std::string &name) {
    auto it = ftable.find(name);
    if (it != ftable.end()) {
        return &it->second;
    }
    return nullptr;
}

inline bool fexist(const std::string &name) {
    if(fsearch(name)->type==EOF_) return false;
    return true;
}

inline void finsert(const std::string &name, const token_type &type, const std::vector<symbol> &args, const std::string &stru="") {
    ftable.insert_or_assign(name, fsymbol{type, args, false, stru});
}


inline fsymbol* fsearch_module(const std::string &name,const std::string &modname) {
    auto it = ftable.find(name);
    if (it != ftable.end()) {
        if(it->second.module_name.c_str()!=modname) return nullptr;
        return &it->second;
    }
    return nullptr;
}

inline bool fexist_module(const std::string &name, const std::string &modname) {
    fsymbol* f = fsearch(name);
    if(!f) return false;
    if(f->type==EOF_||f->module_name!=modname) return false;
    return true;
}

inline void finsert_module(const std::string &name, const token_type &type, const std::vector<symbol> &args, const std::string &stru="", const std::string &modname="") {
    ftable.insert_or_assign(name, fsymbol{type, args, false, stru, modname});
}

inline void finsert_arg(const std::string &name, const symbol &arg) {
    fsymbol* func = fsearch(name);
    if(func) {
        func->args.emplace_back(arg);
    }
}

inline bool is_struct(const std::string &name) {
    for(auto &x : struct_list) {
        if(x==name) return true;
    }
    return false;
}

bool exist_module(const std::string &name, const std::string &modname);
symbol search_module(const std::string &name, const std::string &modname="");


inline void insert_symbol(const std::string &name, const symbol &s) {
    if(table.empty()) [[unlikely]] return;
    table[table.size()-1].insert_or_assign(name, s);
}