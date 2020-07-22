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

    using Expr = std::variant<F64,
                              Bool,
                              Tuple,
                              Proj,
                              Var,
                              App,
                              Lam,
                              Prim,
                              Let,
                              Cond>;
    using expr = std::shared_ptr<Expr>;

    struct F64: Types::Typed {
        const double val;
        F64(double v): val{v} {}
    };

    struct Bool: Types::Typed {
        const bool val;
        Bool(bool v): val{v} {}
    };

    struct Proj: Types::Typed {
        expr tuple;
        size_t field;
        Proj(const size_t f, const expr& t): tuple{t}, field{f} {}
    };

    struct Tuple: Types::Typed {
        std::vector<expr> fields;
        Types::type type = nullptr;
        Tuple(const std::vector<expr>& fs): fields{fs} {}
    };

    struct Var: Types::Typed {
        std::string name;
        Types::type type = nullptr;
        Var(const std::string& n): name{n} {}
    };

    struct App: Types::Typed {
        expr fun;
        std::vector<expr> args;
        App(const expr& f, const std::vector<expr>& as): fun{f}, args{as} {}
    };

    struct Prim: Types::Typed {
        std::string op;
        std::vector<expr> args;
        Prim(const std::string& o, const std::vector<expr>& as): op{o}, args{as} {}
    };

    struct Lam: Types::Typed {
        std::vector<std::string> args;
        expr body;
        Types::type type = nullptr;
        Lam(const std::vector<std::string>& as, const expr& b): args{as}, body{b} {}
    };

    struct Let: Types::Typed {
        std::string var;
        expr val;
        expr body;
        Let(const std::string& n, const expr& v, const expr& b, const Types::type& t): var{n}, val{v}, body{b}, Typed{t} {}
    };
    
    struct Cond: Types::Typed {
        expr pred;
        expr on_t;
        expr on_f;
        Types::type type = nullptr;
        Cond(const expr& p, const expr& t, const expr& f): pred{p}, on_t{t}, on_f{f} {}
    };
  
    struct ToSExp {
        std::ostream& os;
        int indent;
        std::string prefix = "";

        ToSExp(std::ostream& os_, int i=0, const std::string& p=""): os{os_}, indent{i}, prefix{p} { os << prefix << std::string(indent, ' '); }

        void operator()(const Prim& e)  {
           os << "("
               << e.op
               << " ";
            for (const auto& arg: e.args) {
                std::visit(*this, *arg);
                os << " ";
            }
            os << "): " << Types::show_type(e.type);
        }
        void operator()(const Bool& e){ os << (e.val ? "true" : "false")  << ": " << Types::show_type(e.type); }
        void operator()(const F64& e) { os << e.val  << ": " << Types::show_type(e.type); }
        void operator()(const Var& e) { os << e.name << ": " << Types::show_type(e.type); }
        void operator()(const Lam& e) {
            os << "(lambda (";
            for (const auto& arg: e.args) os << arg << " ";
            indent += 4;
            os << "): " << Types::show_type(e.type) << '\n' << prefix << std::string(indent, ' ');
            std::visit(*this, *e.body);
            os << ")";
            indent -= 4;
        }
        void operator()(const Cond& e) {
            os << "(if ";
            std::visit(*this, *e.pred);
            indent += 4;
            os << "\n" << prefix << std::string(indent, ' ');
            std::visit(*this, *e.on_t);
            os << "\n" << prefix << std::string(indent, ' ');
            std::visit(*this, *e.on_f);
            os << ")";
            indent -= 4;
        }
        void operator()(const Tuple& e) {
            os << "(";
            for (const auto& field: e.fields) {
                std::visit(*this, *field);
                os << ", ";
            }
            os << ")";
        }
        void operator()(const Proj& e) {
            os << "(pi-" << e.field << " ";
            std::visit(*this, *e.tuple);
            os << ")";
        }
        void operator()(const App& e) {
            os << "(";
            std::visit(*this, *e.fun);
            os << " ";
            for (const auto& arg: e.args) {
                std::visit(*this, *arg);
                os << " ";
            }
            os << ")";
        }
        void operator()(const Let& e) {
            os << "(let (" << e.var << " ";
            std::visit(*this, *e.val);
            os << ") ";
            indent += 4;
            os << "\n" << prefix << std::string(indent, ' ');
            std::visit(*this, *e.body);
            os << ")";
            indent -= 4;
        }
    };

    void to_sexp(std::ostream& os, const expr& e);

    std::string genvar();

    struct AlphaConvert {
        expr result;
        std::vector<std::pair<std::string, std::string>> env;
        void push_env(const std::string&, const std::string&);
        void pop_env();
        std::optional<std::string> find_env(const std::string&);
        void operator()(const F64& e);
        void operator()(const Bool& e);
        void operator()(const Prim& e);
        void operator()(const Tuple& e);
        void operator()(const Proj& e);
        void operator()(const Var& e);
        void operator()(const Let& e);
        void operator()(const Lam& e);
        void operator()(const App& e);
        void operator()(const Cond& e);
    };

    expr alpha_convert(const expr& e);

    template<typename E, typename... Ts> expr make_expr(const Ts&... args);
    expr boolean(const bool);
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
    expr let(const std::string& var, const expr& bind, const expr& in, const Types::type& t=nullptr);

    expr pi(const std::string& var, size_t field, const expr& tuple, const expr& in);
    expr defn(const std::string& name, const std::vector<std::string>& args, const expr& body, const expr& in);

    expr operator"" _var(const char* v, size_t);
    expr operator"" _f64(long double v);
    expr operator *(const expr& l, const expr& r);
    expr operator +(const expr& l, const expr& r);
    expr operator -(const expr& l, const expr& r);

    void type_error(const std::string& m, const expr& ctx=nullptr);

    Types::type get_type(const expr& e);

    using namespace Types;
    struct TypeCheck {
        std::vector<std::unordered_map<std::string, type>> context;
        std::unordered_map<std::string, type> type_vars;
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

        void unify(const type& lhs, const type& rhs, const expr& ctx=nullptr) {
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

        expr operator()(const Var& e) {
            auto tmp = e;
            auto name = e.name;
            type res = nullptr;
            for (auto ctx = context.rbegin(); ctx != context.rend(); ++ctx) {
                if (ctx->find(name) != ctx->end()) {
                    res = (*ctx)[name];
                    break;
                }
            }
            if (!res) {
                auto ty = genvar_t();
                context.back()[name] = ty;
                res = ty;
            }
            tmp.type = res;
            return make_expr<Var>(tmp);
        }
        expr operator()(const F64& e) {
            auto tmp = e;
            tmp.type = f64_t();
            return make_expr<F64>(tmp);
        }
        expr operator()(const Prim& e) {
            auto tmp = e;
            if ((e.op == "*") ||
                (e.op == "-") ||
                (e.op == "+")) {
                if (e.args.size() != 2) { throw std::runtime_error("Arity error: "s + e.op); }
                for (auto& arg: tmp.args) {
                    arg = std::visit(*this, *arg);
                    unify(get_type(arg), f64_t(), std::make_shared<Expr>(e));
                }
                tmp.type = f64_t();
                return make_expr<Prim>(tmp);
            }
            throw std::runtime_error("Unknow prim op: "s + e.op);
        }
        expr operator()(const Tuple& e) {
            auto tmp = e;
            auto fields = std::vector<Types::type>{};
            for (auto& field: tmp.fields) {
                field = std::visit(*this, *field);
                fields.push_back(get_type(field));
            }
            tmp.type = tuple_t(fields);
            return make_expr<Tuple>(tmp);
        }
        expr operator()(const Proj& e) {
            auto tmp = e;
            tmp.tuple = std::visit(*this, *tmp.tuple);
            auto ty = std::make_shared<Type>(TyTuple{{}, -1});
            auto& tuple_ty = std::get<TyTuple>(*ty);
            for (auto ix = 0ul; ix <= e.field; ++ix) {
                tuple_ty.field_types.push_back(genvar_t());
            }
            unify(get_type(tmp.tuple), ty, std::make_shared<Expr>(e));
            tmp.type  =tuple_ty.field_types[e.field];
            return make_expr<Proj>(tmp);
        }
        expr operator()(const App& e) {
            auto tmp = e;
            tmp.fun = std::visit(*this, *tmp.fun);
            auto ty_fun = get_type(tmp.fun);
            if (!std::holds_alternative<TyFunc>(*ty_fun)) { type_error("IMPOSSIBLE: Must unify with function", std::make_shared<Expr>(e)); }
            auto func = std::get<TyFunc>(*ty_fun);
            for (auto ix = 0ul; ix < func.args.size(); ++ix) {
                tmp.args[ix] = std::visit(*this, *tmp.args[ix]);
                auto ty_arg = get_type(tmp.args[ix]);
                unify(func.args[ix], ty_arg, std::make_shared<Expr>(e));
            }
            tmp.type = func.result;
            return make_expr<App>(tmp);
        }
        expr operator()(const Let& e) {
            auto tmp = e;
            tmp.val = std::visit(*this, *tmp.val);
            auto ty_val = get_type(tmp.val);
            context.push_back({{e.var, ty_val}});
            tmp.body = std::visit(*this, *tmp.body);
            if (e.type) {
                unify(tmp.type, e.type, std::make_shared<Expr>(e));
            }
            tmp.type = get_type(tmp.body);
            context.pop_back();
            return make_expr<Let>(tmp);
        }
        expr operator()(const Lam& e) {
            auto tmp = e;
            context.push_back({});
            auto args = std::vector<type>{};
            for (const auto& arg: e.args) {
                type t = genvar_t();
                args.push_back(t);
                context.back()[arg] = t;
             }
            tmp.body = std::visit(*this, *e.body);
            auto ty_body = get_type(tmp.body);
            context.pop_back();
            tmp.type = func_t(args, ty_body);
            return make_expr<Lam>(tmp);
        }
        expr operator()(const Bool& e) {
            auto tmp = e;
            tmp.type = bool_t();
            return make_expr<Bool>(e);
        }
        expr operator()(const Cond& e) {
            auto tmp = e;
            tmp.pred = std::visit(*this, *tmp.pred);
            auto ty_pred = get_type(tmp.pred);
            unify(ty_pred, bool_t(), std::make_shared<Expr>(e));
            tmp.on_t = std::visit(*this, *tmp.on_t);
            tmp.on_f = std::visit(*this, *tmp.on_f);
            unify(get_type(tmp.on_t), get_type(tmp.on_f), std::make_shared<Expr>(e));
            tmp.type = get_type(tmp.on_t);
            return make_expr<Cond>(tmp);
        }
    };
    
    expr typecheck(const expr& e);
}
