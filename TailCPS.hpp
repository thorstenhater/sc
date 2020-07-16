#pragma once

#include <functional>
#include <vector>
#include <string>
#include <memory>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <unordered_map>
#include <exception>
#include <variant>

#include "AST.hpp"

using namespace std::string_literals;

namespace TailCPS {
    struct LetV;
    struct LetC;
    struct LetT;
    struct LetF;
    struct Halt;
    struct AppC;
    struct AppF;
    struct AppP;

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
                                  AppC,
                                  AppF,
                                  AppP,
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
        variable arg;
        AppF(const std::string& f, const std::string& c, const variable& a): name{f}, cont{c}, arg{a} {}
    };

    struct AppP {
        variable name;
        variable cont;
        variable arg;
        AppP(const std::string& f, const std::string& c, const variable& a): name{f}, cont{c}, arg{a} {}
    };

    namespace convenience {
        template<typename E, typename... Ts> term make_term(const Ts&... args);
        term let(const std::string& n, const value& v, const term& i);
        term pi(int f, const std::string& n, const std::string& t, const term& i);
        term let_cont(const std::string& n, const std::vector<variable>& as, const term& b, const term& i);
        term let_func(const std::string& n, const std::string& c, const std::vector<variable>& as, const term& b, const term& i);
        term app_cont(const std::string& n, const std::string& a);
        term app_func(const std::string& n, const std::string& c, const std::string& a);
        term app_prim(const std::string& n, const std::string& c, const std::string& a);
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
            os << "(let-cont ("
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
               << " "
               << e.arg << ")";
        }
        void operator()(const AppP& e) {
            os << "(apply-prim "
               << e.name
               << " "
               << e.cont
               << " "
               << e.arg
               << ")";
        }
        void operator()(const LetF& e) {
            os << "(let-func "
               << e.name
               << " "
               << e.cont
               << " (";
            for (const auto& arg: e.args) os << arg << " ";
            indent += 4;
            os << ")\n" << prefix << std::string(indent, ' ');
            std::visit(*this, *e.body);
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

        void tuple_helper(const std::vector<AST::expr>&, size_t, const variable&, std::vector<variable>&, const variable&);
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
        virtual void visit(const AST::App& e) override {
            auto kappa = ctx;
            auto k = genvar();
            auto x = genvar();
            ctx = [&](auto z1) {
                ctx = [&](auto z2) {
                    result = let_cont(k, {x}, kappa(x), app_func(z1, k, z2));
                    return result;
                };
                e.args->accept(*this);
                return result;
            };
            e.fun->accept(*this);
            ctx = kappa;
        }
        virtual void visit(const AST::Prim& e) override {
            auto kappa = ctx;
            ctx = [&](auto z2) {
                auto k = genvar();
                auto x = genvar();
                result = let_cont(k, {x}, kappa(x), app_prim(e.op, k, z2));
                return result;
            };
            e.args->accept(*this);
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
        virtual void visit(const AST::Let& e)   override {
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
            replace(tmp.arg);
            return std::make_shared<Term>(tmp);
        }
        term operator()(const AppP& e) {
            auto tmp = e;
            replace(tmp.cont);
            replace(tmp.arg);
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
            if (e.name == "__var_22") {
                auto body = std::visit(*this, *e.body);
                continuations[e.name] = {e.args, body};
                auto in = std::visit(*this, *e.in);
                return in;
            } else {
                auto tmp = e;
                return std::make_shared<Term>(tmp);
            }
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
                for (auto ix = 0; ix < args.size(); ix++) {
                    subst[args[ix]] = e.arg;
                }
                return std::make_shared<Term>(body);
            }
        }
        term operator()(const AppF& e) {
            auto tmp = e;
            return std::make_shared<Term>(tmp);
        }
        term operator()(const AppP& e) {
            auto tmp = e;
            return std::make_shared<Term>(tmp);
        }
        term operator()(const Halt& e) {
            auto tmp = e;
            return std::make_shared<Term>(tmp);
        }
    };

    term beta_cont(const term& t);
}
