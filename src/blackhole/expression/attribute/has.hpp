#pragma once

#include <string>

#include <boost/variant.hpp>

#include "blackhole/attribute.hpp"
#include "blackhole/detail/config/underlying.hpp"
#include "blackhole/expression/helper.hpp"
#include "blackhole/filter.hpp"

namespace blackhole {

namespace expression {

template<typename T>
class has_attribute_visitor : public boost::static_visitor<bool>{
public:
    bool operator()(const T&) const {
        return true;
    }

    template<typename Other>
    bool operator()(const Other&) const {
        return false;
    }
};

template<typename T>
struct has_attr_action_t : public aux::LogicMixin<has_attr_action_t<T>> {
    typedef typename blackhole::aux::underlying_type<T>::type underlying_type;

    const std::string name;

    has_attr_action_t(const std::string& name) : name(name) {}

    bool operator()(const attribute::combined_view_t& attributes) const {
        static has_attribute_visitor<underlying_type> visitor;

        if (auto attribute = attributes.get(name)) {
            return boost::apply_visitor(visitor, *attribute);
        }

        return false;
    }
};

// For dynamic attributes.
template<typename T>
has_attr_action_t<T> has_attr(const std::string& name) {
    return has_attr_action_t<T>(name);
}

// For static attributes.
template<typename T>
has_attr_action_t<typename T::type> has_attr(const T&) {
    return has_attr<typename T::type>(std::string(T::name()));
}

} // namespace expression

} // namespace blackhole
