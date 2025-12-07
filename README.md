# scan-static <!-- omit in toc -->

Статическое типобезопасное исполнение функции-обработчика форматированных строк scan в C++23.

## Общее описание функции

### Сигнатура функции:

```C++
template <format_string format, fixed_string source, Ts...>
[[nodiscard]] consteval scan_result<Ts...> scan()
```

### Аргументы

Ввиду статичности, функция получает аргументы через следующие шаблонные параметры:
1. Форматирующая строка с наборами фигурных скобок на месте извлекаемых значений.  Фигурные скобки могут заключать форматирующие спецификаторы по примеру своего вдохновителя `scanf` из C: `%d` соответствует целочисленной переменной (`int` `int8_t`, `int16_t`, `int32_t`, `int64_t`), `%u` - натуральному числу (`unsigned int` `uint8_t`, `uint16_t`, `uint32_t`, `uint64_t`), `%f` - числу с плавающей точкой (`float`, `double`), `%s` - `std::string_view`. Типом строки является `format_string` -- шаблонная сущность, параметризуемая классом `fixed_string` (см. ниже);
2. Исходная строка-источник извлекаемых значений переменных. Типом строки является `fixed_string` -- `struct`, конструирующийся из постоянных C-строк `char[N]`;
3. Вариативный набор типов ожидаемых переменных, содержащих извлечённые значения;

### Возвращаемое значение
Тип - `scan_result<Ts...>`, содержит поле `std::tuple<Ts...> value`, в котором содержатся считанные переменные.


## Пример использования

```C++
constexpr fixed_string source{"some text with 3 more "
    "placeholders in it: the first is int of 9876 "
    "the second is a string STRING "
    "the third is a double 2.718281828"};

constexpr format_string<"some text with {} more "
    "placeholders in it: the first is int of {} "
    "the second is a string {} "
    "the third is a double {}"> format;

constexpr scan_result result = scan<format, source, 
  int, int, std::string_view, double>();

static_assert(std::get<0>(result.values) == 3);
static_assert(std::get<1>(result.values) == 9876);
static_assert(std::get<2>(result.values) == "STRING"sv);
static_assert(std::abs(std::get<3>(result.values) - 2.718281828) < 1e-7);
```

Особо стоит обратить внимание, что шаблонным параметром `formatted_string` выступает `fixed_string`, также используемая для передачи форматируемой строки в `scan`.

## Ограничения и ошибки

1. `scan` поддерживает следующие типы переменных: `int` `int8_t`, `int16_t`, `int32_t`, `int64_t`, `unsigned int` `uint8_t`, `uint16_t`, `uint32_t`, `uint64_t`, `float`, `double`, `std::string_view`;
2. Использование ссылочных типов в вариативном шаблонном наборе `Ts...` приведёт к ошибке компиляции;
3. Использование форматируюших спецификаторов помимо `%d`, `%u`, `%f`, `%s` приведёт к ошибке компиляции;
4. Несовпадение типов с соответствующими форматирующими спецификаторами приведёт к ошибке компиляции;
5. Ошибки форматирования чисел приведут к ошибке компиляции;
6. Ошибки в проставлении скобок в форматирующей строке приведёт к ошибке компиляции;
7. Попытка использования переменных времени исполнения (без `constexpr`) приведёт к ошибке компиляции;
