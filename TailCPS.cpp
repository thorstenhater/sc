#include "TailCPS.hpp"

namespace TailCPS {
    namespace convenience {
        template<typename E, typename... Ts> term make_term(const Ts&... args) { return std::make_shared<E>(E(args...)); }
        term let(const std::string& n, const value& v, const term& i) { return make_term<LetV>(n, v, i); }
        term pi(int f, const std::string& n, const std::string& t, const term& i) { return std::make_shared<LetT>(f, n, t, i); }
        term let_cont(const std::string& n, const std::vector<variable>& as, const term& b, const term& i) { return make_term<LetC>(n, as, b, i); }
        term let_func(const std::string& n, const std::string& c, const std::vector<variable>& as, const term& b, const term& i) { return make_term<LetF>(n, c, as, b, i); }
        term app_cont(const std::string& n, const std::string& a) { return make_term<AppC>(n, a); }
        term app_func(const std::string& n, const std::string& c, const std::string& a) { return make_term<AppF>(n, c, a); }
        term app_prim(const std::string& n, const std::string& c, const std::string& a) { return make_term<AppP>(n, c, a); }
        term halt(const variable& v) { return make_term<Halt>(v); }

        template<typename E, typename... Ts> value make_value(const Ts&... args) { return std::make_shared<E>(E(args...)); }
        value f64(double v) { return make_value<F64>(v); }
        value boolean(bool v) { return make_value<Bool>(v); }
        value tuple(const std::vector<variable>& fs) { return make_value<Tuple>(fs); }
    }

    std::string ToCPS::genvar() {
        return "__var_" + std::to_string(counter++);
    }

    std::string ToCPSHelper::genvar() {
        return parent.genvar();
    }

    void ToCPSHelper::visit(const AST::Var& e) {
        result = app_cont(ctx, e.name);
    }
    void ToCPSHelper::visit(const AST::F64& e) {
        auto x = genvar();
        result = let(x, f64(e.val), app_cont(ctx, x));
    }
    void ToCPSHelper::visit(const AST::Bool& e) {
        auto x = genvar();
        result = let(x, boolean(e.val), app_cont(ctx, x));
    }
    void ToCPSHelper::visit(const AST::Proj& e) {
        auto kappa = parent.ctx;
        auto x = genvar();
        auto k = ctx;
        parent.ctx = [&](const auto& z){
            parent.result = pi(e.field, x, z, app_cont(k, x));
            return parent.result;
        };
        e.tuple->accept(parent);
        result = parent.result;
        parent.ctx = kappa;
    }
    // NB. this is different (WRONG!) in Kennedy'07
    void ToCPSHelper::visit(const AST::Lam& e) {
        // recurse into body
        auto tmp = ctx;
        auto j = genvar();
        ctx = j;
        e.body->accept(*this);
        auto body = result;
        ctx = tmp;
        // generate result
        auto f = genvar();
        result = let_func(f, j, e.args, body, app_cont(ctx, f));
    }
    void ToCPSHelper::visit(const AST::App& e) {
        auto args = e.args;
        auto kappa = parent.ctx;
        parent.ctx = [=](const auto& x1) {
            auto tmp = parent.ctx;
            parent.ctx = [=](const auto& x2) {
                parent.result = app_func(x1, ctx, x2);
                return parent.result;
            };
            args->accept(parent);
            parent.ctx = tmp;
            return parent.result;
        };
        e.fun->accept(parent);
        result = parent.result;
        parent.ctx = kappa;
    }
    void ToCPSHelper::visit(const AST::Prim& e) {
        auto kappa = parent.ctx;
        parent.ctx = [&](auto x2) {
            parent.result = app_prim(e.op, ctx, x2);
            return parent.result;
        };
        e.args->accept(parent);
        result = parent.result;
        parent.ctx = kappa;
    }
     void ToCPSHelper::tuple_helper(const std::vector<AST::expr>& fields, size_t ix, const variable& x, std::vector<variable>& xs, const variable& kappa) {
        auto tmp = parent.ctx;
        parent.ctx = [&](auto z1) {
            xs.push_back(z1);
            if (ix + 1 == fields.size()) {
                parent.result = let(x, tuple(xs), app_cont(kappa, x));
            } else {
                tuple_helper(fields, ix + 1, x, xs, kappa);
            }
            return parent.result;
        };
        fields[ix]->accept(parent);
        result = parent.result;
        parent.ctx = tmp;
    }
    void ToCPSHelper::visit(const AST::Tuple& e) {
        auto kappa = ctx;
        auto x = genvar();
        std::vector<variable> xs{};
        tuple_helper(e.fields, 0, x, xs, kappa);
    }
    void ToCPSHelper::visit(const AST::Let& e) {
        auto x = e.var;
        auto j = genvar();

        e.body->accept(*this);
        auto body = result;

        ctx = j;
        e.val->accept(*this);
        auto in = result;
        result = let_cont(j, {x}, body, in);
    }
}
