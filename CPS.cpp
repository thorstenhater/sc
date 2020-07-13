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
            term halt(const variable& v, const value& t) { return make_term<Halt>(v, t); }

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
        }

    }
}
