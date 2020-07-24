#pragma once

#include "TailCPS.hpp"

namespace TailCPS {
    struct PrimSimplify {
        std::vector<std::pair<std::string, double>> known_f64;
        std::vector<std::pair<std::string, bool>> known_bool;
        std::vector<std::pair<std::string, std::vector<std::string>>> known_tuple;

        std::vector<std::pair<std::string, double>>::reverse_iterator
        try_find_f64(const std::string& name) {
            return std::find_if(known_f64.rbegin(),
                                known_f64.rend(),
                                [&](const auto& it){ return it.first == name; });
        }

        std::vector<std::pair<std::string, bool>>::reverse_iterator
        try_find_bool(const std::string& name) {
            return std::find_if(known_bool.rbegin(),
                                known_bool.rend(),
                                [&](const auto& it){ return it.first == name; });
        }

        std::vector<std::pair<std::string, std::vector<std::string>>>::reverse_iterator
        try_find_tuple(const std::string& name) {
            return std::find_if(known_tuple.rbegin(),
                                known_tuple.rend(),
                                [&](const auto& it){ return it.first == name; });
        }

        term operator()(const LetV& t) {
            auto tmp = t;
            if (std::holds_alternative<F64>(*tmp.val)) {
                known_f64.push_back({tmp.name, std::get<F64>(*tmp.val).value});
                tmp.in = std::visit(*this, *tmp.in);
                known_f64.pop_back();
                return std::make_shared<Term>(tmp);
            }
            if (std::holds_alternative<Bool>(*tmp.val)) {
                known_bool.push_back({tmp.name, std::get<Bool>(*tmp.val).value});
                tmp.in = std::visit(*this, *tmp.in);
                known_bool.pop_back();
                return std::make_shared<Term>(tmp);
            }
            if (std::holds_alternative<Tuple>(*tmp.val)) {
                known_tuple.push_back({tmp.name, std::get<Tuple>(*tmp.val).fields});
                tmp.in = std::visit(*this, *tmp.in);
                known_tuple.pop_back();
                return std::make_shared<Term>(tmp);
            }
            tmp.in = std::visit(*this, *tmp.in);
            return std::make_shared<Term>(tmp);
        }
        term operator()(const LetC& t) {
            auto tmp = t;
            tmp.body = std::visit(*this, *tmp.body);
            tmp.in = std::visit(*this, *tmp.in);
            return std::make_shared<Term>(tmp);
        }
        term operator()(const LetT& t) {
            auto tmp = t;
            auto tuple = try_find_tuple(tmp.tuple);
            if (tuple != known_tuple.rend()) {
                auto name = tuple->second[t.field];
                auto pf64 = try_find_f64(name);
                if (pf64 != known_f64.rend()) {
                    auto val = pf64->second;
                    known_f64.push_back({tmp.name, val});
                    tmp.in = std::visit(*this, *tmp.in);
                    known_f64.pop_back();
                    return let(tmp.name, f64(val), tmp.in);
                }
                auto pbool = try_find_bool(name);
                if (pbool != known_bool.rend()) {
                    auto val = pbool->second;
                    known_bool.push_back({tmp.name, val});
                    tmp.in = std::visit(*this, *tmp.in);
                    known_bool.pop_back();
                    return let(tmp.name, boolean(val), tmp.in);
                }
            }
            tmp.in = std::visit(*this, *tmp.in);
            return std::make_shared<Term>(tmp);
        }
        term operator()(const LetF& t) {
            auto tmp = t;
            tmp.body = std::visit(*this, *tmp.body);
            tmp.in = std::visit(*this, *tmp.in);
            return std::make_shared<Term>(tmp);
        }
        term operator()(const LetP& t) {
            auto tmp = t;
            if (tmp.name == "+") {
                auto lhs = try_find_f64(tmp.args[0]);
                auto rhs = try_find_f64(tmp.args[1]);
                if ((lhs != known_f64.rend()) && (rhs != known_f64.rend())) {
                    auto res = lhs->second + rhs->second;
                    known_f64.push_back({tmp.var, res});
                    tmp.in = std::visit(*this, *tmp.in);
                    known_f64.pop_back();
                    return let(tmp.var, f64(res), tmp.in);
                }
                return std::make_shared<Term>(tmp);
            }
            if (tmp.name == "-") {
                auto lhs = try_find_f64(tmp.args[0]);
                auto rhs = try_find_f64(tmp.args[1]);
                if ((lhs != known_f64.rend()) && (rhs != known_f64.rend())) {
                    auto res = lhs->second - rhs->second;
                    known_f64.push_back({tmp.var, res});
                    tmp.in = std::visit(*this, *tmp.in);
                    known_f64.pop_back();
                    return let(tmp.var, f64(res), tmp.in);
                }
                return std::make_shared<Term>(tmp);
            }
            if (tmp.name == "*") {
                auto lhs = try_find_f64(tmp.args[0]);
                auto rhs = try_find_f64(tmp.args[1]);
                if ((lhs != known_f64.rend()) && (rhs != known_f64.rend())) {
                    auto res = lhs->second * rhs->second;
                    known_f64.push_back({tmp.var, res});
                    tmp.in = std::visit(*this, *tmp.in);
                    known_f64.pop_back();
                    return let(tmp.var, f64(res), tmp.in);
                }
                return std::make_shared<Term>(tmp);
            }
            throw std::runtime_error("Unimplemented PrimOp: '"s + t.name + "'");
        }
        term operator()(const AppF& t) {
            auto tmp = t;
            return std::make_shared<Term>(tmp);
        }
        term operator()(const AppC& t) {
            auto tmp = t;
            return std::make_shared<Term>(tmp);
        }
        term operator()(const Halt& t) {
            auto tmp = t;
            return std::make_shared<Term>(tmp);
        }
    };

    term prim_simplify(const term& in) {
        return dead_let(std::visit(PrimSimplify(), *in));
    }
}
