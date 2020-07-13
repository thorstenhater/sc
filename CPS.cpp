#include "CPS.hpp"

namespace CPS {
    namespace naive {
        namespace convenience {
            template<typename E, typename... Ts> term make_term(const Ts&... args) { return std::make_shared<E>(E(args...)); }
            term let(const std::string& n, const value& v, const term& i) { return make_term<LetV>(n, v, i); }
            term pi(int f, const std::string& n, const std::string& t, const term& i) { return std::make_shared<LetT>(f, n, t, i); }
            term let_cont(const std::string& n, const std::vector<variable>& as, const term& b, const term& i) { return make_term<LetC>(n, as, b, i); }
            term app_cont(const std::string& n, const std::string& a) { return make_term<AppC>(n, a); }
            term app_func(const std::string& n, const std::string& c, const std::string& a) { return make_term<AppF>(n, c, a); }
            term app_prim(const std::string& n, const std::string& c, const std::string& a) { return make_term<AppP>(n, c, a); }
            term halt(const variable& v) { return make_term<Halt>(v); }

            template<typename E, typename... Ts> value make_value(const Ts&... args) { return std::make_shared<E>(E(args...)); }
            value lambda(const std::string& c, const std::vector<variable>& as, const term& b) { return make_value<Lambda>(c, as, b); }
            value f64(double v) { return make_value<F64>(v); }
            value boolean(bool v) { return make_value<Bool>(v); }
            value tuple(const std::vector<variable>& fs) { return make_value<Tuple>(fs); }
        }

        std::string genvar() {
            static int counter = 0;
            return "__var_" + std::to_string(counter++);
        }

        void ToCPSImprovedHelper::visit(const AST::Proj& e) {
            auto kappa = parent.ctx;
            auto x = genvar();
            parent.ctx = [&](auto z) {
                return pi(e.field, x, z, app_cont(ctx, x));
            };
            e.tuple->accept(parent);
            result = parent.result;
            parent.ctx = kappa;
        }
        void ToCPSImprovedHelper::visit(const AST::Var& e) {
            result = app_cont(ctx, e.name);
        }
        void ToCPSImprovedHelper::visit(const AST::App& e) {
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
        void ToCPSImprovedHelper::visit(const AST::Prim& e) {
            auto kappa = parent.ctx;
            parent.ctx = [&](auto x2) {
                parent.result = app_prim(e.op, ctx, x2);
                return parent.result;
            };
            e.args->accept(parent);
            result = parent.result;
            parent.ctx = kappa;
        }
        void ToCPSImprovedHelper::visit(const AST::F64& e) {
            auto x = genvar();
            result = let(x, f64(e.val), app_cont(ctx, x));
        }
        void ToCPSImprovedHelper::visit(const AST::Bool& e) {
            auto x = genvar();
            result = let(x, boolean(e.val), app_cont(ctx, x));
        }
        void ToCPSImprovedHelper::visit(const AST::Let& e) {
            auto x = e.var;
            auto j = genvar();

            e.body->accept(*this);
            auto body = result;

            ctx = j;
            e.val->accept(*this);
            auto in = result;
            result = let_cont(j, {x}, body, in);
        }
        void ToCPSImprovedHelper::visit(const AST::Lam& e) {
            auto f = genvar();
            auto j = genvar();
            auto x = e.args;
            auto tmp = ctx;
            ctx = j;
            auto body = result;
            result = let(f, lambda(j, x, body), app_cont(ctx, f));
            ctx = tmp;
        }

        void ToCPSImprovedHelper::tuple_helper(const std::vector<AST::expr>& fields, size_t ix, const variable& x, std::vector<variable>& xs, const variable& kappa) {
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
        void ToCPSImprovedHelper::visit(const AST::Tuple& e) {
            auto kappa = ctx;
            auto x = genvar();
            std::vector<variable> xs{};
            tuple_helper(e.fields, 0, x, xs, kappa);
        }
    }
}
