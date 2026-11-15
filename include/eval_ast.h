#pragma once
#include "ast.h"
#include "ast.h"
#include "common.h"
#include "lexer.h"
#include "table.h"
#include <cmath>
#include <concepts>
#include <cstddef>
#include <variant>

class eval_ast {
    public:
    std::string eval_string(const astptr &node) {
        auto n = static_cast<Node*>(node.get());
        return variant2string(n->tok.value);
    }
    double eval_double(const astptr &node) {
        switch (node->kind) {
            case ast_type::JUSTNODE: {
                auto n = static_cast<Node*>(node.get());
                if(n->tok.type==ID) {
                    try {
                        return variant2double(search(variant2string(n->tok.value)).value);
                    } catch(std::bad_variant_access &e) {
                        return variant2float(search(variant2string(n->tok.value)).value);
                    }
                }
                try {
                    return variant2double(n->tok.value);
                } catch(std::bad_variant_access &e) {
                    return variant2float(n->tok.value);
                }
            }
            case ast_type::BINARY: {
                auto n = static_cast<BinaryNode*>(node.get());
                switch(n->op.type) {
                    case PLUS: {
                        return eval_double(n->left) + eval_double(n->right);
                    }
                    case MINUS: {
                        return eval_double(n->left) - eval_double(n->right);
                    }
                    case STAR: {
                        return eval_double(n->left) * eval_double(n->right);
                    }
                    case SLASH: {
                        return eval_double(n->left) / eval_double(n->right);
                    }
                    case MOD: {
                        return std::fmod(eval_double(n->left), eval_double(n->right));
                    }
                    default:
                        break;
                }
            }
            default:
                return 0;
        }
        return 0;
    }
    template<typename T>
    T eval(const astptr &node) {
        switch (node->kind) {
            case ast_type::JUSTNODE: {
                auto n = static_cast<Node*>(node.get());
                if(n->tok.type==ID) {
                    return variant2int<T>(search(variant2string(n->tok.value)).value);
                }
                return variant2int<T>(n->tok.value);
            }
            case ast_type::BINARY: {
                auto n = static_cast<BinaryNode*>(node.get());
                switch(n->op.type) {
                    case PLUS: {
                        return eval<T>(n->left) + eval<T>(n->right);
                    }
                    case MINUS: {
                        return eval<T>(n->left) - eval<T>(n->right);
                    }
                    case STAR: {
                        return eval<T>(n->left) * eval<T>(n->right);
                    }
                    case SLASH: {
                        return eval<T>(n->left) / eval<T>(n->right);
                    }
                    case MOD: {
                        if(std::is_same_v<T, double>) {
                            return std::fmod(eval<T>(n->left), eval<T>(n->right));
                        }
                        return eval<T>(n->left) % eval<T>(n->right);
                    }
                    default:
                        break;
                }
            }
            default:
                return 0;
        }
    }
};
