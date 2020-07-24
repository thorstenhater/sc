#pragma once

#include <iostream>
#include <sstream>
#include <string>
#include <memory>
#include <vector>

#include "AST.hpp"
#include "TailCPS.hpp"
#include "Simplify.hpp"
extern "C" {
    struct ast { AST::expr data = nullptr; };
    struct cps { TailCPS::term data = nullptr; };

    ast* ast_var(const char* name) {
        if (!name) return NULL;
        ast* res = new ast;
        res->data = AST::var(name);
        return res;
    }

    ast* ast_let(const char* name, const ast* value, const ast* body) {
        if (!body || !body->data) return NULL;
        if (!value || !value->data) return NULL;
        ast* res = new ast;
        res->data = AST::let(name, value->data, body->data);
        return res;
    }

    ast* ast_lambda(const char** names, const ast* body) {
        if (!body || !body->data) return NULL;
        ast* res = new ast;
        std::vector<std::string> ns;
        for (auto name = names; *name != NULL; ++name) ns.emplace_back(*name);
        res->data = AST::lambda(ns, body->data);
        return res;
    }

    ast* ast_tuple(const ast** values) {
        if (!values || !*values) return NULL;
        ast* res = new ast;
        std::vector<AST::expr> exprs;
        for (auto value = values; *value != NULL; ++value) {
            exprs.push_back((*value)->data);
        }
        res->data = AST::tuple(exprs);
        return res;
    }

    ast* ast_prim(const char* prim, const ast** values) {
        if (!values || !*values) return NULL;
        ast* res = new ast;
        std::vector<AST::expr> exprs;
        for (auto value = values; *value != NULL; ++value) {
            exprs.push_back((*value)->data);
        }
        res->data = AST::prim(prim, exprs);
        return res;
    }

    ast* ast_pi(int i, const ast* value) {
        if (!value || !value->data) return NULL;
        ast* res = new ast;
        res->data = AST::project(i, value->data);
        return res;
    }

    ast* ast_f64(double v) {
        ast* res = new ast;
        res->data = AST::f64(v);
        return res;
    }

    ast* ast_typecheck(const ast* in) {
        if (!in || !in->data) return NULL;
        ast* out = new ast;
        out->data = AST::typecheck(in->data);
        return out;
    }

    ast* ast_alpha_convert(const ast* in) {
        if (!in || !in->data) return NULL;
        ast* out = new ast;
        out->data = AST::alpha_convert(in->data);
        return out;
    }

    cps* ast_to_cps(const ast* in) {
        if (!in || !in->data) return NULL;
        cps* out = new cps;
        out->data = TailCPS::ast_to_cps(in->data);
        return out;
    }

    void cps_show(const cps* in) {
        if (in && in->data) {
            TailCPS::cps_to_sexp(std::cout, in->data);
        } else {
            std::cout << "(null)";
        }
    }

    void ast_show(const ast* in) {
        if (in && in->data) {
            AST::to_sexp(std::cout, in->data);
        } else {
            std::cout << "(null)";
        }
    }

    void ast_destroy(ast* a) {
        std::cerr << "Deleting AST\n";
        if (!a) return;
        delete a;
    }

    void cps_destroy(cps* a) {
        std::cerr << "Deleting CPS\n";
        if (!a) return;
        delete a;
    }

    cps* cps_beta_cont(const cps* in) {
        if (!in || !in->data) return NULL;
        cps* out = new cps;
        out->data = TailCPS::beta_cont(in->data);
        return out;
    }

    cps* cps_beta_func(const cps* in) {
        if (!in || !in->data) return NULL;
        cps* out = new cps;
        out->data = TailCPS::beta_func(in->data);
        return out;
    }

    cps* cps_dead_let(const cps* in) {
        if (!in || !in->data) return NULL;
        cps* out = new cps;
        out->data = TailCPS::dead_let(in->data);
        return out;
    }

    cps* cps_prim_cse(const cps* in) {
        if (!in || !in->data) return NULL;
        cps* out = new cps;
        out->data = TailCPS::prim_cse(in->data);
        return out;
    }

    cps* cps_prim_simplify(const cps* in) {
        if (!in || !in->data) return NULL;
        cps* out = new cps;
        out->data = TailCPS::prim_simplify(in->data);
        return out;
    }

    void cps_gen_cxx(const cps* in) {
        if (!in || !in->data) return;
        TailCPS::generate_cxx(std::cout, in->data);
    }
}
