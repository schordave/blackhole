#pragma once

#include <functional>
#include <string>
#include <unordered_map>

#include <boost/format.hpp>

#include "blackhole/attribute.hpp"
#include "blackhole/detail/config/underlying.hpp"
#include "blackhole/detail/stream/stream.hpp"
#include "blackhole/detail/util/unique.hpp"
#include "blackhole/formatter/map/datetime.hpp"
#include "blackhole/keyword/timestamp.hpp"

namespace blackhole {

namespace mapping {

template<typename T>
struct extracter {
    typedef std::function<void(stickystream_t&, const T&)> function_type;

    function_type func;

    extracter(function_type func) :
        func(func)
    {}

    void
    operator()(stickystream_t& stream,
               const attribute::value_t& value) const
    {
        typedef typename aux::underlying_type<T>::type underlying_type;
        func(stream, static_cast<T>(boost::get<underlying_type>(value)));
    }
};

class value_t {
    typedef std::function<
        void(stickystream_t&, const attribute::value_t&)
    > mapping_t;

    std::unordered_map<std::string, mapping_t> m_mappings;

public:
    /*!
     * Generic overload for attributes, which has no keywords defined for them.
     */
    template<typename T>
    void
    add(const std::string& key, typename extracter<T>::function_type handler) {
        m_mappings[key] = extracter<T>(handler);
    }

    /*!
     * Overload for registered keywords. There is no need to provide
     * attribute's name, because it is already known.
     */
    template<typename Keyword>
    void
    add(typename extracter<typename Keyword::type>::function_type handler) {
        add<typename Keyword::type>(Keyword::name(), handler);
    }

    /*!
     * Overload for timestamp attribute. Fast datetime formatter will be used
     * for that case.
     */
    template<
        typename Keyword,
        typename TimePicker = timepicker_t<timezone_t::gmtime>,
        class = typename std::enable_if<
            std::is_same<Keyword, keyword::tag::timestamp_t>::value
        >::type
    >
    void
    add(std::string format) {
        add<Keyword>(
            datetime_formatter_action_t<
                TimePicker
            >(std::move(format))
        );
    }

    /*!
     * Overload for timestamp attribute. Fast datetime formatter will be used
     * for that case.
     * @compat GCC4.6
     * For some reason GCC 4.6 requires it for ambiguity resolution.
     */
    template<
        typename Keyword,
        typename TimePicker = timepicker_t<timezone_t::gmtime>,
        class = typename std::enable_if<
            std::is_same<Keyword, keyword::tag::timestamp_t>::value
        >::type
    >
    void
    add(const char* format) {
        add<Keyword, TimePicker>(std::string(format));
    }

    template<typename T>
    void
    operator()(stickystream_t& stream,
               const std::string& key,
               T&& value) const
    {
        auto it = m_mappings.find(key);
        if (it != m_mappings.end()) {
            const mapping_t& action = it->second;
            action(stream, std::forward<T>(value));
        } else {
            stream << value;
        }
    }

    template<typename T>
    boost::optional<std::string>
    operator()(const std::string& key, T&& value) const {
        auto it = m_mappings.find(key);
        if (it != m_mappings.end()) {
            const mapping_t& action = it->second;
            std::string buffer;
            stickystream_t stream;
            stream.attach(buffer);
            action(stream, std::forward<T>(value));
            stream.flush();
            return boost::optional<std::string>(buffer);
        }

        return boost::optional<std::string>();
    }
};

} // namespace mapping

} // namespace blackhole
