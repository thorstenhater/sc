#pragma once

#include <functional>
#include <vector>
#include <string>
#include <memory>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <exception>
#include <variant>

#include "AST.hpp"

using namespace std::string_literals;

namespace TailCPS {
    struct LetV;
    struct LetC;
    struct LetT;
    struct LetF;
    struct LetP;
    struct Halt;
    struct AppC;
    struct AppF;

    struct Tuple;
    struct F64;
    struct Bool;

    // Variables
    using variable = std::string;
    using Value    = std::variant<Tuple,
                                  F64,
                                  Bool>;
    using value    = std::shared_ptr<Value>;
    using Term     = std::variant<LetV,
                                  LetC,
                                  LetT,
                                  LetF,
                                  LetP,
                                  AppF,
                                  AppC,
                                  Halt>;
    using term     = std::shared_ptr<Term>;

    struct Tuple  {
        std::vector<variable> fields;
        Tuple(const std::vector<std::string>& fs): fields{fs} {}
    };

    struct F64 {
        double value;
        F64(double v): value{v} { }
    };

    struct Bool {
        bool value;
        Bool(bool v): value{v} { }
    };

    struct Halt {
        variable name;
        Halt(const std::string&n): name{n} {}
    };

    struct LetV {
        variable name;
        term in;
        value val;
        LetV(const std::string& n, const value& v, const term& i): name{n}, in{i}, val{v} {}
    };

    // Tuple projection
    struct LetT {
        variable name;
        term in;
        int field;
        variable tuple;
        virtual ~LetT() = default;
        LetT(int f, const std::string& n, const std::string& t, const term& i): name{n}, in{i}, field{f}, tuple{t} {}
    };

    struct LetF {
        variable name;
        term in;
        variable cont;
        std::vector<variable> args;
        term body;
        LetF(const std::string& n, const std::string& c, const std::vector<variable>& as, const term& b, const term& i): name{n}, in{i}, cont{c}, args{as}, body{b} {}
    };

    // Local continuation
    struct LetC {
        variable name;
        term in;
        std::vector<variable> args;
        term body;
        LetC(const std::string& n, const std::vector<variable>& as, const term& b, const term& i): name{n}, in{i}, args{as}, body{b} {}
    };

    // Apply Continuation c a0 a1 ...
    struct AppC {
        variable name;
        variable arg;
        AppC(const std::string& n, const variable& a): name{n}, arg{a} {}
    };

    // Apply function f c a0 a1 ...
    struct AppF {
        variable name;
        variable cont;
        std::vector<variable> args;
        AppF(const std::string& f, const std::string& c, const std::vector<variable>& as): name{f}, cont{c}, args{as} {}
    };

    struct LetP {
        variable name;
        variable var;
        std::vector<variable> args;
        term in;
        LetP(const std::string& f, const std::string& v, const std::vector<variable>& a, const term& i): name{f}, var{v}, args{a}, in{i} {}
    };

    namespace convenience {
        template<typename E, typename... Ts> term make_term(const Ts&... args);
        term let(const std::string& n, const value& v, const term& i);
        term pi(int f, const std::string& n, const std::string& t, const term& i);
        term let_cont(const std::string& n, const std::vector<variable>& as, const term& b, const term& i);
        term let_func(const std::string& n, const std::string& c, const std::vector<variable>& as, const term& b, const term& i);
        term let_prim(const std::string&, const std::string&, const std::vector<std::string>&, const term&);
        term app_cont(const std::string& n, const std::string& a);
        term app_func(const std::string& n, const std::string& c, const std::vector<std::string>& a);
        term halt(const variable& v);

        template<typename E, typename... Ts> value make_value(const Ts&... args);
        value f64(double v);
        value boolean(bool v);
        value tuple(const std::vector<variable>& fs);
    }

    using namespace convenience;

    struct ToSExp {
        std::ostream& os;
        int indent;
        std::string prefix = "";

        ToSExp(std::ostream& os_, int i=0, const std::string& p=""):
            os{os_}, indent{i}, prefix{p} {
            os << prefix << std::string(indent, ' ');
        }

        void operator()(const LetV& e) {
            os << "(let-value ("
               << e.name
               << " ";
            std::visit(*this, *e.val);
            indent += 4;
            os << ")\n" << prefix << std::string(indent, ' ');
            std::visit(*this, *e.in);
            os << ")";
            indent -= 4;
        }
        void operator()(const LetC& e) {
            os << "(let-cont "
               << e.name
               << " (";
            for(const auto& arg: e.args) os << arg << " ";
            indent += 4;
            os << ")\n" << prefix << std::string(indent, ' ');
            std::visit(*this, *e.body);
            os << "\n" << prefix << std::string(indent, ' ');
            std::visit(*this, *e.in);
            os << ")";
            indent -= 4;
        }
        void operator()(const LetT& e) {
            os << "(pi-"
               << e.field
               << " "
               << e.name
               << " "
               << e.tuple;
            indent += 4;
            os << ")\n" << prefix << std::string(indent, ' ');
            std::visit(*this, *e.in);
            os << ")";
            indent -= 4;
        }
        void operator()(const Halt& e) {
            os << "(halt "
               << e.name
               << ")";
        }
        void operator()(const AppC& e) {
            os << "(apply-cont "
               << e.name
               << " "
               << e.arg
               << ")";
        }
        void operator()(const AppF& e) {
            os << "(apply-func "
               << e.name
               << " "
               << e.cont
               << " ";
            for (const auto& arg: e.args) os << arg << " ";
            os << ")";
        }
        void operator()(const LetP& e) {
            os << "(let-prim "
               << e.var
               << " ("
               << e.name
               << " ";
            for (const auto& arg: e.args) os << arg << " ";
            indent += 4;
            os << ")\n" << prefix << std::string(indent, ' ');
            std::visit(*this, *e.in);
            os << ")";
            indent -= 4;
        }
        void operator()(const LetF& e) {
            os << "(let-func "
               << e.name
               << " "
               << e.cont
               << " (";
            for (const auto& arg: e.args) os << arg << " ";
            indent += 4;
            os << ")\n" << prefix << std::string(indent, ' ') << ";; function";
            os << "\n" << prefix << std::string(indent, ' ');
            std::visit(*this, *e.body);
            os << "\n" << prefix << std::string(indent, ' ') << ";; in";
            os << "\n" << prefix << std::string(indent, ' ');
            std::visit(*this, *e.in);
            os << ")";

            indent -= 4;
        };
        void operator()(const Tuple& v) {
            os << "(";
            for (const auto& field: v.fields) os << field << ", ";
            os << ")";
        };
        void operator()(const F64& v) {
            os << v.value;
        };
        void operator()(const Bool& v) {
            os << v.value;
        };
    };

    struct ToCPS;

    struct ToCPSHelper: AST::Visitor {
        ToCPS& parent;
        term result;
        variable ctx;
        ToCPSHelper(ToCPS&p): parent{p} {}

        std::string genvar();

        virtual void visit(const AST::F64&)   override;
        virtual void visit(const AST::Bool&)  override;
        virtual void visit(const AST::Let&)   override;
        virtual void visit(const AST::Var&)   override;
        virtual void visit(const AST::Proj&)  override;
        virtual void visit(const AST::App&)   override;
        virtual void visit(const AST::Lam&)   override;
        virtual void visit(const AST::Tuple&) override;
        virtual void visit(const AST::Prim&)  override;

        virtual void visit(const AST::Cond&)  override {}

        void app_helper(const std::vector<AST::expr>&, size_t, const variable&, std::vector<variable>&, const variable&, const variable&);
        void tuple_helper(const std::vector<AST::expr>&, size_t, const variable&, std::vector<variable>&, const variable&);
        void prim_helper(const std::vector<AST::expr>& args, size_t ix, std::vector<variable>& ys, const variable& op, const variable& kappa);
    };

    struct ToCPS: AST::Visitor {
        size_t counter;
        ToCPSHelper helper;

        ToCPS(): counter{0}, helper{*this} {}

        using context = std::function<term(variable)>;
        context ctx = [&](auto v) { return halt(v); };

        term result;

        std::string genvar();

        term convert(const AST::expr& e) {
            e->accept(*this);
            return result;
        }

        virtual void visit(const AST::Var& e) override {
            result = ctx(e.name);
        }
        template<typename K>
        void app_helper(const std::vector<AST::expr>& args, size_t ix, const variable& zs, std::vector<variable>& ys, const variable& f, K& kappa) {
            auto tmp = ctx;
            ctx = [&](auto y) {
                ys.push_back(y);
                if (ix + 1 == args.size()) {
                    auto k = genvar();
                    result = let_cont(k, {zs}, kappa(zs),
                                      app_func(f, k, ys));
                } else {
                    app_helper(args, ix + 1, zs, ys, f, kappa);
                }
                return result;
            };
            args[ix]->accept(*this);
            ctx = tmp;
        }
        virtual void visit(const AST::App& e) override {
            auto kappa = ctx;
            auto zs = genvar();
            std::vector<variable> ys{};
            ctx = [&](auto y) {
                app_helper(e.args, 0, zs, ys, y, kappa);
                return result;
            };
            e.fun->accept(*this);
            ctx = kappa;
        }

        template<typename K>
        void prim_helper(const std::vector<AST::expr>& args, size_t ix, std::vector<variable>& ys, const variable& op, K& kappa) {
            auto tmp = ctx;
            ctx = [&](auto y) {
                ys.push_back(y);
                if (ix + 1 == args.size()) {
                    auto n = genvar();
                    result = let_prim(op, n, ys, kappa(n));
                } else {
                    prim_helper(args, ix + 1, ys, op, kappa);
                }
                return result;
            };
            args[ix]->accept(*this);
            ctx = tmp;
        }
        virtual void visit(const AST::Prim& e) override {
            std::cout << "Visiting PrimOp " << e.op << "\n";
            auto kappa = ctx;
            std::vector<variable> ys = {};
            prim_helper(e.args, 0, ys, e.op, kappa);
            ctx = kappa;
        }
        virtual void visit(const AST::Lam& e) override {
            auto f = genvar();
            auto k = genvar();
            auto x = e.args;
            helper.ctx = k;
            e.body->accept(helper);
            auto body = helper.result;
            result = let_func(f, k, x, body, ctx(f));
        }
        template<typename K>
        void tuple_helper(const std::vector<AST::expr>& fields, size_t ix, const variable& x, std::vector<variable>& xs, K& kappa) {
            auto tmp = ctx;
            ctx = [&](auto z1) {
                xs.push_back(z1);
                if (ix + 1 == fields.size()) {
                    result = let(x, tuple(xs), kappa(x));
                } else {
                    tuple_helper(fields, ix + 1, x, xs, kappa);
                }
                return result;
            };
            fields[ix]->accept(*this);
            ctx = tmp;
        }
        virtual void visit(const AST::Tuple& e) override {
            auto kappa = ctx;
            auto x = genvar();
            std::vector<variable> xs{};
            tuple_helper(e.fields, 0, x, xs, kappa);
        }
        virtual void visit(const AST::F64& e) override {
            auto x = genvar();
            result = let(x, f64(e.val), ctx(x));
        }
        virtual void visit(const AST::Bool& e)  override {
            auto x = genvar();
            result = let(x, boolean(e.val), ctx(x));
        }
        virtual void visit(const AST::Proj& e)  override {
            auto kappa = ctx;
            auto x = genvar();
            ctx = [&](auto z) { return pi(e.field, x, z, kappa(x)); };
            e.tuple->accept(*this);
            ctx = kappa;
        }
        virtual void visit(const AST::Let& e) override {
            e.body->accept(*this);
            auto cont = result;
            auto j = genvar();
            helper.ctx = j;
            e.val->accept(helper);
            auto body = helper.result;
            result = let_cont(j, {e.var}, cont, body);
        }
        virtual void visit(const AST::Cond&)  override {}
    };

    term ast_to_cps(const AST::expr&);
    void cps_to_sexp(std::ostream&, const term&);

    struct Substitute {
        std::unordered_map<std::string, std::string> mapping;
        Substitute(const std::unordered_map<std::string, std::string>& m): mapping{m} {}

        term operator()(const LetV& e) {
            auto tmp = e;
            if (std::holds_alternative<Tuple>(*e.val)) {
                auto& val = std::get<Tuple>(*e.val);
                for (auto& field: val.fields) {
                    replace(field);
                }
            }
            tmp.in = std::visit(*this, *tmp.in);
            return std::make_shared<Term>(tmp);
        }
        term operator()(const LetC& e) {
            auto tmp = e;
            tmp.in   = std::visit(*this, *tmp.in);
            tmp.body = std::visit(*this, *tmp.body);
            return std::make_shared<Term>(tmp);
        }
        term operator()(const LetT& e) {
            auto tmp = e;
            replace(tmp.tuple);
            tmp.in = std::visit(*this, *tmp.in);
            return std::make_shared<Term>(tmp);
        }
        term operator()(const LetF& e) {
            auto tmp = e;
            tmp.in   = std::visit(*this, *tmp.in);
            tmp.body = std::visit(*this, *tmp.body);
            return std::make_shared<Term>(tmp);
        }
        term operator()(const AppC& e) {
            auto tmp = e;
            replace(tmp.name);
            replace(tmp.arg);
            return std::make_shared<Term>(tmp);
        }
        term operator()(const AppF& e) {
            auto tmp = e;
            replace(tmp.name);
            replace(tmp.cont);
            for (auto& arg: tmp.args) replace(arg);
            return std::make_shared<Term>(tmp);
        }
        term operator()(const LetP& e) {
            auto tmp = e;
            for (auto& arg: tmp.args) replace(arg);
            tmp.in = std::visit(*this, *tmp.in);
            return std::make_shared<Term>(tmp);
        }
        term operator()(const Halt& e) {
            auto tmp = e;
            replace(tmp.name);
            return std::make_shared<Term>(tmp);
        }

        void replace(std::string& name) {
            if (mapping.find(name) != mapping.end()) {
                name = mapping[name];
            }
        }
    };

    term substitute(const term& t, const std::unordered_map<variable, variable>& mapping);

    struct BetaCont {
        std::unordered_map<variable, std::pair<std::vector<variable>, term>> continuations;

        term operator()(const LetV& e) {
            auto tmp = e;
            tmp.in = std::visit(*this, *tmp.in);
            return std::make_shared<Term>(tmp);
        }
        term operator()(const LetC& e) {
            auto tmp = e;
            auto body = std::visit(*this, *e.body);
            continuations[e.name] = {e.args, body};
            auto in = std::visit(*this, *e.in);
            tmp.body = body;
            tmp.in = in;
            return std::make_shared<Term>(tmp);
        }
        term operator()(const LetT& e) {
            auto tmp = e;
            tmp.in = std::visit(*this, *tmp.in);
            return std::make_shared<Term>(tmp);
        }
        term operator()(const LetF& e) {
            auto tmp = e;
            tmp.in   = std::visit(*this, *tmp.in);
            tmp.body = std::visit(*this, *tmp.body);
            return std::make_shared<Term>(tmp);
        }
        term operator()(const AppC& e) {
            if (continuations.find(e.name) == continuations.end()) {
                return std::make_shared<Term>(e);
            } else {
                auto args =  continuations[e.name].first;
                auto body = *continuations[e.name].second;
                auto subst = std::unordered_map<variable, variable>{};
                if (args.size() > 1) {
                    std::cout << "Please fix continuations to have more than one argument" << '\n';
                    exit(42);
                }
                std::cout << "Substituting in " << e.name << ": " << args[0] << " -> " << e.arg << '\n';
                subst[args[0]] = e.arg;
                auto res = std::make_shared<Term>(body);
                return substitute(res, subst);
            }
        }
        term operator()(const AppF& e) {
            auto tmp = e;
            return std::make_shared<Term>(tmp);
        }
        term operator()(const LetP& e) {
            auto tmp = e;
            tmp.in = std::visit(*this, *tmp.in);
            return std::make_shared<Term>(tmp);
        }
        term operator()(const Halt& e) {
            auto tmp = e;
            return std::make_shared<Term>(tmp);
        }
    };

    term beta_cont(const term& t);

    struct UsedSymbols {
        std::unordered_set<variable> symbols;

        void operator()(const LetV& e) {
            if (std::holds_alternative<Tuple>(*e.val)) {
                auto tuple = std::get<Tuple>(*e.val);
                for (const auto& field: tuple.fields) {
                    symbols.insert(field);
                }
            }
            std::visit(*this, *e.in);
        }
        void operator()(const LetC& e) {
            std::visit(*this, *e.body);
            std::visit(*this, *e.in);
        }
        void operator()(const LetT& e) {
            symbols.insert(e.tuple);
            std::visit(*this, *e.in);
        }
        void operator()(const LetF& e) {
            std::visit(*this, *e.in);
            std::visit(*this, *e.body);
        }
        void operator()(const AppC& e) {
            symbols.insert(e.name);
            symbols.insert(e.arg);
        }
        void operator()(const AppF& e) {
            symbols.insert(e.name);
            symbols.insert(e.cont);
            for (const auto& arg: e.args) {
                symbols.insert(arg);
            }
        }
        void operator()(const LetP& e) {
            for (const auto& arg: e.args) {
                symbols.insert(arg);
            }
            std::visit(*this, *e.in);
        }
        void operator()(const Halt& e) {
            symbols.insert(e.name);
        }
    };

    std::unordered_set<variable> used_symbols(const term& t);

    struct DeadLet {
        std::unordered_set<variable> live;
        DeadLet(std::unordered_set<variable> l): live{l} {}

        term operator()(const LetV& e) {
            auto tmp = e;
            tmp.in = std::visit(*this, *e.in);
            if (live.find(e.name) != live.end()) {
                return std::make_shared<Term>(tmp);
            } else {
                return tmp.in;
            }
        }
        term operator()(const LetC& e) {
            auto tmp = e;
            tmp.body = std::visit(*this, *e.body);
            tmp.in   = std::visit(*this, *e.in);
            if (live.find(e.name) != live.end()) {
                return std::make_shared<Term>(tmp);
            } else {
                return tmp.in;
            }
        }
        term operator()(const LetT& e) {
            auto tmp = e;
            tmp.in = std::visit(*this, *e.in);
            if (live.find(e.name) != live.end()) {
                return std::make_shared<Term>(tmp);
            } else {
                return tmp.in;
            }
        }
        term operator()(const LetP& e) {
            auto tmp = e;
            tmp.in = std::visit(*this, *e.in);
            if (live.find(e.var) != live.end()) {
                return std::make_shared<Term>(tmp);
            } else {
                std::cerr << "Dead prim " << e.name << '\n';
                return tmp.in;
            }
        }
        term operator()(const LetF& e) {
            auto tmp = e;
            tmp.body = std::visit(*this, *e.body);
            tmp.in   = std::visit(*this, *e.in);
            if (live.find(e.name) != live.end()) {
                return std::make_shared<Term>(tmp);
            } else {
                return tmp.in;
            }
        }
        term operator()(const AppC& e) { return std::make_shared<Term>(e); }
        term operator()(const AppF& e) { return std::make_shared<Term>(e); }
        term operator()(const Halt& e) { return std::make_shared<Term>(e); }
    };

    term dead_let(std::unordered_set<variable> live, const term& t);

    struct BetaFunc {
        std::unordered_map<variable, std::pair<std::vector<variable>, term>> functions;

        term operator()(const LetV& e) {
            auto tmp = e;
            tmp.in = std::visit(*this, *tmp.in);
            return std::make_shared<Term>(tmp);
        }
        term operator()(const LetC& e) {
            auto tmp = e;
            tmp.body = std::visit(*this, *e.body);
            tmp.in   = std::visit(*this, *e.in);
            return std::make_shared<Term>(tmp);
        }
        term operator()(const LetT& e) {
            auto tmp = e;
            tmp.in = std::visit(*this, *tmp.in);
            return std::make_shared<Term>(tmp);
        }
        term operator()(const LetP& e) {
            auto tmp = e;
            tmp.in = std::visit(*this, *tmp.in);
            return std::make_shared<Term>(tmp);
        }
        term operator()(const LetF& e) {
            auto tmp = e;
            tmp.body = std::visit(*this, *tmp.body);
            functions[e.name] = {tmp.args, tmp.body};
            tmp.in   = std::visit(*this, *tmp.in);
            return std::make_shared<Term>(tmp);
        }
        term operator()(const AppC& e) {
            auto tmp = e;
            return std::make_shared<Term>(tmp);
        }
        term operator()(const AppF& e) {
            if (functions.find(e.name) == functions.end()) {
                return std::make_shared<Term>(e);
            } else {
                std::cout << "  + " << e.name  << '\n';
                auto args =  functions[e.name].first;
                auto body = *functions[e.name].second;
                auto subst = std::unordered_map<variable, variable>{};
                for (auto ix = 0; ix < args.size(); ix++) {
                    subst[args[ix]] = e.args[ix];
                    std::cout << "    - " << args[ix] << " -> " << e.args[ix] << '\n';
                }
                auto res = std::make_shared<Term>(body);
                return substitute(res, subst);
            }
        }
        term operator()(const Halt& e) {
            auto tmp = e;
            return std::make_shared<Term>(tmp);
        }
    };

    term beta_func(const term& t);

    struct PrimCSE {
        std::unordered_map<std::string, variable> seen;
        std::unordered_map<variable, variable> replace;

        void operator()(const LetV& e) {
            std::visit(*this, *e.in);
        }
        void operator()(const LetC& e) {
            std::visit(*this, *e.body);
            std::visit(*this, *e.in);
        }
        void operator()(const LetT& e) {
            std::visit(*this, *e.in);
        }
        void operator()(const LetP& e) {
            std::string key = e.name;
            for (const auto& arg: e.args) { key += ":" + arg; }
            if (seen.find(key) != seen.end()) {
                std::cout << "Found prim op with key: " << key << " -> " << seen[key] << '\n';
                replace[e.var] = seen[key];
            } else {
                std::cout << "Adding prim op with key: " << key << " -> " << seen[key] << '\n';
                seen[key] = e.var;
            }
            std::visit(*this, *e.in);

        }
        void operator()(const LetF& e) {
            std::visit(*this, *e.body);
            std::visit(*this, *e.in);
        }
        void operator()(const AppC& e) {}
        void operator()(const AppF& e) {}
        void operator()(const Halt& e) {}
    };

    term prim_cse(const term& t);

    struct GenCXX {
        std::string ret = "";
        std::vector<std::string> code;
        void operator()(const LetV& e) {
            auto value = std::visit(*this, *e.val);
            std::string line = "const auto " + e.name + " = " + value + ";";
            code.push_back(line);
            std::visit(*this, *e.in);
        }
        void operator()(const LetC& e) {
            code.push_back("// def continuation " + e.name);
            std::visit(*this, *e.body);
            std::visit(*this, *e.in);
        }
        void operator()(const LetT& e) {
            std::string line = "const auto " + e.name + " = std::get<" + std::to_string(e.field) + ">(" + e.tuple + ");";
            code.push_back(line);
            std::visit(*this, *e.in);
        }
        void operator()(const LetP& e) {
            std::string line = "const auto " + e.var + " = ";
            if ((e.name == "+") || (e.name == "-") || (e.name == "*")) {
                line += e.args[0] + " " + e.name + " " + e.args[1] + ";";
            }
            code.push_back(line);
            std::visit(*this, *e.in);
        }
        void operator()(const LetF& e) {
            auto tmp = ret;
            ret = e.cont;
            std::string line = "auto " + e.name + "(";
            for (const auto& arg: e.args) line += "const auto& " + arg + ", ";
            if (line.back() == ' ') {
                line.pop_back();
                line.pop_back();
            }
            line += ") {";
            code.push_back(line);
            std::visit(*this, *e.body);
            code.push_back("}");
            ret = tmp;
            std::visit(*this, *e.in);
        }
        void operator()(const AppC& e) {
            if (e.name == ret) {
                code.push_back("return " + e.arg + ";");
                ret = "";
            } else {
                code.push_back("// continuation " + e.name);
            }
        }
        void operator()(const AppF& e) {
            code.push_back("// function " + e.name);
        }
        void operator()(const Halt& e) {
            code.push_back("// HALT " + e.name);
        }
        std::string operator()(const F64& v) {
            return std::to_string(v.value);
        }
        std::string operator()(const Bool& v) {
            return std::to_string(v.value);
        }
        std::string operator()(const Tuple& v) {
            std::string tup = "std::tuple<";
            std::string res = "{";
            for (const auto& field: v.fields) {
                res += field + ", ";
                tup += "auto"s + ", ";
            }
            if (tup.back() == ' ') {
                tup.pop_back();
                tup.pop_back();
            }
            tup += ">";
            res += "}";
            return tup + res;
        }
    };

    void generate_cxx(std::ostream&, const term&);
}
