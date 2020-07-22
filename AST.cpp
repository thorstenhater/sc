#include "AST.hpp"

namespace AST {
        void to_sexp(std::ostream& os, const expr& e) {
                ToSExp to_sexp(os);
                std::visit(to_sexp, *e);
        }

        std::string genvar() {
                static int counter = 0;
                return "__ast_var_" + std::to_string(counter++);
        }

        void AlphaConvert::push_env(const std::string& k, const std::string& v) {
                env.emplace_back(k, v);
        }
        void AlphaConvert::pop_env() {
                env.pop_back();
        }
        std::optional<std::string> AlphaConvert::find_env(const std::string& k) {
                auto res = std::find_if(env.rbegin(), env.rend(), [&](const auto& p) { return p.first == k; });
                return (res == env.rend()) ? std::nullopt : std::optional<std::string>{res->second};
        }


        void AlphaConvert::operator()(const F64& e) { result = make_expr<F64>(e); }
        void AlphaConvert::operator()(const Bool& e) { result = make_expr<Bool>(e); }
        void AlphaConvert::operator()(const Prim& e)  {
                auto tmp = e;
                for (auto& arg: tmp.args) {
                        std::visit(*this, *arg);
                        arg = result;
                }
                result = make_expr<Prim>(tmp);
        }
        void AlphaConvert::operator()(const Tuple& e) {
                auto tmp = e;
                for (auto& field: tmp.fields) {
                        std::visit(*this, *field);
                        field = result;
                }
                result = make_expr<Tuple>(tmp);
        }
        void AlphaConvert::operator()(const Proj& e) {
                auto tmp = e;
                std::visit(*this, *tmp.tuple);
                tmp.tuple = result;
                result = make_expr<Proj>(tmp);

        }
        void AlphaConvert::operator()(const Var& e)   {
                auto tmp = e;
                tmp.name = find_env(e.name).value_or(e.name);
                result = make_expr<Var>(tmp);
        }
        void AlphaConvert::operator()(const Let& e) {
                auto tmp = e;
                std::visit(*this, *tmp.val);
                tmp.val = result;
                tmp.var = genvar();
                push_env(e.var, tmp.var);
                std::visit(*this, *tmp.body);
                tmp.body = result;
                result = make_expr<Let>(tmp);
                pop_env();
        }
        void AlphaConvert::operator()(const Lam& e) {
                auto tmp = e;
                for (auto& arg: tmp.args) {
                        auto rep = genvar();
                        push_env(arg, rep);
                        arg = rep;
                }
                std::visit(*this, *tmp.body);
                tmp.body = result;
                result = make_expr<Lam>(tmp);
                for (auto& arg: tmp.args) {
                        pop_env();
                }
        }
        void AlphaConvert::operator()(const App& e) {
                auto tmp = e;
                std::visit(*this, *tmp.fun);
                tmp.fun = result;
                for (auto& arg: tmp.args) {
                        std::visit(*this, *arg);
                        arg = result;
                }
                result = make_expr<App>(tmp);
        }
        void AlphaConvert::operator()(const Cond& e) {
                auto tmp = e;
                std::visit(*this, *tmp.pred);
                tmp.pred = result;
                std::visit(*this, *tmp.on_t);
                tmp.on_t = result;
                std::visit(*this, *tmp.on_f);
                tmp.on_f = result;
                result = make_expr<Cond>(tmp);
        }

        expr alpha_convert(const expr& e) {
                auto alpha = AST::AlphaConvert();
                std::visit(alpha, *e);
                return alpha.result;
        }

        template<typename E, typename... Ts> expr make_expr(const Ts&... args) { return std::make_shared<Expr>(E(args...)); }
        expr boolean(const bool b) { return make_expr<Bool>(b); }
        expr tuple(const std::vector<expr>& fs) { return make_expr<Tuple>(fs); }
        expr project(size_t i, const expr& f) { return make_expr<Proj>(i, f); }
        expr cond(const expr& p, const expr& t, const expr& f) { return make_expr<Cond>(p, t, f); }
        expr f64(double v) { return make_expr<F64>(v); }
        expr var(const std::string& n) { return make_expr<Var>(n); }
        expr add(const expr& l, const expr& r) { return make_expr<Prim>("+", std::vector<expr>{l, r}); }
        expr mul(const expr& l, const expr& r) { return make_expr<Prim>("*", std::vector<expr>{l, r}); }
        expr sub(const expr& l, const expr& r) { return make_expr<Prim>("-", std::vector<expr>{l, r}); }
        expr lambda(const std::vector<std::string>& args, const expr& body) { return make_expr<Lam>(args, body); }
        expr apply(const expr& fun, const std::vector<expr>&& args) { return make_expr<App>(fun, args); }
        expr let(const std::string& var, const expr& bind, const expr& in, const Types::type& t) { return make_expr<Let>(var, bind, in, t); }

        expr pi(const std::string& var, size_t field, const expr& tuple, const expr& in) { return let(var, project(field, tuple), in); }
        expr defn(const std::string& name, const std::vector<std::string>& args, const expr& body, const expr& in) { return let(name, lambda(args, body), in); }

        expr operator"" _var(const char* v, size_t) { return var(v); }
        expr operator"" _f64(long double v) { return f64(v); }
        expr operator *(const expr& l, const expr& r) { return mul(l, r); }
        expr operator +(const expr& l, const expr& r) { return add(l, r); }
        expr operator -(const expr& l, const expr& r) { return sub(l, r); }

        expr typecheck(const expr& e) {
                auto types = TypeCheck();
                return std::visit(types, *e);
        }
        void type_error(const std::string& m, const expr& ctx) {
                std::stringstream ss;
                ss << m;
                if (ctx) {
                        ss << "\n";
                        auto sexp = AST::ToSExp(ss, 2, "  |");
                        std::visit(sexp, *ctx);
                }
                throw TypeError{ss.str()};
        }
        Types::type get_type(const expr& e) {
                struct Get {
                        Types::type operator()(const F64& e) { return e.type; }
                        Types::type operator()(const Bool& e) { return e.type; }
                        Types::type operator()(const Prim& e) { return e.type; }
                        Types::type operator()(const Tuple& e) { return e.type; }
                        Types::type operator()(const Proj& e) { return e.type; }
                        Types::type operator()(const Var& e) { return e.type; }
                        Types::type operator()(const Let& e) { return e.type; }
                        Types::type operator()(const Lam& e) { return e.type; }
                        Types::type operator()(const App& e) { return e.type; }
                        Types::type operator()(const Cond& e) { return e.type; }
                };
                return std::visit(Get(), *e);
        }
}
