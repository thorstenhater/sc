#include "AST.hpp"

namespace AST {
        void to_sexp(std::ostream& os, const expr& e) {
                ToSExp to_sexp(os);
                e->accept(to_sexp);
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


        void AlphaConvert::visit(const F64& e)  { result = std::make_shared<F64>(e); }
        void AlphaConvert::visit(const Bool& e) { result = std::make_shared<Bool>(e); }
        void AlphaConvert::visit(const Prim& e)  {
                auto tmp = std::make_shared<Prim>(e);
                for (auto& arg: tmp->args) {
                        arg->accept(*this);
                        arg = result;
                }
                result = tmp;
        }
        void AlphaConvert::visit(const Tuple& e) {
                auto tmp = std::make_shared<Tuple>(e);
                for (auto& field: tmp->fields) {
                        field->accept(*this);
                        field = result;
                }
                result = tmp;
        }
        void AlphaConvert::visit(const Proj& e) {
                auto tmp = std::make_shared<Proj>(e);
                tmp->tuple->accept(*this);
                tmp->tuple = result;
                result = tmp;
        }
        void AlphaConvert::visit(const Var& e)   {
                auto tmp = std::make_shared<Var>(e);
                tmp->name = find_env(e.name).value_or(e.name);
                result = tmp;
        }
        void AlphaConvert::visit(const Let& e) {
                auto tmp = std::make_shared<Let>(e);
                tmp->val->accept(*this);
                tmp->val = result;
                tmp->var = genvar();
                push_env(e.var, tmp->var);
                tmp->body->accept(*this);
                tmp->body = result;
                result = tmp;
                pop_env();
        }
        void AlphaConvert::visit(const Lam& e) {
                auto tmp = std::make_shared<Lam>(e);
                for (auto& arg: tmp->args) {
                        auto rep = genvar();
                        push_env(arg, rep);
                        arg = rep;
                }
                tmp->body->accept(*this);
                tmp->body = result;
                result = tmp;
                for (auto& arg: tmp->args) {
                        pop_env();
                }
        }
        void AlphaConvert::visit(const App& e) {
                auto tmp = std::make_shared<App>(e);
                tmp->fun->accept(*this);
                tmp->fun = result;
                for (auto& arg: tmp->args) {
                        arg->accept(*this);
                        arg = result;
                }
                result = tmp;
        }
        void AlphaConvert::visit(const Cond& e) {
                auto tmp = std::make_shared<Cond>(e);
                tmp->pred->accept(*this);
                tmp->pred = result;
                tmp->on_t->accept(*this);
                tmp->on_t = result;
                tmp->on_f->accept(*this);
                tmp->on_f = result;
                result = tmp;
        }

        expr alpha_convert(const expr& e) {
                auto alpha = AST::AlphaConvert();
                e->accept(alpha);
                return alpha.result;
        }

        namespace convenience {
                template<typename E, typename... Ts> expr make_expr(const Ts&... args) { return std::make_shared<E>(E(args...)); }
                expr nil() { return make_expr<Tuple>(); }
                expr yes() { return make_expr<Bool>(true); }
                expr no() { return make_expr<Bool>(false); }
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
                expr let(const std::string& var, const expr& bind, const expr& in) { return make_expr<Let>(var, bind, in); }

                expr pi(const std::string& var, size_t field, const expr& tuple, const expr& in) { return let(var, project(field, tuple), in); }
                expr defn(const std::string& name, const std::vector<std::string>& args, const expr& body, const expr& in) { return let(name, lambda(args, body), in); }

                expr operator"" _var(const char* v, size_t) { return var(v); }
                expr operator"" _f64(long double v) { return f64(v); }
                expr operator *(const expr& l, const expr& r) { return mul(l, r); }
                expr operator +(const expr& l, const expr& r) { return add(l, r); }
                expr operator -(const expr& l, const expr& r) { return sub(l, r); }
        }
        type typecheck(const expr& e) {
                auto types = TypeCheck();
                e->accept(types);
                return types.result;
        }
        void type_error(const std::string& m, const AST::Expr* ctx) {
                std::stringstream ss;
                ss << m;
                if (ctx) {
                        ss << "\n";
                        auto sexp = AST::ToSExp(ss, 2, "  |");
                        ctx->accept(sexp);
                }
                throw TypeError{ss.str()};
        }

}
