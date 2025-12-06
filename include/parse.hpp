#pragma once

#include "types.hpp"
#include "format_string.hpp"

#include <cstdint>
#include <string_view>
#include <utility>

namespace stdx::internals
{
    /* Шаблонная функция, возвращающая пару позиций в 
    строке с исходными данными, соотвествующих I-ому плейсхолдеру */
    template<size_t I, format_string format, fixed_string source>
    consteval std::pair<const size_t, const size_t> get_parsing_boundaries()
    {
        static_assert(I < format.n_placeholders, 
            "Invalid placeholder index");

        // Получаем границы текущего плейсхолдера в формате
        constexpr size_t fmt_start = format.placeholder_positions[I].first;
        constexpr size_t fmt_end = format.placeholder_positions[I].second;

        // Находим начало в исходной строке
        constexpr size_t src_start = 
            [&]()
            {
                if constexpr (!I) return fmt_start;
                else
                {
                    // Находим конец предыдущего плейсхолдера в исходной строке
                    constexpr std::pair<size_t, size_t> prev_bounds = 
                        get_parsing_boundaries<I - 1, format, source>();
                    const size_t prev_end = prev_bounds.second;

                    // Получаем разделитель между текущим и предыдущим плейсхолдерами
                    constexpr size_t prev_fmt_end = 
                        format.placeholder_positions[I - 1].second;
                    
                    constexpr std::string_view sep = 
                        format.str.sv().substr(prev_fmt_end + 1, fmt_start - (prev_fmt_end + 1));

                    // Ищем разделитель после предыдущего значения
                    size_t pos = source.sv().find(sep, prev_end);
                    return (pos != std::string_view::npos) 
                        ? pos + sep.size() 
                        : source.size;
                }
            }();

        // Находим конец в исходной строке
        constexpr size_t src_end = 
            [&]()
            {
                // Получаем разделитель после текущего плейсхолдера
                if constexpr (fmt_end == (format.str.size - 1))
                {
                    return source.size;
                }

                constexpr std::string_view sep = 
                    format.str.sv().substr(fmt_end + 1,
                        (I < format.n_placeholders - 1)
                            ? format.placeholder_positions[I + 1].first - (fmt_end + 1)
                            : format.str.sv().size() - (fmt_end + 1));
                
                // Ищем разделитель после текущего значения
                constexpr auto pos = source.sv().find(sep, src_start);
                return pos != std::string_view::npos ? pos : source.sv().size();
            }();
        return std::pair{src_start, src_end};
    }

    //=== Обработчики данных ===
    /* Попадание сюда возможно, только если 
    требуемый тип -- не (u)int, не double и 
    не string_view */
    template <fixed_string fs, typename T, typename>
    consteval T parse_value()
    {
        static_assert(false, "Unsupported type -- "
            "you shouldn't have wound up here at all...");
        return std::declval<T>();
    }

    /* Случай int */
    template <fixed_string fs, typename Int, 
        std::enable_if_t<std::is_same_v<Int, int>>* = nullptr>
    consteval int parse_value()
    {
        int out = 0;
        if constexpr (!fs.size) return out;

        constexpr bool is_signed = 
            (fs.data[0] == '-' || fs.data[0] == '+');
        constexpr bool is_negative = (fs.data[0] == '-');
        
        if constexpr (std::find_if(fs.sv().begin() + is_signed, 
            fs.sv().end(), 
            [](const char digit)
            {
                return digit < '0' || digit > '9';
            }) != fs.sv().end()) static_assert(false, "Invalid number format");

        for (size_t i = is_signed; i < fs.size; ++i)
        {
            out *= 10;
            out += fs.data[i] - '0';
        }
        
        return is_negative ? -out : out;
    }

    /* Случай прочих целочисленных типов */
    template <fixed_string fs, typename IntType, 
        std::enable_if_t<!std::is_same_v<IntType, int> && 
            std::is_integral_v<IntType>>* = nullptr>
    consteval IntType parse_value()
    {
        constexpr int out = parse_value<fs, int>();

        if constexpr (std::is_same_v<IntType, int8_t>)
        {
            static_assert(std::abs(out) <= 0x7f, 
                "Integer overflow");
            return (int8_t)out;
        }

        if constexpr (std::is_same_v<IntType, uint8_t>)
        {
            static_assert(out >= 0, "Integer underflow");
            static_assert(out <= 0xff, "Integer overflow");
            return (uint8_t)out;
        }

        if constexpr (std::is_same_v<IntType, int16_t>)
        {
            static_assert(std::abs(out) <= 0x7f'ff, 
                "Integer overflow");
            return (int16_t)out;
        }

        if constexpr (std::is_same_v<IntType, uint16_t>)
        {
            static_assert(out >= 0, "Integer underflow");
            static_assert(out <= 0xff'ff, "Integer overflow");
            return (uint16_t)out;
        }

        if constexpr (std::is_same_v<IntType, int32_t>)
        {
            static_assert(std::abs(out) <= 0x7f'ff'ff'ff, 
                "Integer overflow");
            return (int32_t)out;
        }

        if constexpr (std::is_same_v<IntType, uint32_t>)
        {
            static_assert(out >= 0, "Integer underflow");
            static_assert(out <= 0xff'ff'ff'ff, "Integer overflow");
            return (uint32_t)out;
        }
        
        return (IntType)out;
    }

    /* Случай double */
    template <fixed_string fs, typename Double, 
        std::enable_if_t<std::is_same_v<Double, double>>* = nullptr>
    consteval double parse_value()
    {
        if constexpr (!fs.size) return 0;

        /* Исключаем из рассмотрения завершающую f (например, 1.0f) 
        в случае её наличия */
        constexpr size_t size = (fs.data[fs.size - 1] == 'f')
            ? fs.size - 1 
            : fs.size;

        int whole = 0;
        int frac = 0;
        int exp = 0;
        
        double frac_scale = 1;
        double factor = 1;

        constexpr size_t point_pos = fs.sv().find_first_of('.');
        constexpr size_t exp_pos = fs.sv().find_first_of("eE");

        // Исключаем повторяющиеся точки и е
        if constexpr (fs.sv().find_first_of('.', point_pos + 1) != 
            std::string_view::npos) static_assert(false, "Invalid number format");
        if constexpr (fs.sv().find_first_of("eE", exp_pos + 1) != 
            std::string_view::npos) static_assert(false, "Invalid number format");
        
        // Показатель степени не может быть дробным 123е1.23
        if constexpr (point_pos != std::string_view::npos && 
            exp_pos < point_pos)
        {
            static_assert(false, "Invalid number format");
        }

        // Знак
        constexpr bool is_negative = fs.data[0] == '-';

        // Целая часть
        constexpr size_t whole_capacity = 
            ((size < point_pos) 
                ? ((size < exp_pos) ? size : exp_pos)
                : point_pos) + 1;
        constexpr fixed_string<whole_capacity> 
            whole_fs{fs.data, fs.data + whole_capacity - 1};
        whole = parse_value<whole_fs, int>();
        
        // Дробная часть
        if constexpr (point_pos < size)
        {
            // Минусы возможны только в начале либо числа, либо степени
            if constexpr (fs.data[point_pos + 1] == '-')
            {
                static_assert(false, "Invalid number format");
            }

            constexpr size_t frac_capacity = 
                ((size < exp_pos) ? size : exp_pos) - point_pos;
            constexpr fixed_string<frac_capacity> 
                frac_fs{&fs.data[point_pos + 1], 
                    &fs.data[point_pos + frac_capacity]};
            frac = parse_value<frac_fs, int>();
            
            for (size_t i = 0; i < frac_capacity - 1; ++i)
            {
                frac_scale *= 10;
            }
        }
        
        // Степень
        if constexpr (exp_pos < size)
        {
            constexpr size_t exp_capacity = size - exp_pos;
            constexpr fixed_string<exp_capacity> 
                exp_fs{&fs.data[exp_pos + 1], 
                    &fs.data[exp_pos + exp_capacity]};
            exp = parse_value<exp_fs, int>();

            for (size_t i = 0; i < std::abs(exp); ++i)
            {
                factor *= 10;
            }
        }
        
        double out = (exp > 0) 
            ? whole * factor + (is_negative ? -1 : 1) * frac / (frac_scale / factor)
            : whole / factor + (is_negative ? -1 : 1) * frac / (frac_scale * factor);
        return out;
    }

    /* Случай float */
    template <fixed_string fs, typename Float, 
        std::enable_if_t<std::is_same_v<Float, float>>* = nullptr>
    consteval float parse_value()
    {
        return (float)parse_value<fs, double>();
    }

    /* Случай string_view.  Ответ с expected для 
    единообразности с другими специализациями */
    template <fixed_string fs, typename StringView, 
        std::enable_if_t<std::is_same_v<StringView, std::string_view>>* = nullptr>
    consteval std::string_view parse_value() noexcept
    {
        return fs.sv();
    }

    //=== Форматировщики данных ===
    /* Попадание сюда возможно, только если 
    форматирующая буква не соответствует типу */
    template <char formatter, typename T, typename>
    consteval void format_value()
    {
        static_assert(false, "Type-format mismatch: "
            "%d for integer types, "
            "%u for unsigned integer types, "
            "%s for std::string_view, "
            "%f for floating point types");
    }

    template <char formatter, typename IntType, 
        std::enable_if_t<std::is_signed_v<IntType> && 
            formatter == 'd'>* = nullptr>
    consteval void format_value()
    {};

    template <char formatter, typename UIntType, 
        std::enable_if_t<std::is_signed_v<UIntType> && 
            formatter == 'u'>* = nullptr>
    consteval void format_value()
    {};

    template <char formatter, typename StringType, 
        std::enable_if_t<std::is_same_v<StringType, std::string_view> && 
            formatter == 's'>* = nullptr>
    consteval void format_value()
    {};

    template <char formatter, typename FloatType, 
        std::enable_if_t<std::is_floating_point_v<FloatType> && 
            formatter == 'f'>* = nullptr>
    consteval void format_value()
    {};

    /* Шаблонная функция, выполняющая преобразования исходных данных в 
    конкретный тип на основе I-го плейсхолдера */
    template <size_t I, format_string format, fixed_string source, typename Out>
    consteval Out parse_input() 
    {
        constexpr std::pair<size_t, size_t> source_pos = 
            get_parsing_boundaries<I, format, source>();
        constexpr std::pair<size_t, size_t> format_pos = 
            format.placeholder_positions[I];
        constexpr fixed_string<source_pos.second - source_pos.first + 1> 
            target{source.data + source_pos.first, 
                source.data + source_pos.second};
        
        /* Ни специализация шаблонов, ни requires не могут справиться с
        нахождением нужного обработчика, так что поступаем топорнее... */
        Out out = parse_value<target, Out>();
        
        // Случай наличия указаний по форматированию
        if constexpr (format_pos.second - format_pos.first > 2)
        {
            constexpr char format_c = 
                format.str.data[format_pos.first + 2];
            format_value<format_c, Out>();
        }

        return out;
    }
} // namespace stdx::internals