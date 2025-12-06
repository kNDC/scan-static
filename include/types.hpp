#pragma once

#include <cstddef>
#include <string_view>
#include <tuple>
#include <algorithm>

namespace stdx::internals
{
    // Шаблонный класс, хранящий C-style строку фиксированной длины
    template <typename CharT, size_t capacity>
    struct basic_fixed_string
    {
        // Хранилище данных
        CharT data[capacity]{};

        // Ёмкость включает ноль-терминатор, размер - нет.
        constexpr const static size_t size = capacity - 1;
        
        constexpr basic_fixed_string(const CharT (&cstr)[capacity])
        {
            std::copy_n(cstr, capacity, data);
        }

        template <size_t cstr_capacity>
        constexpr basic_fixed_string(const CharT (&cstr)[cstr_capacity])
        {
            if constexpr (capacity > cstr_capacity)
            {
                std::copy_n(cstr, cstr_capacity, data);
            }
            else
            {
                static_assert(false, "The c-string is too long "
                    "to fit in the basic_fixed_string");
            }
        }

        constexpr basic_fixed_string(const CharT* from, const CharT* to)
        {
            size_t pos = 0;
            while (pos < size && from < to)
            {
                data[pos++] = *from++;
            }

            data[pos] = '\0';
        }

        consteval static bool empty() { return !size; }
        consteval std::basic_string_view<CharT> sv() const { return { data, size }; }
    };

    template <size_t size>
    using fixed_string = basic_fixed_string<char, size>;

    template <size_t size>
    using fixed_wstring = basic_fixed_string<wchar_t, size>;

    /* Шаблонный класс, хранящий fixed_string 
    достаточной длины для хранения ошибки парсинга */
    constexpr const size_t PARSE_ERR_CAPACITY = 30;
    struct parse_error : fixed_string<PARSE_ERR_CAPACITY>
    {};

    // Шаблонный класс для хранения считанных переменных
    template <typename... Args>
    struct scan_result
    {
        const std::tuple<Args...> values;

        constexpr scan_result(Args&&... args) : 
            values{std::forward<Args>(args)...}
        {}
    };

    /* Вспомогательные сущности, создающие последовательность 
    естественных чисел I = {0; 1; 2; ...} для прогона parse_input<I...> */
    // Готовая последовательность индексов
    template <size_t... seq>
    struct indices
    {};

    // Накопитель последовательности индексов 
    template <size_t countdown, size_t... seq_tail>
    struct sequencer
    {
        using type = sequencer<countdown - 1, countdown - 1, seq_tail...>::type;
    };

    // Опора рекурсии с готовой последовательностью
    template <size_t... seq>
    struct sequencer<0, seq...>
    {
        using type = indices<seq...>;
    };

    /* Для перехода от длины последовательности к 
    самой сущности, её содержащей */
    template <size_t countdown>
    using generate_indices = sequencer<countdown>::type;
}  // namespace stdx::internals