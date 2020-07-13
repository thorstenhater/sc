#include "AST.hpp"
#include "Types.hpp"
#include "CPS.hpp"

using namespace AST::convenience;

void compile(const AST::expr& e) {
    auto sexp = AST::ToSExp(std::cout);
    e->accept(sexp);
    try {
        auto types = Types::TypeCheck();
        e->accept(types);
        std::cout << "\n  : " << types.type_of(e)->show() << "\n";
    } catch (Types::TypeError& e) {
        std::cout << "\n  TypeError: " << e.what() << '\n';
    }
    auto cps = CPS::naive::ToCPS().convert(e);
    auto cps_to_sexp = CPS::naive::ToSExp(std::cout);
    cps_to_sexp(cps);
    std::cout << '\n';
}

int main() {
    compile("a"_var);
    compile(42.0_f64);
    compile(let("a", 42.0_f64, "a"_var));
    compile(lambda({"a"}, "a"_var));
    compile(tuple({23.0_f64}));
    compile(tuple({23.0_f64, 42.0_f64}));
    compile(tuple({23.0_f64, 42.0_f64, 7.0_f64}));
    compile(add(23.0_f64, 42.0_f64));

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
    compile(let("a", 42.0_f64, let("b", "a"_var, "b"_var)));
}
