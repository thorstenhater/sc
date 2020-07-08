#pragma once

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
    struct Add;
    struct F64;
    struct Bool;
    struct Tuple;
    struct Proj;
    struct Var;
    struct App;
    struct Lam;
    struct Mul;
    struct Let;
    struct Cond;

    struct Visitor {
        virtual void visit(const F64&)   = 0;
        virtual void visit(const Add&)   = 0;
        virtual void visit(const Mul&)   = 0;
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
        virtual void accept(Visitor&) const {};
    };

    using expr = std::shared_ptr<Expr>;

    struct F64: Expr {
        const double val;
        F64(double v): val{v} {}
        virtual void accept(Visitor& v) const override { v.visit(*this); };
    };

    struct Bool: Expr {
        const bool val;
        Bool(bool v): val{v} {}
        virtual void accept(Visitor& v) const override { v.visit(*this); };
    };

    struct Proj: Expr {
        expr tuple;
        size_t field;
        Proj(const size_t f, const expr& t): tuple{t}, field{f} {}
        virtual void accept(Visitor& v) const override { v.visit(*this); };
    };

    struct Tuple: Expr {
        std::vector<expr> fields;
        Tuple() = default;
        Tuple(const std::vector<expr>& fs): fields{fs} {}
        virtual void accept(Visitor& v) const override { v.visit(*this); };
    };

    struct Var: Expr {
        const std::string name;
        Var(const std::string& n): name{n} {}
        virtual void accept(Visitor& v) const override { v.visit(*this); };
    };

    struct App: Expr {
        expr fun;
        std::vector<expr> args;
        App(const expr& f, const std::vector<expr>& as): fun{f}, args{as} {}
        virtual void accept(Visitor& v) const override { v.visit(*this); };
    };

    struct Add: Expr {
        expr lhs, rhs;
        Add(const expr& l, const expr& r): lhs{l}, rhs{r} {}
        virtual void accept(Visitor& v) const override { v.visit(*this); };
    };

    struct Mul: Expr {
        expr lhs, rhs;
        Mul(const expr& l, const expr& r): lhs{l}, rhs{r} {}
        virtual void accept(Visitor& v) const override { v.visit(*this); };
    };

    struct Lam: Expr {
        std::vector<std::string> args;
        expr body;
        Lam(const std::vector<std::string>& as, const expr& b): args{as}, body{b} {}
        virtual void accept(Visitor& v) const override { v.visit(*this); };
    };

    struct Let: Expr {
        std::string var;
        expr val;
        expr body;
        Let(const std::string& n, const expr& v, const expr& b): var{n}, val{v}, body{b} {}
        virtual void accept(Visitor& v) const override { v.visit(*this); };
    };
    
    struct Cond: Expr {
        expr pred;
        expr on_t;
        expr on_f;
        Cond(const expr& p, const expr& t, const expr& f): pred{p}, on_t{t}, on_f{f} {}
        virtual void accept(Visitor& v) const override { v.visit(*this); };
    };
  
    struct ToSExp: Visitor {
        virtual std::string name() { return "to sexp"; }

        std::ostream& os;
        int indent;
        std::string prefix = "";

        ToSExp(std::ostream& os_, int i=0, const std::string& p=""): os{os_}, indent{i}, prefix{p} { os << prefix << std::string(indent, ' '); }

        virtual void visit(const Add& e) override {
            os << "(+ ";
            e.lhs->accept(*this);
            os << " ";
            e.rhs->accept(*this);
            os << ")";
        }

        virtual void visit(const Bool& e) override {
            os << (e.val ? "true" : "false");
        }

        virtual void visit(const F64& e) override { os << e.val; }

        void visit(const Mul& e) {
            os << "(* ";
            e.lhs->accept(*this);
            os << " ";
            e.rhs->accept(*this);
            os << ")";
        }

        void visit(const Var& e) { os << e.name; }

        void visit(const Lam& e) {
            os << "(lambda (";
            for (const auto& arg: e.args) os << arg << " ";
            indent += 4;
            os << ")\n" << prefix << std::string(indent, ' ');
            e.body->accept(*this);
            os << ")";
            indent -= 4;
        }

        void visit(const Cond& e) {
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

        void visit(const Tuple& e) {
            os << "(";
            for (const auto& field: e.fields) {
                field->accept(*this);
                os << ", ";
            }
            os << ")";
        }

        void visit(const Proj& e) {
            os << "(pi-" << e.field << " ";
            e.tuple->accept(*this);
            os << ")";
        }

        void visit(const App& e) {
            os << "(";
            e.fun->accept(*this);
            os << " ";
            for (const auto& arg: e.args) {
                arg->accept(*this);
                os << " ";
            }
            os << ")";
        }

        void visit(const Let& e) {
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
        template<typename E, typename... Ts> expr make_expr(const Ts&... args) { return std::make_shared<E>(E(args...)); }
        expr nil() { return make_expr<Tuple>(); }
        expr yes() { return make_expr<Bool>(true); }
        expr no() { return make_expr<Bool>(false); }
        expr tuple(const std::vector<expr>& fs) { return make_expr<Tuple>(fs); }
        expr project(size_t i, const expr& f) { return make_expr<Proj>(i, f); }
        expr cond(const expr& p, const expr& t, const expr& f) { return make_expr<Cond>(p, t, f); }
        expr f64(double v) { return make_expr<F64>(v); }
        expr var(const std::string& n) { return make_expr<Var>(n); }
        expr add(const expr& l, const expr& r) { return make_expr<Add>(l, r); }
        expr mul(const expr& l, const expr& r) { return make_expr<Mul>(l, r); }
        expr neg(const expr& r) { return make_expr<Mul>(f64(-1), r); }
        expr sub(const expr& l, const expr& r) { return make_expr<Add>(l, neg(r)); }
        expr lambda(const std::vector<std::string>& args, const expr& body) { return make_expr<Lam>(args, body); }
        expr apply(const expr& fun, const std::vector<expr>&& args) { return make_expr<App>(fun, args); }
        expr let(const std::string& var, const expr& bind, const expr& in) { return make_expr<Let>(var, bind, in); }

        expr pi(const std::string& var, size_t field, const expr& tuple, const expr& in) { return let(var, project(field, tuple), in); }
        expr defn(const std::string& name, const std::vector<std::string>& args, const expr& body, const expr& in) { return let(name, lambda(args, body), in); }

        expr operator"" _var(const char* v, size_t) { return var(v); }
        expr operator"" _f64(long double v) { return f64(v); }
        expr operator *(const expr& l, const expr& r) { return mul(l, r); }
        expr operator +(const expr& l, const expr& r) { return add(l, r); }
        expr operator -(const expr& l, const expr& r) { return sub(l, r); }
    }
}
