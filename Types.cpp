#include "Types.hpp"

namespace Types {
    std::string show_type(const type& t) {
        struct ShowType {
            std::string operator()(const TyFunc& e) {
                auto res = std::stringstream{};
                res << "(";
                for (const auto& arg: e.args) res << std::visit(*this, *arg) << ", ";
                res << ") -> ";
                res << std::visit(*this, *e.result);
                return res.str();
            }

            std::string operator()(const TyTuple& e) {
                auto res = std::stringstream{};
                res << "(";
                for (const auto& field: e.field_types) res << std::visit(*this, *field) << ", ";
                res << ")";
                return res.str();
            }
            std::string operator()(const TyVar& e) {
                if (e.alias.get()) {
                    return std::visit(*this, *e.alias);
                } else {
                    return e.name;
                }
            }
            std::string operator()(const TyF64&)  { return "F64"; }
            std::string operator()(const TyBool&) { return "Bool"; }
        };
        auto show = ShowType();
        return std::visit(show, *t);
    }


    template<typename E, typename... Ts> type make_type(const Ts&... args) { return std::make_shared<Type>(E(args...)); }
    type f64_t() { return make_type<TyF64>(); }
    type var_t(const std::string& n)  { return make_type<TyVar>(n); }
    type tuple_t(const std::vector<type> fields) { return make_type<TyTuple>(fields, fields.size()); }
    type bool_t() { return make_type<TyBool>(); }
    type func_t(const std::vector<type>& args, const type& res) { return make_type<TyFunc>(args, res); }

    bool operator==(const TyFunc& lhs, const TyFunc& rhs) {
        if (lhs.result != rhs.result) { return false; }
        if (lhs.args.size() != rhs.args.size()) { return false; }
        for (auto ix = 0ul; ix < lhs.args.size(); ++ix) {
            if (lhs.args[ix] != rhs.args[ix]) {
                return false;
            }
        }
        return true;
    }

    bool operator==(const TyTuple& lhs, const TyTuple& rhs) {
        if (lhs.field_types.size() != rhs.field_types.size()) { return false; }
        for (auto ix = 0ul; ix < rhs.field_types.size(); ++ix) {
            if (lhs.field_types[ix] != rhs.field_types[ix]) { return false; }
        }
        return true;
    }

    bool operator==(const TyBool&, const TyBool&) { return true; }
    bool operator==(const TyF64&, const TyF64&) { return true; }
    bool operator==(const TyVar& lhs, const TyVar& rhs) { return lhs.name == rhs.name; }
}
