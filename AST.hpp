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

using namespace std::string_literals;

namespace AST {
    struct F64;
    struct Bool;
    struct Tuple;
    struct Proj;
    struct Var;
    struct App;
    struct Lam;
    struct Prim;
    struct Let;
    struct Cond;

    struct Visitor {
        virtual void visit(const F64&)   = 0;
        virtual void visit(const Prim&)   = 0;
        virtual void visit(const Tuple&) = 0;
        virtual void visit(const Proj&)  = 0;
        virtual void visit(const Var&)   = 0;
        virtual void visit(const Let&)   = 0;
        virtual void visit(const Lam&)   = 0;
        virtual void visit(const App&)   = 0;
        virtual void visit(const Bool&)  = 0;
        virtual void visit(const Cond&)  = 0;
    };

    struct Expr {
        void hi() { std::cout << "hi from expr\n"; }
        virtual void accept(Visitor&) const {};
    };

    using expr = std::shared_ptr<Expr>;

    struct F64: Expr {
        const double val;
        virtual ~F64() = default;
        F64(double v): val{v} {}
        virtual void accept(Visitor& v) const override { v.visit(*this); }
    };

    struct Bool: Expr {
        const bool val;
        virtual ~Bool() = default;
        Bool(bool v): val{v} {}
        virtual void accept(Visitor& v) const override { v.visit(*this); }
    };

    struct Proj: Expr {
        expr tuple;
        size_t field;
        virtual ~Proj() = default;
        Proj(const size_t f, const expr& t): tuple{t}, field{f} {}
        virtual void accept(Visitor& v) const override { v.visit(*this); }
    };

    struct Tuple: Expr {
        std::vector<expr> fields;
        virtual ~Tuple() = default;
        Tuple() = default;
        Tuple(const std::vector<expr>& fs): fields{fs} {}
        virtual void accept(Visitor& v) const override { v.visit(*this); };
    };

    struct Var: Expr {
        virtual ~Var() = default;
        const std::string name;
        Var(const std::string& n): name{n} {}
        virtual void accept(Visitor& v) const override { v.visit(*this); };
    };

    struct App: Expr {
        expr fun;
        expr args;
        virtual ~App() = default;
        App(const expr& f, const std::vector<expr>& as): fun{f}, args{std::make_shared<Tuple>(as)} {}
        virtual void accept(Visitor& v) const override { v.visit(*this); };
    };

    struct Prim: Expr {
        std::string op;
        expr args;
        virtual ~Prim() = default;
        Prim(const std::string& o, const std::vector<expr>& as): op{o}, args{std::make_shared<Tuple>(as)} {}
        virtual void accept(Visitor& v) const override { v.visit(*this); };
    };

    struct Lam: Expr {
        std::vector<std::string> args;
        expr body;
        virtual ~Lam() = default;
        Lam(const std::vector<std::string>& as, const expr& b): args{as}, body{b} {}
        virtual void accept(Visitor& v) const override { v.visit(*this); };
    };

    struct Let: Expr {
        std::string var;
        expr val;
        expr body;
        virtual ~Let() = default;
        Let(const std::string& n, const expr& v, const expr& b): var{n}, val{v}, body{b} {}
        virtual void accept(Visitor& v) const override { v.visit(*this); };
    };
    
    struct Cond: Expr {
        expr pred;
        expr on_t;
        expr on_f;
        virtual ~Cond() = default;
        Cond(const expr& p, const expr& t, const expr& f): pred{p}, on_t{t}, on_f{f} {}
        virtual void accept(Visitor& v) const override { v.visit(*this); };
    };
  
    struct ToSExp: Visitor {
        virtual std::string name() { return "to sexp"; }

        std::ostream& os;
        int indent;
        std::string prefix = "";

        ToSExp(std::ostream& os_, int i=0, const std::string& p=""): os{os_}, indent{i}, prefix{p} { os << prefix << std::string(indent, ' '); }

        virtual void visit(const Prim& e) override {
            os << "("
               << e.op
               << " ";
            e.args->accept(*this);
            os << ")";
        }

        virtual void visit(const Bool& e) override {
            os << (e.val ? "true" : "false");
        }

        virtual void visit(const F64& e) override { os << e.val; }

        virtual void visit(const Var& e) override { os << e.name; }

        virtual void visit(const Lam& e) override {
            os << "(lambda (";
            for (const auto& arg: e.args) {
                os << arg << " ";
            }
            indent += 4;
            os << ")\n" << prefix << std::string(indent, ' ');
            e.body->accept(*this);
            os << ")";
            indent -= 4;
        }

        virtual void visit(const Cond& e) override {
            os << "(if ";
            e.pred->accept(*this);
            indent += 4;
            os << "\n" << prefix << std::string(indent, ' ');
            e.on_t->accept(*this);
            os << "\n" << prefix << std::string(indent, ' ');
            e.on_f->accept(*this);
            os << ")";
            indent -= 4;
        }

        virtual void visit(const Tuple& e) override {
            os << "(";
            for (const auto& field: e.fields) {
                field->accept(*this);
                os << ", ";
            }
            os << ")";
        }

        virtual void visit(const Proj& e) override {
            os << "(pi-" << e.field << " ";
            e.tuple->accept(*this);
            os << ")";
        }

        virtual void visit(const App& e) override {
            os << "(";
            e.fun->accept(*this);
            os << " ";
            e.args->accept(*this);
            os << ")";
        }

        virtual void visit(const Let& e) override {
            os << "(let (" << e.var << " ";
            e.val->accept(*this);
            os << ") ";
            indent += 4;
            os << "\n" << prefix << std::string(indent, ' ');
            e.body->accept(*this);
            os << ")";
            indent -= 4;
        }
    };

    namespace convenience {
        template<typename E, typename... Ts> expr make_expr(const Ts&... args);
        expr nil();
        expr yes();
        expr no();
        expr tuple(const std::vector<expr>& fs);
        expr project(size_t i, const expr& f);
        expr cond(const expr& p, const expr& t, const expr& f);
        expr f64(double v);
        expr var(const std::string& n);
        expr add(const expr& l, const expr& r);
        expr mul(const expr& l, const expr& r);
        expr sub(const expr& l, const expr& r);
        expr lambda(const std::vector<std::string>& args, const expr& body);
        expr apply(const expr& fun, const std::vector<expr>&& args);
        expr let(const std::string& var, const expr& bind, const expr& in);

        expr pi(const std::string& var, size_t field, const expr& tuple, const expr& in);
        expr defn(const std::string& name, const std::vector<std::string>& args, const expr& body, const expr& in);

        expr operator"" _var(const char* v, size_t);
        expr operator"" _f64(long double v);
        expr operator *(const expr& l, const expr& r);
        expr operator +(const expr& l, const expr& r);
        expr operator -(const expr& l, const expr& r);
    }
}
