#include "AST.hpp"

namespace AST {
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
}
