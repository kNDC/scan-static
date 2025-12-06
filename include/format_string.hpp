#pragma once

#include "types.hpp"
#include <array>
#include <expected>

namespace stdx::internals
{
    // Шаблонный класс для хранения форматирующей строчки и ее особенностей
    // ваш код здесь
    template <fixed_string fs>
    class format_string
    {
    private:
        /* Функция для получения количества плейсхолдеров и 
        проверки корректности формирующей строки */
        consteval static std::expected<size_t, parse_error>
        get_placeholder_count();

        // Функция проверяет, что нет ошибки, и присваивает ответ
        consteval static size_t assign_placeholder_count();

    public:
        constexpr static const fixed_string<fs.size + 1>& str = fs;

        /* При ошибке обработки попытка положить parse_error в 
        n_placeholders породит ошибку компиляции. */
        constexpr static size_t n_placeholders = 
            assign_placeholder_count();

    private:
        // Функция для получения позиций плейсхолдеров
        using PosArray = std::array<std::pair<size_t, size_t>, n_placeholders>;
        consteval static PosArray get_placeholder_positions();

    public:
        constexpr static PosArray placeholder_positions = 
            get_placeholder_positions();
    };

    // Пользовательский литерал
    template <fixed_string fs>
    constexpr auto operator""_fs()
    {
        return fs;
    }

    template <fixed_string fs>
    consteval std::expected<size_t, parse_error> 
    format_string<fs>::get_placeholder_count()
    {
        if constexpr (str.empty()) return 0;

        size_t out = 0;
        size_t pos = 0;

        while (pos < str.size)
        {
            // Пропускаем все символы до '{'
            if (str.data[pos] != '{')
            {
                ++pos;
                continue;
            }

            // Проверка на незакрытость
            if (pos >= str.size)
            {
                return std::unexpected(parse_error{"Missing closing brace"});
            }

            // Начало плейсхолдера
            ++out;
            ++pos;

            // Проверка спецификатора формата
            if (str.data[pos] == '%')
            {
                ++pos;
                if (pos >= str.size)
                {
                    return std::unexpected(parse_error{"Missing closing brace"});
                }

                // Проверка допустимости спецификатора
                const char spec = str.data[pos];
                constexpr char valid_specs[] = {'d', 'u', 's', 'f'};
                bool valid = false;

                for (const char s : valid_specs)
                {
                    if (spec == s)
                    {
                        valid = true;
                        break;
                    }
                }

                if (!valid)
                {
                    return std::unexpected(parse_error{"Invalid specifier"});
                }
                ++pos;
            }

            // Проверка наличия закрывающей скобки
            if (pos >= str.size || str.data[pos] != '}')
            {
                return std::unexpected(parse_error{"Missing closing brace"});
            }
            ++pos;
        }

        return out;
    }

    template <fixed_string fs>
    consteval size_t format_string<fs>::assign_placeholder_count()
    {
        constexpr std::expected<size_t, parse_error> out = 
            get_placeholder_count();
        
        static_assert(!!out, 
            "Error parsing the format string");
        
        return *out;
    }

    template <fixed_string fs>
    consteval format_string<fs>::PosArray 
    format_string<fs>::get_placeholder_positions()
    {
        PosArray out;
        size_t pos = 0;
        size_t i = 0;

        while (pos < str.size)
        {
            switch (str.data[pos])
            {
            case '{':
                out[i].first = pos++;
                break;
            case '}':
                out[i++].second = pos++;
                break;
            default:
                ++pos;
                break;
            }
        }

        return out;
    }
}  // namespace stdx::internals