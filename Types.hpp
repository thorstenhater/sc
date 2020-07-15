#pragma once

#include <vector>
#include <string>
#include <memory>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <unordered_map>
#include <exception>

#include <variant>

#include "AST.hpp"

using namespace std::string_literals;

namespace Types {
    struct TypeError: std::exception {
        std::string message;
        TypeError(const std::string& m): message{m} {}
        virtual const char* what() const throw() {
            return message.c_str();
        }
    };

    void type_error(const std::string& m, const AST::Expr* ctx=nullptr);

    struct TyF64;
    struct TyBool;
    struct TyFunc;
    struct TyTuple;
    struct TyVar;

    using Type = std::variant<TyF64,
                              TyBool,
                              TyFunc,
                              TyTuple,
                              TyVar>;
    using type = std::shared_ptr<Type>;


    struct TyFunc {
        std::vector<type> args;
        type result;
        TyFunc() = default;
        TyFunc(const std::vector<type>& as, const type& r) noexcept: args{as}, result{r} {}
    };

    struct TyF64 {
        TyF64() = default;
    };

    struct TyTuple {
        std::vector<type> field_types;
        int size;
        TyTuple() = default;
        TyTuple(const std::vector<type>& fs) noexcept: field_types{fs}, size{-1} {}
        TyTuple(const std::vector<type>& fs, const int i) noexcept: field_types{fs}, size{i} {}
    };

    struct TyBool {
        TyBool() = default;
    };

    struct TyVar {
        std::string name;
        type alias;
        TyVar() = default;
        TyVar(const std::string& n) noexcept: name{n}, alias{nullptr} {}
    };

    bool operator==(const TyFunc& lhs, const TyFunc& rhs);
    bool operator==(const TyTuple& lhs, const TyTuple& rhs);
    bool operator==(const TyBool&, const TyBool&);
    bool operator==(const TyF64&, const TyF64&);
    bool operator==(const TyVar& lhs, const TyVar& rhs);

    std::string show_type(const type& t);

    type f64_t();
    type var_t(const std::string& n);
    type tuple_t(const std::vector<type> fields);
    type bool_t();
    type func_t(const std::vector<type>& args, const type& res);

    struct TypeCheck: AST::Visitor {
        std::vector<std::unordered_map<std::string, type>> context;
        std::unordered_map<std::string, type> type_vars;
        type result;
        size_t ty_var_counter;

        TypeCheck(): context({{}}), ty_var_counter{0} {}

        type genvar_t() {
            auto res = var_t("__ty_var_" + std::to_string(ty_var_counter++));
            return res;
        }

        type solve(const type& ty) {
            type res;
            for (res = ty; std::holds_alternative<TyVar>(*res);) {
                auto ty_var = std::get<TyVar>(*res);
                if (type_vars.find(ty_var.name) != type_vars.end()) {
                    res = type_vars[ty_var.name];
                } else {
                    break;
                }
            }
            return res;
        }

        type type_of(const AST::expr& e) {
            e->accept(*this);
            return result;
        }

        void unify(const type& lhs, const type& rhs, const AST::Expr* ctx=nullptr) {
            auto ty_lhs = solve(lhs);
            auto ty_rhs = solve(rhs);
            if ((*ty_lhs) == (*ty_rhs)) { return; }

            if (std::holds_alternative<TyVar>(*ty_lhs)) {
                auto& ty_var_lhs = std::get<TyVar>(*ty_lhs);
                type_vars[ty_var_lhs.name] = ty_rhs;
                ty_var_lhs.alias = ty_rhs;
                return;
            }

            if (std::holds_alternative<TyVar>(*ty_rhs)) {
                auto& ty_var_rhs = std::get<TyVar>(*ty_rhs);
                type_vars[ty_var_rhs.name] = ty_lhs;
                ty_var_rhs.alias = ty_lhs;
                return;
            }

            if (std::holds_alternative<TyTuple>(*ty_lhs) && std::holds_alternative<TyTuple>(*ty_rhs)) {
                auto& ty_tuple_lhs = std::get<TyTuple>(*ty_lhs);
                auto& ty_tuple_rhs = std::get<TyTuple>(*ty_rhs);
                // Need to extend LHS
                auto s_lhs = ty_tuple_lhs.field_types.size();
                auto s_rhs = ty_tuple_rhs.field_types.size();
                if ((s_lhs < s_rhs) && (ty_tuple_lhs.size == -1)) {
                    for (auto ix = s_lhs; ix < s_rhs; ++ix) {
                        ty_tuple_lhs.field_types.push_back(genvar_t());
                    }
                }
                // Need to extend rhs
                if ((s_rhs < s_lhs) && (ty_tuple_rhs.size == -1)) {
                    for (auto ix = s_rhs; ix < s_lhs; ++ix) {
                        ty_tuple_rhs.field_types.push_back(genvar_t());
                    }
                }

                if (ty_tuple_rhs.field_types.size() == ty_tuple_lhs.field_types.size()) {
                    for (auto ix = 0ul; ix < ty_tuple_rhs.field_types.size(); ++ix) {
                        unify(ty_tuple_lhs.field_types[ix], ty_tuple_rhs.field_types[ix], ctx);
                    }
                    return;
                }
            }

            if (std::holds_alternative<TyFunc>(*ty_lhs) && std::holds_alternative<TyFunc>(*ty_rhs)) {
                auto& ty_func_lhs = std::get<TyFunc>(*ty_lhs);
                auto& ty_func_rhs = std::get<TyFunc>(*ty_rhs);
                if (ty_func_lhs.args.size() == ty_func_rhs.args.size()) {
                    for (auto ix = 0ul; ix < ty_func_rhs.args.size(); ++ix) {
                        unify(ty_func_lhs.args[ix], ty_func_rhs.args[ix], ctx);
                    }
                    unify(ty_func_lhs.result, ty_func_rhs.result, ctx);
                    return;
                }
            }

            type_error("Cannot unify types "s + show_type(ty_lhs) + " and " + show_type(ty_rhs), ctx);
        }

        virtual void visit(const AST::Var& e) {
            auto name = e.name;
            for (auto ctx = context.rbegin(); ctx != context.rend(); ++ctx) {
                if (ctx->find(name) != ctx->end()) {
                    result = (*ctx)[name];
                    return;
                }
            }
            auto ty = genvar_t();
            context.back()[name] = ty;
            result = ty;
        }

        virtual void visit(const AST::F64&) { result = f64_t(); }

        virtual void visit(const AST::Prim& e) {
            if ((e.op == "*") ||
                (e.op == "-") ||
                (e.op == "+")) {
                e.args->accept(*this);
                auto ty_arg = result;
                unify(ty_arg, tuple_t({f64_t(), f64_t()}), &e);
                result = f64_t();
                return;
            }
            throw std::runtime_error("Unknow prim op: "s + e.op);
        }

        virtual void visit(const AST::Tuple& e) {
            auto tmp = std::vector<type>{};
            for (const auto& field: e.fields) {
                field->accept(*this);
                tmp.push_back(result);
            }
            result = tuple_t(tmp);
        }
        virtual void visit(const AST::Proj& e) {
            e.tuple->accept(*this);
            auto res = result;

            auto ty = std::make_shared<Type>(TyTuple{{}, -1});
            auto& tuple_ty = std::get<TyTuple>(*ty);
            for (auto ix = 0ul; ix <= e.field; ++ix) {
                tuple_ty.field_types.push_back(genvar_t());
            }
            unify(res, ty, &e);
            result = tuple_ty.field_types[e.field];
        }
        virtual void visit(const AST::App& e) {
            e.fun->accept(*this);
            auto ty_fun = solve(result);

            e.args->accept(*this);
            auto ty_arg = solve(result);
            unify(ty_arg, std::make_shared<Type>(TyTuple{{}, -1}));
            if (std::holds_alternative<TyTuple>(*ty_arg)) {
                auto& fields = std::get<TyTuple>(*ty_arg).field_types;
                unify(ty_fun, func_t(fields, genvar_t()));
            } else {
                type_error("IMPOSSIBLE: Must unify with tuple", &e);
            }

            ty_fun = solve(ty_fun);
            if (std::holds_alternative<TyFunc>(*ty_fun)) {
                result = std::get<TyFunc>(*ty_fun).result;
            } else {
                type_error("IMPOSSIBLE: Must unify with function", &e);
            }
        }
        virtual void visit(const AST::Let& e) {
            e.val->accept(*this);
            context.push_back({{e.var, result}});
            e.body->accept(*this);
            context.pop_back();
        }
        virtual void visit(const AST::Lam& e) {
            context.push_back({});
            auto args = std::vector<type>{};
            for (const auto& arg: e.args) {
                type t = genvar_t();
                args.push_back(t);
                context.back()[arg] = t;
             }

            e.body->accept(*this);
            auto ty_body = result;
            result = func_t(args, ty_body);
            context.pop_back();
        }
        virtual void visit(const AST::Bool&) { result = bool_t(); }
        virtual void visit(const AST::Cond& e) {
            e.pred->accept(*this);
            auto ty_pred = result;
            unify(ty_pred, bool_t(), &e);
            e.on_t->accept(*this);
            auto ty_on_t = result;
            e.on_f->accept(*this);
            auto ty_on_f = result;
            unify(ty_on_t, ty_on_f, &e);
            result = ty_on_t;
        }
    };
}
