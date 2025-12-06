#pragma once

#include "types.hpp"
#include "format_string.hpp"
#include "parse.hpp"

namespace stdx
{
    using namespace stdx::internals;

    // Главная функция
    template <format_string format, 
        fixed_string source, typename... Ts>
    [[nodiscard]] consteval scan_result<Ts...> scan()
    {
        using namespace stdx::internals;

        /* Можно обыграть с помощью requires, но так можно в 
        явном виде прописать указание на причину ошибки. */
        static_assert((... && (!std::is_reference_v<Ts> && 
            (std::is_integral_v<Ts> || 
            std::is_floating_point_v<Ts> || 
            std::is_same_v<Ts, std::string_view>))),
            "Only integral types, float, double and "
            "std::string_view are accepted; "
            "references are not permitted");

        return []<size_t... I>(indices<I...>)
            {
                return scan_result<Ts...>{parse_input<I, format, source, Ts>()...};
            }(generate_indices<format.n_placeholders>{});
    }
} // namespace stdx