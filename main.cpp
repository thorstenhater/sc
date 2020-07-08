#include "AST.hpp"
#include "Types.hpp"

void print_ast_sexp(AST::expr& e) {
    auto sexp = AST::ToSExp(std::cout);
    e->accept(sexp);
    try {
        auto types = Types::TypeCheck();
        e->accept(types);
        std::cout << "\n  : " << types.type_of(e)->show() << "\n";
    } catch (Types::TypeError& e) {
        std::cout << "\n  TypeError: " << e.what() << '\n';
    }
}

using namespace AST::convenience;

int main() {
    auto sum = (23.0_f64 + 42.0_f64);
    print_ast_sexp(sum);

    auto f1 = lambda({}, (23.0_f64 + 42.0_f64));
    print_ast_sexp(f1);

    auto f2 = lambda({"a"}, (23.0_f64 + 42.0_f64));
    print_ast_sexp(f2);

    auto fun = lambda({"a"}, ("a"_var + 42.0_f64));
    print_ast_sexp(fun);

    auto list = tuple({1.0_f64, 2.0_f64, 3.0_f64});
    print_ast_sexp(list);

    auto let1 = let("a", 2.0_f64, (23.0_f64 + "a"_var));
    print_ast_sexp(let1);

    auto let2 = let("a", 2.0_f64,
                    let("b", "a"_var,
                        (23.0_f64 + "b"_var)));
    print_ast_sexp(let2);

    auto ite = cond(no(), 2.0_f64, (3.0_f64 + 4.0_f64));
    print_ast_sexp(ite);

    auto test1 = apply(fun, {list});
    print_ast_sexp(test1);

    auto test2 = apply(sum, {list});
    print_ast_sexp(test2);

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
    print_ast_sexp(Ih_current);
}
