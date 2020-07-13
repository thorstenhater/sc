#pragma once

#include <vector>
#include <string>
#include <memory>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <unordered_map>
#include <exception>

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

    void type_error(const std::string& m, const AST::Expr* ctx=nullptr) {
        std::stringstream ss;
        ss << m;
        if (ctx) {
            ss << "\n";
            auto sexp = AST::ToSExp(ss, 2, "  |");
            ctx->accept(sexp);
        }
        throw TypeError{ss.str()};
    }

    struct Type {
        virtual ~Type() = default;
        virtual std::string show() { return "TYPE"; }
        virtual bool equal(const Type& ) const { return true; }
        bool operator==(const Type& other) const { return typeid(*this) == typeid(other) && equal(other); }
        bool operator!=(const Type& other) const { return !(*this == other); }
    };

    using type = std::shared_ptr<Type>;

    struct TyFunc: Type {
        virtual ~TyFunc() = default;
        std::vector<type> args;
        type result;
        TyFunc(const std::vector<type>& as, const type& r): args{as}, result{r} {}
        virtual std::string show() override {
            auto res = std::stringstream{};
            res << "(";
            for (const auto& arg: args) res << arg->show() << ", ";
            res << ") -> ";
            res << result->show();
            return res.str();
        }
        virtual bool equal(const Type& rhs) const override {
            const auto& other = static_cast<const TyFunc&>(rhs);
            if (result != other.result) {
                return false;
            }
            if (args.size() != other.args.size()) {
                return false;
            }
            for (auto ix = 0ul; ix < args.size(); ++ix) {
                if (args[ix] != other.args[ix]) {
                    return false;
                }
            }
            return true;
        }
    };

    struct TyF64: Type {
        virtual std::string show() override { return "F64"; }
        virtual ~TyF64() = default;
    };

    struct TyTuple: Type {
        virtual ~TyTuple() = default;
        std::vector<type> field_types;
        size_t size;
        TyTuple() = default;
        TyTuple(const std::vector<type>& fs): field_types{fs}, size{0} {}
        TyTuple(const std::vector<type>& fs, const size_t i): field_types{fs}, size{i} {}
        virtual std::string show() override {
            auto res = std::stringstream{};
            res << "(";
            for (const auto& field: field_types) res << field->show() << ", ";
            res << ")";
            return res.str();
        }
        bool equal(const Type& rhs) const override {
            const auto& other = static_cast<const TyTuple&>(rhs);
            if (field_types.size() != other.field_types.size()) {
                return false;
            }
            for (auto ix = 0ul; ix < field_types.size(); ++ix) {
                if (field_types[ix] != other.field_types[ix]) {
                    return false;
                }
            }
            return true;
        }
    };

    struct TyBool: Type {
        virtual ~TyBool() = default;
        virtual std::string show() override { return "Bool"; }
    };

    struct TyVar: Type {
        virtual ~TyVar() = default;
        static int counter;
        std::string name;
        type alias;
        TyVar(): name{"__ty_var_" + std::to_string(counter++)}, alias{nullptr} {}
        TyVar(const std::string& n): name{n} {}
        virtual std::string show() override {
            if (alias.get()) {
                return alias->show();
            } else {
                return name;
            }
        }
        virtual bool equal(const Type& rhs) const override {
            const auto& other = static_cast<const TyVar&>(rhs);
            return other.name == name;
        }
    };

    int TyVar::counter = 0;

    template<typename E, typename... Ts> type make_type(const Ts&... args) { return std::make_shared<E>(E(args...)); }
    type f64_t()  { return make_type<TyF64>(); }
    type var_t()  { return make_type<TyVar>(); }
    type var_t(const std::string& n)  { return make_type<TyVar>(n); }
    type tuple_t(const std::vector<type> fields) { return make_type<TyTuple>(fields, fields.size()); }
    type bool_t() { return make_type<TyBool>(); }
    type func_t(const std::vector<type>& args, const type& res) { return make_type<TyFunc>(args, res); }

    struct TypeCheck: AST::Visitor {
        std::vector<std::unordered_map<std::string, type>> context;
        std::unordered_map<std::string, type> type_vars;
        type result;

        TypeCheck(): context({{}}) {}

        type solve(const type& ty_) {
            // if (!ty_) { return var_t(); }
            type ty = ty_;
            for(;;) {
                auto ty_var = dynamic_cast<TyVar*>(ty.get());
                if (ty_var) {
                    if (type_vars.find(ty_var->name) != type_vars.end()) {
                        ty = type_vars[ty_var->name];
                    } else {
                        break;
                    }
                } else {
                    break;
                }
            }
            return ty;
        }

        type type_of(const AST::expr& e) {
            e->accept(*this);
            return result;
        }

        void unify(const type& lhs, const type& rhs, const AST::Expr* ctx=nullptr) {
            auto ty_lhs = solve(lhs);
            auto ty_rhs = solve(rhs);
            if (*ty_lhs == *ty_rhs) {
                return;
            }
            auto ty_var_lhs = dynamic_cast<TyVar*>(ty_lhs.get());
            auto ty_var_rhs = dynamic_cast<TyVar*>(ty_rhs.get());
            if (ty_var_lhs && ty_var_rhs) {
                type_vars[ty_var_lhs->name] = ty_rhs;
                ty_var_lhs->alias = ty_rhs;
                return;
            }
            if (ty_var_lhs) {
                type_vars[ty_var_lhs->name] = ty_rhs;
                ty_var_lhs->alias = ty_rhs;
                return;
            }
            if (ty_var_rhs) {
                type_vars[ty_var_rhs->name] = ty_lhs;
                ty_var_rhs->alias = ty_lhs;
                return;
            }


            TyTuple* ty_tuple_lhs = dynamic_cast<TyTuple*>(ty_lhs.get());
            TyTuple* ty_tuple_rhs = dynamic_cast<TyTuple*>(ty_rhs.get());
            if (ty_tuple_lhs && ty_tuple_rhs) {
                // Need to extend LHS
                if (ty_tuple_lhs->field_types.size() < ty_tuple_rhs->field_types.size()) {
                    if (ty_tuple_lhs->size) {
                        type_error("Cannot unify types "s + ty_lhs->show() + " and " + ty_rhs->show(), ctx);
                    }
                    for (auto ix = ty_tuple_lhs->field_types.size();
                         ix < ty_tuple_rhs->field_types.size(); ++ix) {
                        ty_tuple_lhs->field_types.push_back(var_t());
                    }
                }
                // Need to extend rhs
                if (ty_tuple_rhs->field_types.size() < ty_tuple_lhs->field_types.size()) {
                    if (ty_tuple_rhs->size) {
                        type_error("Cannot unify types "s + ty_lhs->show() + " and " + ty_rhs->show(), ctx);
                    }
                    for (auto ix = ty_tuple_rhs->field_types.size();
                         ix < ty_tuple_lhs->field_types.size(); ++ix) {
                        ty_tuple_rhs->field_types.push_back(var_t());
                    }
                }
                if (ty_tuple_rhs->field_types.size() != ty_tuple_lhs->field_types.size()) {
                    type_error("Cannot unify types "s + ty_lhs->show() + " and " + ty_rhs->show(), ctx);
                }
                for (auto ix = 0ul;
                     ix < ty_tuple_lhs->field_types.size();
                     ++ix) {
                    unify(ty_tuple_lhs->field_types[ix], ty_tuple_rhs->field_types[ix], ctx);
                }
                return;
            }
            type_error("Cannot unify types "s + ty_lhs->show() + " and " + ty_rhs->show(), ctx);
        }

        virtual void visit(const AST::Var& e) {
            auto name = e.name;
            for (auto ctx = context.rbegin(); ctx != context.rend(); ++ctx) {
                if (ctx->find(name) != ctx->end()) {
                    result = (*ctx)[name];
                    return;
                }
            }
            auto ty = var_t();
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
            auto ty = result;
            auto tuple_ty = std::make_shared<TyTuple>();
            for (auto ix = 0ul; ix <= e.field; ++ix) {
                tuple_ty->field_types.push_back(var_t());
            }
            unify(ty, tuple_ty, &e);
            result = tuple_ty->field_types[e.field];
        }
        virtual void visit(const AST::App& e) {
            e.fun->accept(*this);
            auto ty_app = result;
            Type* ty = ty_app.get();
            TyFunc* func_t = dynamic_cast<TyFunc*>(ty);
            if (!func_t) {
                TyVar* ty_var = dynamic_cast<TyVar*>(ty);
                if (!ty_var) {
                    type_error("Got "s + ty->show() + " expected a function", &e);
                }
                
            }

            auto args = dynamic_cast<AST::Tuple*>(e.args.get());
            if (!args) {
                type_error("expected a tuple", &e);
            }


            for (auto ix = 0ul; ix < args->fields.size(); ++ix) {
                args->fields[ix]->accept(*this);
                unify(func_t->args[ix], result, &e);
            }
            result = func_t->result;
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
                type t = var_t();
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
