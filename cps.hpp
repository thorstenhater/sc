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

#include "AST.hpp"

namespace CPS {
    namespace naive {
        // Variables
        using variable = std::string;

        // Values
        struct Value {

        };

        using value = std::shared_ptr<Value>;

        struct Tuple: Value  {
            std::vector<variable> fields;
        };

        struct F64: Value {
            double value;
        };

        struct Lambda: Value {
            std::vector<variable> args;
            term body;
          };

        // Terms
        struct Term {

        };

        using term = std::shared_ptr<Term>;

        // Value binding
        struct LetV: Term {
            variable name;
            value value;
            term in;
        };

        // Tuple projection
        struct LetT: Term {
            variable name;
            value tuple;
            int field;
            term in;
        };

        // Local continuation
        struct LetC: Term {
            variable name;
            std::vector<variable> args;
            term body;
            term in;
        };

        // Apply Continuation
        struct AppC: Term {
            term cont;
            std::vector<variable> args;
        };

        // Apply function
        struct AppF: Term {
            term cont;
            term func;
            std::vector<variable> args;
        };

    }

    struct ToCPS: AST::Visitor {
        virtual void visit(const AST::F64&)   override {}
        virtual void visit(const AST::Add&)   override {}
        virtual void visit(const AST::Mul&)   override {}
        virtual void visit(const AST::Tuple&) override {}
        virtual void visit(const AST::Proj&)  override {}
        virtual void visit(const AST::Var&)   override {}
        virtual void visit(const AST::Let&)   override {}
        virtual void visit(const AST::Lam&)   override {}
        virtual void visit(const AST::App&)   override {}
        virtual void visit(const AST::Bool&)  override {}
        virtual void visit(const AST::Cond&)  override {}
    };
}
