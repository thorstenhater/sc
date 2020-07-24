#include "TailCPS.hpp"

namespace TailCPS {
    namespace convenience {
        template<typename E, typename... Ts> term make_term(const Ts&... args) { return std::make_shared<Term>(E(args...)); }
        term let(const std::string& n, const value& v, const term& i) { return make_term<LetV>(n, v, i); }
        term pi(int f, const std::string& n, const std::string& t, const term& i) { return make_term<LetT>(f, n, t, i); }
        term let_cont(const std::string& n, const std::vector<variable>& as, const term& b, const term& i) { return make_term<LetC>(n, as, b, i); }
        term let_func(const std::string& n, const std::string& c, const std::vector<variable>& as, const term& b, const term& i, const Types::type& t) { return make_term<LetF>(n, c, as, b, i, t); }
        term let_prim(const std::string& n, const std::string& c, const std::vector<std::string>& a, const term& i, const Types::type& t) { return make_term<LetP>(n, c, a, i, t); }
        term app_cont(const std::string& n, const std::string& a) { return make_term<AppC>(n, a); }
        term app_func(const std::string& n, const std::string& c, const std::vector<std::string>& a) { return make_term<AppF>(n, c, a); }
        term halt(const variable& v) { return make_term<Halt>(v); }

        template<typename E, typename... Ts> value make_value(const Ts&... args) { return std::make_shared<Value>(E(args...)); }
        value f64(double v) { return make_value<F64>(v); }
        value boolean(bool v) { return make_value<Bool>(v); }
        value tuple(const std::vector<variable>& fs, const Types::type& t) { return make_value<Tuple>(fs, t); }
    }

    std::string ToCPS::genvar() {
        return "__var_" + std::to_string(counter++);
    }

    std::string ToCPSHelper::genvar() { return parent.genvar(); }

    void ToCPSHelper::operator()(const AST::Var& e) {
        result = app_cont(ctx, e.name);
    }
    void ToCPSHelper::operator()(const AST::F64& e) {
        auto x = genvar();
        result = let(x, f64(e.val), app_cont(ctx, x));
    }
    void ToCPSHelper::operator()(const AST::Bool& e) {
        auto x = genvar();
        result = let(x, boolean(e.val), app_cont(ctx, x));
    }
    void ToCPSHelper::operator()(const AST::Proj& e) {
        auto kappa = parent.ctx;
        auto x = genvar();
        auto k = ctx;
        parent.ctx = [&](const auto& z){
            parent.result = pi(e.field, x, z, app_cont(k, x));
            return parent.result;
        };
        std::visit(parent, *e.tuple);
        result = parent.result;
        parent.ctx = kappa;
    }
    // NB. this is different (WRONG!) in Kennedy'07
    void ToCPSHelper::operator()(const AST::Lam& e) {
        // recurse into body
        auto tmp = ctx;
        auto j = genvar();
        ctx = j;
        std::visit(*this, *e.body);
        auto body = result;
        ctx = tmp;
        // generate result
        auto f = genvar();
        result = let_func(f, j, e.args, body, app_cont(ctx, f), e.type);
    }
    void ToCPSHelper::app_helper(const std::vector<AST::expr>& args, size_t ix, const variable& zs, std::vector<variable>& ys, const variable& f, const variable& kappa) {
        auto tmp = parent.ctx;
        parent.ctx = [&](auto y) {
            ys.push_back(y);
            if (ix + 1 == args.size()) {
                parent.result = app_func(f, kappa, ys);
                result = parent.result;
            } else {
                app_helper(args, ix + 1, zs, ys, f, kappa);
            }
            return parent.result;
        };
        std::visit(parent, *args[ix]);
        parent.ctx = tmp;
    }
    void ToCPSHelper::operator()(const AST::App& e) {
        auto kappa = parent.ctx;
        auto zs = genvar();
        auto f = genvar();
        std::vector<variable> ys{};
        auto tmp = parent.ctx;
        parent.ctx = [&](auto y) {
            app_helper(e.args, 0, zs, ys, y, ctx);
            return parent.result;
        };
        std::visit(parent, *e.fun);
        result = parent.result;
        parent.ctx = kappa;
    }

    void ToCPSHelper::prim_helper(const std::vector<AST::expr>& args, size_t ix, std::vector<variable>& ys, const variable& op, const variable& kappa, const Types::type& t) {
        auto tmp = parent.ctx;
        parent.ctx = [&](auto y) {
            ys.push_back(y);
            if (ix + 1 == args.size()) {
                auto n = genvar();
                parent.result = let_prim(op, n, ys, app_cont(kappa, n), t);
                result = parent.result;
            } else {
                prim_helper(args, ix + 1, ys, op, kappa, t);
            }
            return parent.result;
        };
        std::visit(parent, *args[ix]);
        result = parent.result;
        parent.ctx = tmp;
    }
    void ToCPSHelper::operator()(const AST::Prim& e) {
        auto kappa = parent.ctx;
        std::vector<variable> ys = {};
        prim_helper(e.args, 0, ys, e.op, ctx, e.type);
        parent.ctx = kappa;
    }

    void ToCPSHelper::tuple_helper(const std::vector<AST::expr>& fields, size_t ix, const variable& x, std::vector<variable>& xs, const variable& kappa, const Types::type& t) {
        auto tmp = parent.ctx;
        parent.ctx = [&](auto z1) {
            xs.push_back(z1);
            if (ix + 1 == fields.size()) {
                parent.result = let(x, tuple(xs, t), app_cont(kappa, x));
            } else {
                tuple_helper(fields, ix + 1, x, xs, kappa, t);
            }
            return parent.result;
        };
        std::visit(parent, *fields[ix]);
        result = parent.result;
        parent.ctx = tmp;
    }
    void ToCPSHelper::operator()(const AST::Tuple& e) {
        auto kappa = ctx;
        auto x = genvar();
        std::vector<variable> xs{};
        tuple_helper(e.fields, 0, x, xs, kappa, e.type);
    }
    void ToCPSHelper::operator()(const AST::Let& e) {
        std::visit(*this, *e.body);
        auto body = result;
        auto x = e.var;
        auto j = genvar();
        ctx = j;
        std::visit(*this, *e.val);
        auto in = result;
        result = let_cont(j, {x}, body, in);
    }

    term ast_to_cps(const AST::expr& e) {
        auto to_cps = ToCPS();
        std::visit(to_cps, *e);
        return to_cps.result;
    }

    void cps_to_sexp(std::ostream& os, const term& t) {
        ToSExp to_sexp(os);
        std::visit(to_sexp, *t);
    }

    term substitute(const term& t, const std::unordered_map<variable, variable>& mapping) {
        auto subst = Substitute(mapping);
        return  std::visit(subst, *t);
    }

    std::unordered_set<variable> used_symbols(const term& t) {
        auto used = UsedSymbols();
        std::visit(used, *t);
        return used.symbols;
    }

    term dead_let(const term& t) {
        auto tmp = t;
        for (;;) {
            auto live = used_symbols(tmp);
            auto dead = DeadLet(live);
            tmp = std::visit(dead, *tmp);
            if (dead.count == 0) return tmp;
        }
    }

    term beta_func(const term& t) {
        auto expanded = std::visit(BetaFunc{}, *t);
        auto used     = UsedSymbols(); std::visit(used, *expanded);
        auto cleaned  = std::visit(DeadLet(used.symbols), *expanded);
        return cleaned;
    }

    term beta_cont(const term& t) {
        auto expanded = std::visit(BetaCont{}, *t);
        auto used     = UsedSymbols(); std::visit(used, *expanded);
        auto cleaned  = std::visit(DeadLet(used.symbols), *expanded);
        return cleaned;
    }

    term prim_cse(const term& t) {
        auto cse = PrimCSE();
        std::visit(cse, *t);
        auto tmp = substitute(t, cse.replace);
        return dead_let(tmp);
    }

    void generate_cxx(std::ostream& os, const term& t) {
        auto gen = GenCXX();
        std::visit(gen, *t);
        for(const auto& line: gen.code) {
            os << line << '\n';
        }
    }
}
