#pragma once

#include <functional>
#include <optional>
#include <vector>
#include <string>
#include <memory>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <unordered_map>
#include <exception>

#include "Types.hpp"

using namespace std::string_literals;

namespace AST {
    struct F64;
    struct Bool;
    struct Tuple;
    struct Proj;
    struct Var;
    struct App;
    struct Lam;
    struct Prim;
    struct Let;
    struct Cond;

    struct Visitor {
        virtual void visit(const F64&)   = 0;
        virtual void visit(const Prim&)   = 0;
        virtual void visit(const Tuple&) = 0;
        virtual void visit(const Proj&)  = 0;
        virtual void visit(const Var&)   = 0;
        virtual void visit(const Let&)   = 0;
        virtual void visit(const Lam&)   = 0;
        virtual void visit(const App&)   = 0;
        virtual void visit(const Bool&)  = 0;
        virtual void visit(const Cond&)  = 0;
    };

    struct Expr {
        virtual void accept(Visitor&) const {};
    };

    using expr = std::shared_ptr<Expr>;

    struct F64: Expr {
        const double val;
        virtual ~F64() = default;
        F64(double v): val{v} {}
        virtual void accept(Visitor& v) const override { v.visit(*this); }
    };

    struct Bool: Expr {
        const bool val;
        virtual ~Bool() = default;
        Bool(bool v): val{v} {}
        virtual void accept(Visitor& v) const override { v.visit(*this); }
    };

    struct Proj: Expr {
        expr tuple;
        size_t field;
        virtual ~Proj() = default;
        Proj(const size_t f, const expr& t): tuple{t}, field{f} {}
        virtual void accept(Visitor& v) const override { v.visit(*this); }
    };

    struct Tuple: Expr {
        std::vector<expr> fields;
        virtual ~Tuple() = default;
        Tuple() = default;
        Tuple(const std::vector<expr>& fs): fields{fs} {}
        virtual void accept(Visitor& v) const override { v.visit(*this); };
    };

    struct Var: Expr {
        virtual ~Var() = default;
        std::string name;
        std::shared_ptr<Types::Type> type = nullptr;
        Var(const std::string& n): name{n} {}
        virtual void accept(Visitor& v) const override { v.visit(*this); };
    };

    struct App: Expr {
        expr fun;
        std::vector<expr> args;
        virtual ~App() = default;
        App(const expr& f, const std::vector<expr>& as): fun{f}, args{as} {}
        virtual void accept(Visitor& v) const override { v.visit(*this); };
    };

    struct Prim: Expr {
        std::string op;
        std::vector<expr> args;
        virtual ~Prim() = default;
        Prim(const std::string& o, const std::vector<expr>& as): op{o}, args{as} {}
        virtual void accept(Visitor& v) const override { v.visit(*this); };
    };

    struct Lam: Expr {
        std::vector<std::string> args;
        expr body;
        virtual ~Lam() = default;
        Lam(const std::vector<std::string>& as, const expr& b): args{as}, body{b} {}
        virtual void accept(Visitor& v) const override { v.visit(*this); };
    };

    struct Let: Expr {
        std::string var;
        expr val;
        expr body;
        virtual ~Let() = default;
        Let(const std::string& n, const expr& v, const expr& b): var{n}, val{v}, body{b} {}
        virtual void accept(Visitor& v) const override { v.visit(*this); };
    };
    
    struct Cond: Expr {
        expr pred;
        expr on_t;
        expr on_f;
        virtual ~Cond() = default;
        Cond(const expr& p, const expr& t, const expr& f): pred{p}, on_t{t}, on_f{f} {}
        virtual void accept(Visitor& v) const override { v.visit(*this); };
    };
  
    struct ToSExp: Visitor {
        virtual std::string name() { return "to sexp"; }

        std::ostream& os;
        int indent;
        std::string prefix = "";

        ToSExp(std::ostream& os_, int i=0, const std::string& p=""): os{os_}, indent{i}, prefix{p} { os << prefix << std::string(indent, ' '); }

        virtual void visit(const Prim& e) override {
            os << "("
               << e.op
               << " ";
            for (const auto& arg: e.args) {
                arg->accept(*this);
                os << " ";
            }
            os << ")";
        }

        virtual void visit(const Bool& e) override {
            os << (e.val ? "true" : "false");
        }

        virtual void visit(const F64& e) override { os << e.val; }

        virtual void visit(const Var& e) override { os << e.name; }

        virtual void visit(const Lam& e) override {
            os << "(lambda (";
            for (const auto& arg: e.args) {
                os << arg << " ";
            }
            indent += 4;
            os << ")\n" << prefix << std::string(indent, ' ');
            e.body->accept(*this);
            os << ")";
            indent -= 4;
        }

        virtual void visit(const Cond& e) override {
            os << "(if ";
            e.pred->accept(*this);
            indent += 4;
            os << "\n" << prefix << std::string(indent, ' ');
            e.on_t->accept(*this);
            os << "\n" << prefix << std::string(indent, ' ');
            e.on_f->accept(*this);
            os << ")";
            indent -= 4;
        }

        virtual void visit(const Tuple& e) override {
            os << "(";
            for (const auto& field: e.fields) {
                field->accept(*this);
                os << ", ";
            }
            os << ")";
        }

        virtual void visit(const Proj& e) override {
            os << "(pi-" << e.field << " ";
            e.tuple->accept(*this);
            os << ")";
        }

        virtual void visit(const App& e) override {
            os << "(";
            e.fun->accept(*this);
            os << " ";
            for (const auto& arg: e.args) {
                arg->accept(*this);
                os << " ";
            }
            os << ")";
        }

        virtual void visit(const Let& e) override {
            os << "(let (" << e.var << " ";
            e.val->accept(*this);
            os << ") ";
            indent += 4;
            os << "\n" << prefix << std::string(indent, ' ');
            e.body->accept(*this);
            os << ")";
            indent -= 4;
        }
    };

    void to_sexp(std::ostream& os, const expr& e);

    std::string genvar();

    struct AlphaConvert: Visitor {
        expr result;
        std::vector<std::pair<std::string, std::string>> env;
        void push_env(const std::string&, const std::string&);
        void pop_env();
        std::optional<std::string> find_env(const std::string&);
        virtual void visit(const F64& e) override;
        virtual void visit(const Bool& e) override;
        virtual void visit(const Prim& e) override;
        virtual void visit(const Tuple& e) override;
        virtual void visit(const Proj& e) override;
        virtual void visit(const Var& e) override;
        virtual void visit(const Let& e) override;
        virtual void visit(const Lam& e) override;
        virtual void visit(const App& e) override;
        virtual void visit(const Cond& e) override;
    };

    expr alpha_convert(const expr& e);

    namespace convenience {
        template<typename E, typename... Ts> expr make_expr(const Ts&... args);
        expr nil();
        expr yes();
        expr no();
        expr tuple(const std::vector<expr>& fs);
        expr project(size_t i, const expr& f);
        expr cond(const expr& p, const expr& t, const expr& f);
        expr f64(double v);
        expr var(const std::string& n);
        expr add(const expr& l, const expr& r);
        expr mul(const expr& l, const expr& r);
        expr sub(const expr& l, const expr& r);
        expr lambda(const std::vector<std::string>& args, const expr& body);
        expr apply(const expr& fun, const std::vector<expr>&& args);
        expr let(const std::string& var, const expr& bind, const expr& in);

        expr pi(const std::string& var, size_t field, const expr& tuple, const expr& in);
        expr defn(const std::string& name, const std::vector<std::string>& args, const expr& body, const expr& in);

        expr operator"" _var(const char* v, size_t);
        expr operator"" _f64(long double v);
        expr operator *(const expr& l, const expr& r);
        expr operator +(const expr& l, const expr& r);
        expr operator -(const expr& l, const expr& r);
    }

    void type_error(const std::string& m, const Expr* ctx=nullptr);

    using namespace Types;
    struct TypeCheck: Visitor {
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

        void unify(const type& lhs, const type& rhs, const Expr* ctx=nullptr) {
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

        virtual void visit(Var& e) {
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

        virtual void visit(const Var& e) { std::cout << "Ooops\n"; }


        virtual void visit(const F64&) { result = f64_t(); }

        virtual void visit(const Prim& e) {
            if ((e.op == "*") ||
                (e.op == "-") ||
                (e.op == "+")) {
                if (e.args.size() != 2) { throw std::runtime_error("Arity error: "s + e.op); }
                for (auto ix = 0ul; ix < e.args.size(); ++ix) {
                    e.args[ix]->accept(*this);
                    auto ty_arg = result;
                    unify(ty_arg, f64_t(), &e);
                }
                result = f64_t();
                return;
            }
            throw std::runtime_error("Unknow prim op: "s + e.op);
        }

        virtual void visit(const Tuple& e) {
            auto tmp = std::vector<type>{};
            for (const auto& field: e.fields) {
                field->accept(*this);
                tmp.push_back(result);
            }
            result = tuple_t(tmp);
        }
        virtual void visit(const Proj& e) {
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
        virtual void visit(const App& e) {
            e.fun->accept(*this);
            auto ty_fun = solve(result);
            ty_fun = solve(ty_fun);
            if (std::holds_alternative<TyFunc>(*ty_fun)) {
                auto func = std::get<TyFunc>(*ty_fun);
                for (auto ix = 0ul; ix < func.args.size(); ++ix) {
                    e.args[ix]->accept(*this);
                    auto ty_arg = result;
                    unify(func.args[ix], ty_arg);
                }
                result = func.result;
            } else {
                type_error("IMPOSSIBLE: Must unify with function", &e);
            }
        }
        virtual void visit(const Let& e) {
            e.val->accept(*this);
            context.push_back({{e.var, result}});
            e.body->accept(*this);
            context.pop_back();
        }
        virtual void visit(const Lam& e) {
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
        virtual void visit(const Bool&) { result = bool_t(); }
        virtual void visit(const Cond& e) {
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

    type typecheck(const expr& e);

}
