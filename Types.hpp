#pragma once

#include <vector>
#include <string>
#include <memory>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <unordered_map>
#include <exception>

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
        virtual std::string show() { return "TYPE"; }
        virtual bool equal(const Type& ) const { return true; }
        bool operator==(const Type& other) const { return typeid(*this) == typeid(other) && equal(other); }
        bool operator!=(const Type& other) const { return !(*this == other); }
    };

    using type = std::shared_ptr<Type>;

    struct TyFunc: Type {
        std::vector<type> args;
        type result;
        TyFunc(const std::vector<type>& as, const type& r): args{as}, result{r} {}
        virtual std::string show() {
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
        virtual std::string show() { return "F64"; }
    };

    struct TyTuple: Type {
        std::vector<type> field_types;
        size_t size;
        TyTuple() = default;
        TyTuple(const std::vector<type>& fs): field_types{fs}, size{0} {}
        TyTuple(const std::vector<type>& fs, const size_t i): field_types{fs}, size{i} {}
        virtual std::string show() {
            auto res = std::stringstream{};
            res << "(";
            for (const auto& field: field_types) res << field->show() << ", ";
            res << ")";
            return res.str();
        }
    };

    struct TyBool: Type {
        virtual std::string show() { return "Bool"; }
    };

    struct TyVar: Type {
        static int counter;
        std::string name;
        type alias;
        TyVar(): name{"__ty_var_" + std::to_string(counter++)}, alias{nullptr} {}
        TyVar(const std::string& n): name{n} {}
        virtual std::string show() {
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
        std::unordered_map<const AST::Expr*, type> types;
        std::unordered_map<std::string, type> type_vars;

        type solve(const type& ty_) {
            if (!ty_) {
                return var_t();
            }
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
        type type_of(const AST::expr& e) { return solve(types[e.get()]); }

        void unify(const type& lhs, const type& rhs, const AST::Expr* ctx=nullptr) {
            auto ty_lhs = solve(lhs);
            auto ty_rhs = solve(rhs);
            if (*ty_lhs != *ty_rhs) {
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

                auto ss = std::stringstream{};
                ss << "Cannot unify types " << ty_lhs->show() << " and " << ty_rhs->show();
                type_error(ss.str(), ctx);
            }
        }

        virtual void visit(const AST::F64& e) { types[&e] = f64_t(); }

        virtual void visit(const AST::Add& e) {
            e.lhs->accept(*this);
            e.rhs->accept(*this);
            unify(types[e.lhs.get()], f64_t(), &e);
            unify(types[e.rhs.get()], f64_t(), &e);
            types[&e] = f64_t();
        }

        virtual void visit(const AST::Mul& e) {
            e.lhs->accept(*this);
            e.rhs->accept(*this);
            unify(types[e.lhs.get()], f64_t(), &e);
            unify(types[e.rhs.get()], f64_t(), &e);
            types[&e] = f64_t();
        }

        virtual void visit(const AST::Tuple& e) {
            auto tmp = std::vector<type>{};
            for (const auto field: e.fields) {
                field->accept(*this);
                tmp.push_back(types[field.get()]);
            }
            types[&e] = tuple_t(tmp);
        }
        virtual void visit(const AST::Proj& e) {
            e.tuple->accept(*this);
            auto ty = type_of(e.tuple);

            TyTuple* tuple_ty = dynamic_cast<TyTuple*>(ty.get());
            if (tuple_ty) {
                if (e.field >= tuple_ty->size) {
                    if (tuple_ty->size) {
                        type_error("Tuple index error: index="s + std::to_string(e.field), &e);
                    } else {
                        for (auto ix = tuple_ty->field_types.size(); ix <= e.field; ++ix) tuple_ty->field_types.push_back(var_t());
                    }
                }
                types[&e] = tuple_ty->field_types[e.field];
                return;
            }

            TyVar* var_ty = dynamic_cast<TyVar*>(ty.get());
            if (var_ty) {
                auto tuple_ty = std::make_shared<TyTuple>();
                for (auto ix = 0ul; ix <= e.field; ++ix) tuple_ty->field_types.push_back(var_t());
                unify(types[e.tuple.get()], tuple_ty, &e);
                types[&e] = tuple_ty->field_types[e.field];
                return;
            }

            type_error("Expected a tuple", &e);
        }
        virtual void visit(const AST::Var& e) {
            // walk contexts outwards and search
            auto name = e.name;
            for (auto ctx = context.rbegin(); ctx != context.rend(); ++ctx) {
                if (ctx->find(name) != ctx->end()) {
                    types[&e] = (*ctx)[name];
                    return;
                }
            }
        }
        virtual void visit(const AST::App& e) {
            e.fun->accept(*this);
            Type* ty = types[&(*e.fun)].get();
            TyFunc* func_t = dynamic_cast<TyFunc*>(ty);
            if (!func_t) {
                type_error("Got "s + ty->show() + " expected a function", &e);
            }
            if (e.args.size() != func_t->args.size()) {
                type_error("Arity does not match: "s + ty->show(), &e);
            }
            for (auto ix = 0ul; ix < e.args.size(); ++ix) {
                e.args[ix]->accept(*this);
                unify(func_t->args[ix], types[e.args[ix].get()], &e);
            }
            types[&e] = func_t->result;
        }
        virtual void visit(const AST::Let& e) {
            e.val->accept(*this);
            context.push_back({{e.var, types[e.val.get()]}});
            e.body->accept(*this);
            types[&e] = types[e.body.get()];
            context.pop_back();
        }
        virtual void visit(const AST::Lam& e) {
            context.push_back({});
            auto args = std::vector<type>{};
            for (const auto arg: e.args) {
                type t = var_t();
                args.push_back(t);
                context.back()[arg] = t;
            }
            e.body->accept(*this);
            types[&e] = func_t(args, types[e.body.get()]);
            context.pop_back();
        }
        virtual void visit(const AST::Bool& e) { types[&e] = bool_t(); }
        virtual void visit(const AST::Cond& e) {
            e.pred->accept(*this);
            e.on_t->accept(*this);
            e.on_f->accept(*this);
            unify(types[e.pred.get()], bool_t(), &e);
            unify(types[e.on_t.get()], types[e.on_f.get()], &e);
            types[&e] = types[e.on_t.get()];
        }
    };
}
