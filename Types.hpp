#pragma once

#include <vector>
#include <string>
#include <memory>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <unordered_map>
#include <exception>
#include <variant>

using namespace std::string_literals;

namespace Types {
    struct TypeError: std::exception {
        std::string message;
        TypeError(const std::string& m): message{m} {}
        virtual const char* what() const throw() {
            return message.c_str();
        }
    };

    struct TyF64;
    struct TyBool;
    struct TyFunc;
    struct TyTuple;
    struct TyVar;

    using Type = std::variant<TyF64,
                              TyBool,
                              TyFunc,
                              TyTuple,
                              TyVar>;
    using type = std::shared_ptr<Type>;

    struct Typed {
        Types::type type;
        Typed(): type{nullptr} {}
        Typed(const Types::type& t): type{t} {}
    };

    struct TyFunc {
        std::vector<type> args;
        type result;
        TyFunc() = default;
        TyFunc(const std::vector<type>& as, const type& r) noexcept: args{as}, result{r} {}
    };

    struct TyF64 {
        TyF64() = default;
    };

    struct TyTuple {
        std::vector<type> field_types;
        int size;
        TyTuple() = default;
        TyTuple(const std::vector<type>& fs) noexcept: field_types{fs}, size{-1} {}
        TyTuple(const std::vector<type>& fs, const int i) noexcept: field_types{fs}, size{i} {}
    };

    struct TyBool {
        TyBool() = default;
    };

    struct TyVar {
        std::string name;
        type alias;
        TyVar() = default;
        TyVar(const std::string& n) noexcept: name{n}, alias{nullptr} {}
    };

    bool operator==(const TyFunc& lhs, const TyFunc& rhs);
    bool operator==(const TyTuple& lhs, const TyTuple& rhs);
    bool operator==(const TyBool&, const TyBool&);
    bool operator==(const TyF64&, const TyF64&);
    bool operator==(const TyVar& lhs, const TyVar& rhs);

    std::string show_type(const type& t);

    type f64_t();
    type var_t(const std::string& n);
    type tuple_t(const std::vector<type> fields);
    type bool_t();
    type func_t(const std::vector<type>& args, const type& res);
}
