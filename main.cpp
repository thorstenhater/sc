#include "AST.hpp"
#include "Types.hpp"
#include "TailCPS.hpp"

using namespace AST::convenience;

void compile(const AST::expr& to_compile) {
    std::cout << "\n**************************************************\n";
    std::cout << "*** Type check ***********************************\n";
    AST::to_sexp(std::cout, to_compile);
    auto type = AST::typecheck(to_compile);
    try {
        std::cout << "\n  : " << Types::show_type(type);
    } catch (Types::TypeError& e) {
        std::cout << "\n  TypeError: " << e.what();
    }
    std::cout << "\n*** Alpha conversion *****************************\n";
    auto ast = AST::alpha_convert(to_compile);
    AST::to_sexp(std::cout, ast);
    std::cout << "\n*** CPS conversion *******************************\n";
    auto tail_cps = TailCPS::ast_to_cps(ast);
    TailCPS::cps_to_sexp(std::cout, tail_cps);
    std::cout << "\n*** Beta expand continuations ********************\n";
    auto beta_cont = TailCPS::beta_cont(tail_cps);
    TailCPS::cps_to_sexp(std::cout, beta_cont);
    std::cout << "\n*** Dead continuations ***************************\n";
    std::cout << "Live:\n";
    auto live_after_beta_cont = used_symbols(beta_cont);
    for (const auto& cont: live_after_beta_cont) std::cout << "  - " << cont << '\n';
    std::cout << "\n";
    auto dead_conts = dead_let(live_after_beta_cont, beta_cont);
    TailCPS::cps_to_sexp(std::cout, dead_conts);
    std::cout << "\n*** Beta expand functions ********************\n";
    auto beta_func = TailCPS::beta_func(dead_conts);
    TailCPS::cps_to_sexp(std::cout, beta_func);
    std::cout << "\n*** Dead functions ****************************\n";
    std::cout << "Live:\n";
    auto live_funcs = used_symbols(beta_func);
    for (const auto& func: live_funcs) std::cout << "  - " << func << '\n';
    std::cout << "\n";
    auto dead_funcs = dead_let(live_funcs, beta_func);
    TailCPS::cps_to_sexp(std::cout, dead_funcs);
    std::cout << "\n*** PrimOp CSE ***********************************\n";
    auto after_prim_cse = prim_cse(dead_funcs);
    TailCPS::cps_to_sexp(std::cout, after_prim_cse);
    std::cout << "\n*** Generate CXX *********************************\n";
    generate_cxx(std::cout, after_prim_cse);
    std::cout << "\n**************************************************\n";
}

int main() {
    auto Ih_current =
        lambda({"sim", "mech"},
               pi("sim_v", 0, "sim"_var,
                  pi("sim_i", 1, "sim"_var,
                     pi("sim_g", 2, "sim"_var,
                        pi("mech_m", 0, "mech"_var,
                           pi("mech_gbar", 1, "mech"_var,
                              pi("mech_ehcn", 2, "mech"_var,
                                 let("i_new",
                                     ("sim_i"_var + (("mech_gbar"_var * "mech_m"_var) * ("sim_v"_var - "mech_ehcn"_var))),
                                     let("g_new",
                                         ("sim_g"_var + ("mech_gbar"_var * "mech_m"_var)),
                                         tuple({"i_new"_var, "g_new"_var}))))))))));
    compile(Ih_current);
    compile(let("f", lambda({"x"}, "x"_var + "x"_var), apply("f"_var, {42.0_f64})));
    compile(project(2, "tuple"_var));
    compile(1.0_f64 + 2.0_f64);
    compile(let("a", 42.0_f64, let("b", "a"_var, "b"_var)));
    compile(let("f", lambda({"y"}, "y"_var + "y"_var), let("a", apply("f"_var, {1.0_f64}), "a"_var)));
}
