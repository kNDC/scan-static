#include "scan.hpp"

void FixedString_Tests()
{
    using namespace stdx::internals;

    // Построение из C-строки
    {
        constexpr fixed_string fs{"1234"};
        static_assert(fs.size == 4);
        static_assert(fs.data[fs.size] == '\0');
    }

    // Теперь с явной ёмкостью
    {
        constexpr fixed_string<6> fs{"1234"};
        static_assert(fs.size == 5);
        static_assert(fs.data[4] == '\0');
    }

    /* Теперь по указателям - с явно заданным размером
    указатели могут заключать даже не влезающую строку */
    {
        constexpr std::array<char, 10> array = 
            []()
            {
                std::array<char, 10> out;
                for (size_t i = 0; i < 10; ++i) out[i] = i;
                return out;
            }();

        constexpr size_t capacity = 8;
        constexpr fixed_string<capacity> fs((const char*)&array[0], 
            (const char*)&array[9]);
        
        [&]<size_t... I>(indices<I...>)
        {
            static_assert((... && (fs.data[I] == I)));
        }(generate_indices<fs.size>{});

        static_assert(fs.data[fs.size] == '\0');
    }
}

void FormatString_Tests()
{
    using namespace stdx::internals;
    
    {
        constexpr format_string<"012345{%d} more text"> fs;
        static_assert(fs.n_placeholders == 1);
        static_assert(fs.placeholder_positions.size() == 1);
        static_assert(fs.placeholder_positions[0].first == 6);
        static_assert(fs.placeholder_positions[0].second == 9);
    }

    {
        constexpr format_string<"012345{%d} more text {}"> fs;
        static_assert(fs.n_placeholders == 2);
    }

    {
        constexpr format_string<"{}"> fs;
        static_assert(fs.n_placeholders == 1);
        static_assert(fs.placeholder_positions.size() == 1);
        static_assert(fs.placeholder_positions[0].first == 0);
        static_assert(fs.placeholder_positions[0].second == 1);
    }

    /* Не скомпилируется
    {
        constexpr format_string<"{"> fs;
    }
    */

    /* Не скомпилируется
    {
        constexpr format_string<"}"> fs;
    }
    */

    /* Не скомпилируется
    {
        constexpr format_string<"{{}}"> fs;
    }
    */

    /* Не скомпилируется
    {
        constexpr format_string<"{a}"> fs;
    }
    */
}

void Int_Parse_Tests()
{
    using namespace stdx::internals;
    constexpr format_string<"{}"> format;

    {
        constexpr fixed_string source{"3456"};
        constexpr int val = 
            parse_input<0, format, source, int>();
        static_assert(val == 3456);
    }

    {
        constexpr fixed_string source{"+1234567890"};
        constexpr int val = 
            parse_input<0, format, source, int>();
        static_assert(val == 1234567890);
    }

    {
        constexpr fixed_string source{"-1234567890"};
        constexpr int val = 
            parse_input<0, format, source, int>();
        static_assert(val == -1234567890);
    }

    {
        constexpr fixed_string source{"00001"};
        constexpr int val = 
            parse_input<0, format, source, int>();
        static_assert(val == 1);
    }

    /* Не скомпилируется
    {
        constexpr fixed_string source{"-1234e67890"};
        constexpr int val = 
            parse_input<0, format, source, int>();
    }
    */

    /* Не скомпилируется
    {
        constexpr fixed_string source{"--1234567890"};
        constexpr int val = 
            parse_input<0, format, source, int>();
    }
    */

    /* Не скомпилируется
    {
        constexpr fixed_string source{"-1234567890+"};
        constexpr int val = 
            parse_input<0, format, source, int>();
    }
    */
}

void Double_Parse_Tests()
{
    using namespace stdx::internals;
    constexpr format_string<"{}"> format;

    {
        constexpr fixed_string source{"123e4"};
        constexpr double val = 
            parse_input<0, format, source, double>();
        static_assert(std::abs(val - 123e4) < 1e-6);
    }

    {
        constexpr fixed_string source{"123E4"};
        constexpr double val = 
            parse_input<0, format, source, double>();
        static_assert(std::abs(val - 123e4) < 1e-6);
    }

    {
        constexpr fixed_string source{"+123.456e8"};
        constexpr double val = 
            parse_input<0, format, source, double>();
        static_assert(std::abs(val - 123.456e8) < 1e-6);
    }

    {
        constexpr fixed_string source{"-123456.789"};
        constexpr double val = 
            parse_input<0, format, source, double>();
        static_assert(std::abs(val + 123456.789) < 1e-6);
    }

    {
        constexpr fixed_string source{"-1234e"};
        constexpr double val = 
            parse_input<0, format, source, double>();
        static_assert(std::abs(val + 1234.0) < 1e-6);
    }

    {
        constexpr fixed_string source{"-.1234"};
        constexpr double val = 
            parse_input<0, format, source, double>();
        static_assert(std::abs(val + 0.1234) < 1e-6);
    }

    {
        constexpr fixed_string source{"-1.234e-2"};
        constexpr double val = 
            parse_input<0, format, source, double>();
        static_assert(std::abs(val + 1.234e-2) < 1e-6);
    }

    {
        constexpr fixed_string source{"-1.00234e-2"};
        constexpr double val = 
            parse_input<0, format, source, double>();
        static_assert(std::abs(val + 1.00234e-2) < 1e-6);
    }

    {
        constexpr fixed_string source{"-1.0123e-3f"};
        constexpr double val = 
            parse_input<0, format, source, double>();
        static_assert(std::abs(val + 1.0123e-3) < 1e-6);
    }

    /* Не скомпилируется
    {
        constexpr fixed_string source{"123..456"};
        constexpr int val = 
            parse_input<0, format, source, double>();
    }
    */

    /* Не скомпилируется
    {
        constexpr fixed_string source{"-123ee10"};
        constexpr int val = 
            parse_input<0, format, source, double>();
    }
    */

    /* Не скомпилируется
    {
        constexpr fixed_string source{"-123f10"};
        constexpr int val = 
            parse_input<0, format, source, double>();
    }
    */

    /* Не скомпилируется
    {
        constexpr fixed_string source{"-1234e5.67"};
        constexpr int val = 
            parse_input<0, format, source, double>();
    }
    */

    /* Не скомпилируется
    {
        constexpr fixed_string source{"-1234.-67"};
        constexpr int val = 
            parse_input<0, format, source, double>();
    }
    */
}

void StringView_Parse_Tests()
{
    using namespace stdx::internals;
    using namespace std::string_view_literals;
    
    constexpr format_string<"{}, bla-bla-bla"> format;
    constexpr fixed_string source{"lorem ipsum, bla-bla-bla"};

    constexpr std::string_view sv = 
        parse_input<0, format, source, std::string_view>();
    static_assert(sv == "lorem ipsum"sv);
}

void Composite_Parse_Tests()
{
    using namespace stdx::internals;
    using namespace std::string_view_literals;

    constexpr fixed_string source{"some text before 123456 "
        "more text after MYSTERY WORD "
        "and still more text here 3.14159265e-1"};
    constexpr format_string<"some text before {%d} "
        "more text after {%s} "
        "and still more text here {%f}"> format;
    
    {
        constexpr std::pair<const size_t, const size_t> pos = 
            get_parsing_boundaries<0, format, source>();
        static_assert(pos.first == 17);
        static_assert(pos.second == 23);
    }

    {
        constexpr std::pair<size_t, size_t> pos = 
            get_parsing_boundaries<1, format, source>();
        static_assert(pos.first == 40);
        static_assert(pos.second == 52);
    }

    {
        constexpr std::pair<size_t, size_t> pos = 
            get_parsing_boundaries<2, format, source>();
        static_assert(pos.first == 78);
        static_assert(pos.second == 91);
    }
    
    constexpr int val = parse_input<0, format, source, int>();
    static_assert(val == 123456);
    
    constexpr std::string_view sv = 
        parse_input<1, format, source, std::string_view>();
    static_assert(sv == "MYSTERY WORD"sv);
    
    constexpr double dbl_val = 
        parse_input<2, format, source, double>();
    static_assert(std::abs(dbl_val - 3.14159265e-1) < 1e-7);
}

void Scan_Tests()
{
    using namespace stdx;
    using namespace stdx::internals;
    using namespace std::string_view_literals;

    constexpr fixed_string source{"some text with 3 more "
        "placeholders in it: the first is int of 9876 "
        "the second is a string STRING "
        "the third is a double 2.718281828"};
    constexpr format_string<"some text with {} more "
        "placeholders in it: the first is int of {} "
        "the second is a string {} "
        "the third is a double {}"> format;
    
    {
        constexpr scan_result result = scan<format, source, 
            int, int, std::string_view, double>();
        static_assert(std::get<0>(result.values) == 3);
        static_assert(std::get<1>(result.values) == 9876);
        static_assert(std::get<2>(result.values) == "STRING"sv);
        static_assert(std::abs(std::get<3>(result.values) - 2.718281828) < 1e-7);
    }

    {
        constexpr scan_result result = scan<format, source, 
            int, std::string_view, std::string_view, double>();
        static_assert(std::get<1>(result.values) == "9876"sv);
    }

    {
        constexpr scan_result result = scan<format, source, 
            int, const int, std::string_view, double>();
        static_assert(std::get<1>(result.values) == 9876);
    }

    {
        constexpr scan_result result = scan<format, source, 
            int8_t, const int, std::string_view, double>();
        static_assert(std::get<0>(result.values) == 3);
    }

    {
        constexpr scan_result result = scan<format, source, 
            const int16_t, const int, std::string_view, double>();
        static_assert(std::get<0>(result.values) == 3);
    }

    /* Не скомипилируется
    {
        constexpr scan_result result = scan<format, source, 
            int, int&, std::string_view, double>();
    }
    */

    /* Не скомипилируется
    {
        constexpr scan_result result = scan<format, source, 
            int, int, std::string_view, double&&>();
    }
    */

    /* Не скомипилируется
    {
        constexpr scan_result result = scan<format, source, 
            int, int, std::string_view&, double>();
    }
    */

    /* Не скомипилируется из-за переполнения int8_t
    {
        constexpr scan_result result = scan<format, source, 
            int, int8_t, std::string_view, double>();
    }
    */

    /* Не скомипилируется
    {
        fixed_string runtime_source{"123456"};
        constexpr format_string<"{}"> format_;
        constexpr scan_result result = 
            scan<format_, runtime_source, int>();
    }
    */

    /* Не скомипилируется
    {
        constexpr fixed_string source_{"123456"};
        constexpr format_string<"{}"> runtime_format;
        constexpr scan_result result = 
            scan<runtime_format, source_, int>();
    }
    */
}

int main(int argc, char* argv[])
{
    FixedString_Tests();
    FormatString_Tests();

    Int_Parse_Tests();
    Double_Parse_Tests();
    StringView_Parse_Tests();
    Composite_Parse_Tests();
    
    Scan_Tests();
}