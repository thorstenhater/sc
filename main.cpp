#include "AST.hpp"
#include "Types.hpp"
#include "CPS.hpp"
#include "TailCPS.hpp"

using namespace AST::convenience;

TailCPS::term compile(const AST::expr& to_compile) {
    std::cout << "\n**************************************************\n";
    std::cout << "*** Type check ***********************************\n";
    AST::to_sexp(std::cout, to_compile);
    auto type = Types::typecheck(to_compile);
    try {
        std::cout << "\n  : " << Types::show_type(type);
    } catch (Types::TypeError& e) {
        std::cout << "\n  TypeError: " << e.what();
    }
    std::cout << "\n*** Alpha conversion *****************************\n";
    auto ast = AST::alpha_convert(to_compile);
    AST::to_sexp(std::cout, ast);
    std::cout << "\nCPS conversion: naive\n";
    auto naive_cps = CPS::ast_to_naive_cps(ast);
    CPS::cps_to_sexp(std::cout, naive_cps);
    std::cout << "\nCPS conversion: improved\n";
    auto cps = CPS::ast_to_cps(ast);
    CPS::cps_to_sexp(std::cout, cps);
    std::cout << "\n*** CPS conversion *******************************\n";
    auto tail_cps = TailCPS::ast_to_cps(ast);
    TailCPS::cps_to_sexp(std::cout, tail_cps);
    std::cout << "\n*** Beta expand continuations ********************\n";
    auto beta_cont = TailCPS::beta_cont(tail_cps);
    TailCPS::cps_to_sexp(std::cout, beta_cont);
    std::cout << "\n**************************************************\n";
    return tail_cps;
}

int main() {
    auto Ih_current =
        lambda({"sim", "mech"},
               pi("sim_v", 0, "sim"_var,
                  pi("sim_i", 1, "sim"_var,
                     pi("mech_m", 0, "mech"_var,
                        pi("mech_gbar", 1, "mech"_var,
                           pi("mech_ehcn", 2, "mech"_var,
                              let("i_new",
                                  ("sim_i"_var + (("mech_gbar"_var * "mech_m"_var) * ("sim_v"_var - "mech_ehcn"_var))),
                                  let("g_new",
                                      ("sim_g"_var + ("mech_gbar"_var * "mech_m"_var)),
                                      tuple({"i_new"_var, "g_new"_var})))))))));
    compile(Ih_current);
}
