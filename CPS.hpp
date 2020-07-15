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

#include "AST.hpp"

using namespace std::string_literals;

namespace CPS {
    struct LetV;
    struct LetC;
    struct LetT;
    struct Halt;
    struct AppC;
    struct AppF;
    struct AppP;

    struct Tuple;
    struct F64;
    struct Lambda;
    struct Bool;

    struct Visitor {
        virtual void visitT(const LetV&) {}
        virtual void visitT(const LetC&) {}
        virtual void visitT(const LetT&) {}
        virtual void visitT(const Halt&) {}
        virtual void visitT(const AppC&) {}
        virtual void visitT(const AppF&) {}
        virtual void visitT(const AppP&) {}

        virtual void visitV(const Tuple&) {}
        virtual void visitV(const F64&) {}
        virtual void visitV(const Bool&) {}
        virtual void visitV(const Lambda&) {}
    };

    // Variables
    using variable = std::string;

    // Values
    struct Value {
        virtual void accept(Visitor&) const {};
    };

    using value = std::shared_ptr<Value>;

    // Terms
    struct Term {
        virtual void accept(Visitor&) const {};
    };

    using term = std::shared_ptr<Term>;

    struct Tuple: Value  {
        std::vector<variable> fields;
        virtual ~Tuple() = default;
        Tuple(const std::vector<std::string>& fs): fields{fs} {}
        virtual void accept(Visitor& v) const { v.visitV(*this); };
    };

    struct F64: Value {
        double value;
        virtual ~F64() = default;
        F64(double v): value{v} { }
        virtual void accept(Visitor& v) const { v.visitV(*this); };
    };


    struct Bool: Value {
        bool value;
        Bool(bool v): value{v} { }
        virtual ~Bool() = default;
        virtual void accept(Visitor& v) const { v.visitV(*this); };
    };

    struct Lambda: Value {
        variable cont;
        std::vector<variable> args;
        term body;
        virtual ~Lambda() = default;
        Lambda(const std::string& c, const std::vector<std::string>& as, const term& b): cont{c}, args{as}, body{b} {}
        virtual void accept(Visitor& v) const { v.visitV(*this); };
    };

    struct Halt: Term {
        virtual ~Halt() = default;
        variable name;
        Halt(const std::string&n): name{n} {}
        virtual void accept(Visitor& v) const override { v.visitT(*this); };
    };

    // Bindings
    struct Let: Term {
        variable name;
        term in;
        virtual ~Let() = default;
        Let(const std::string& n, const term& i): name{n}, in{i} {}
    };

    struct LetV: Let {
        value val;
        LetV(const std::string& n, const value& v, const term& i): Let{n, i}, val{v} {}
        virtual void accept(Visitor& v) const override { v.visitT(*this); };
    };

    // Tuple projection
    struct LetT: Let {
        int field;
        variable tuple;
        virtual ~LetT() = default;
        LetT(int f, const std::string& n, const std::string& t, const term& i): Let{n, i}, field{f}, tuple{t} {}
        virtual void accept(Visitor& v) const override { v.visitT(*this); };
    };

    // Local continuation
    struct LetC: Let {
        std::vector<variable> args;
        term body;
        virtual ~LetC() = default;
        LetC(const std::string& n, const std::vector<variable>& as, const term& b, const term& i): Let{n, i}, args{as}, body{b} {}
        virtual void accept(Visitor& v) const override { v.visitT(*this); };
    };

    // Apply Continuation c a0 a1 ...
    struct AppC: Term {
        variable name;
        variable arg;
        virtual ~AppC() = default;
        AppC(const std::string& n, const variable& a): name{n}, arg{a} {}
        virtual void accept(Visitor& v) const override { v.visitT(*this); };
    };

    // Apply function f c a0 a1 ...
    struct AppF: Term {
        variable func;
        variable cont;
        variable arg;
        virtual ~AppF() = default;
        AppF(const std::string& f, const std::string& c, const variable& a): func{f}, cont{c}, arg{a} {}
        virtual void accept(Visitor& v) const override { v.visitT(*this); };
    };

    struct AppP: Term {
        variable func;
        variable cont;
        variable arg;
        virtual ~AppP() = default;
        AppP(const std::string& f, const std::string& c, const variable& a): func{f}, cont{c}, arg{a} {}
        virtual void accept(Visitor& v) const override { v.visitT(*this); };
    };

    namespace convenience {
        template<typename E, typename... Ts> term make_term(const Ts&... args);
        term let(const std::string& n, const value& v, const term& i);
        term pi(int f, const std::string& n, const std::string& t, const term& i);
        term let_cont(const std::string& n, const std::vector<variable>& as, const term& b, const term& i);
        term app_cont(const std::string& n, const std::string& a);
        term app_func(const std::string& n, const std::string& c, const std::string& a);
        term app_prim(const std::string& n, const std::string& c, const std::string& a);
        term halt(const variable& v);

        template<typename E, typename... Ts> value make_value(const Ts&... args);
        value lambda(const std::string& c, const std::vector<variable>& as, const term& b);
        value f64(double v);
        value boolean(bool v);
        value tuple(const std::vector<variable>& fs);
    }

    using namespace convenience;
        
    struct ToSExp: Visitor {
        std::ostream& os;
        int indent;
        std::string prefix = "";

        ToSExp(std::ostream& os_, int i=0, const std::string& p=""): os{os_}, indent{i}, prefix{p} { os << prefix << std::string(indent, ' '); }

        void operator()(const term& cps) {
            cps->accept(*this);
        }

        virtual void visitT(const LetV& e) override {
            os << "(let-value ("
               << e.name
               << " ";
            e.val->accept(*this);
            indent += 4;
            os << ")\n" << prefix << std::string(indent, ' ');
            e.in->accept(*this);
            os << ")";
            indent -= 4;
        }
        virtual void visitT(const LetC& e) override {
            os << "(let-cont ("
               << e.name
               << " (";
            for(const auto& arg: e.args) os << arg << " ";
            indent += 4;
            os << ")\n" << prefix << std::string(indent, ' ');
            e.body->accept(*this);
            os << "\n" << prefix << std::string(indent, ' ');
            e.in->accept(*this);
            os << ")";
            indent -= 4;
        }
        virtual void visitT(const LetT& e) override {
            os << "(pi-"
               << e.field
               << " "
               << e.name
               << " "
               << e.tuple;
            indent += 4;
            os << ")\n" << prefix << std::string(indent, ' ');
            e.in->accept(*this);
            os << ")";
            indent -= 4;
        }
        virtual void visitT(const Halt& e) override {
            os << "(halt "
               << e.name
               << ")";
        }
        virtual void visitT(const AppC& e) override {
            os << "(apply-cont "
               << e.name
               << " "
               << e.arg
               << ")";
        }
        virtual void visitT(const AppF& e) override {
            os << "(apply-func "
               << e.func
               << " "
               << e.cont
               << " "
               << e.arg << ")";
        }
        virtual void visitT(const AppP& e) override {
            os << "(apply-prim "
               << e.func
               << " "
               << e.cont
               << " "
               << e.arg
               << ")";
        }

        virtual void visitV(const Tuple& v) override {
            os << "(";
            for (const auto& field: v.fields) os << field << ", ";
            os << ")";
        };

        virtual void visitV(const F64& v) override {
            os << v.value;
        };

        virtual void visitV(const Bool& v) override {
            os << v.value;
        };

        virtual void visitV(const Lambda& v) override {
            os << "(lambda "
               << v.cont
               << " (";
            for (const auto& arg: v.args) os << arg << " ";
            indent += 4;
            os << ")\n" << prefix << std::string(indent, ' ');
            v.body->accept(*this);
            os << ")";
            indent -= 4;
        };
    };

    struct ToCPS: AST::Visitor {
        using context = std::function<term(variable)>;
        context ctx = [&](auto v) { return halt(v); };

        ToCPS(): counter{0} {}

        term result;
        std::string genvar();

        size_t counter;

        term convert(const AST::expr& e) {
            e->accept(*this);
            return result;
        }

        virtual void visit(const AST::F64& e) override {
            auto x = genvar();
            result = let(x, f64(e.val), ctx(x));
        }

        virtual void visit(const AST::Bool& e)  override {
            auto x = genvar();
            result = let(x, boolean(e.val), ctx(x));
        }
        virtual void visit(const AST::Var& e) override {
            result = ctx(e.name);
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
        virtual void visit(const AST::Proj& e)  override {
            auto kappa = ctx;
            auto x = genvar();
            ctx = [&](auto z) { return pi(e.field, x, z, kappa(x)); };
            e.tuple->accept(*this);
            ctx = kappa;
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
        virtual void visit(const AST::Cond&)  override {}

        virtual void visit(const AST::Let& e)   override {
            e.body->accept(*this);
            auto cont = result;

            auto j = genvar();
            {
                auto tmp = ctx;
                ctx = [&](auto z){ return app_cont(j, {z}); };
                e.val->accept(*this);
                ctx = tmp;
            }
            auto body = result;
            result = let_cont(j, {e.var}, cont, body);
        }
        virtual void visit(const AST::Lam& e) override {
            auto f = genvar();
            auto k = genvar();
            auto x = e.args;
            auto tmp = ctx;
            ctx = [&](auto z) { return app_cont(k, {z}); };
            e.body->accept(*this);
            auto body = result;
            ctx = tmp;
            result = let(f, lambda(k, x, body), ctx(f));
        }
    };

    struct ToCPSImproved;

    struct ToCPSImprovedHelper: AST::Visitor {
        ToCPSImproved& parent;
        term result;
        variable ctx;
        ToCPSImprovedHelper(ToCPSImproved&p): parent{p} {}

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
        
    struct ToCPSImproved: ToCPS {
        ToCPSImprovedHelper helper;

        ToCPSImproved(): ToCPS(), helper{*this} {}
        
        virtual void visit(const AST::Let& e)   override {
            e.body->accept(*this);
            auto cont = result;
            auto j = genvar();
            helper.ctx = j;
            e.val->accept(helper);
            auto body = helper.result;
            result = let_cont(j, {e.var}, cont, body);
        }
        virtual void visit(const AST::Lam& e) override {
            auto f = genvar();
            auto k = genvar();
            auto x = e.args;
            helper.ctx = k;
            e.body->accept(helper);
            auto body = helper.result;
            result = let(f, lambda(k, x, body), ctx(f));
        }
    };
}
