#include "FoldExpression.h"
#include "invoke_apply.h"

#include <algorithm>
#include <array>
#include <any>
#include <bitset>
#include <cassert>
#include <charconv>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <tuple>
#include <variant>



/*
 Сайты: 
 invoke, apply: http://scrutator.me/post/2018/05/25/cpp17_lang_features_p4.aspx
                https://habr.com/ru/companies/pvs-studio/articles/340014/
                https://habr.com/ru/companies/otus/articles/656363/
                https://stackoverflow.com/questions/38222029/variadic-members-in-non-template-class?rq=3
                https://www.vishalchovatiya.com/variadic-template-cpp-implementing-unsophisticated-tuple/
                https://www.itcodar.com/c-plus-1/template-tuple-calling-a-function-on-each-element.html
                https://medium.com/@raghavmnnit/variadic-templates-and-function-arguments-part2-1d35d06730d9
 */


struct [[nodiscard]] Example
{
    std::string _name;
    int _value = 0;
};

struct Location
{
    std::string city;
    std::string country;
};

struct Person 
{
    std::string name;
    uint32_t age;
    Location loc;
};

[[nodiscard]] std::unique_ptr<int> CreateNumber()
{
    return std::make_unique<int>(1);
}

[[nodiscard]] Example CreateExample()
{
    return { "Hello", 1 };
}

int DoExmapleCall()
{
    return 10;
}

std::ostream& operator<<(std::ostream& os, std::byte b)
{
    return os << std::bitset<8>(std::to_integer<int>(b));
}

/// static inline member class
namespace static_inline
{
    // До C++17
    namespace C14
    {
        struct X
        {
            static int number;
        };
        
        [[maybe_unused]] int X::number = 0;
    }

    // С++17
    namespace C17
    {
        struct Y
        {
            [[maybe_unused]] static inline int i = 0;
        };
    }
}

namespace SFINAE
{
    template <typename T>
    struct Number
    {
        Number(const T& number) : value(number) {}

        T value;
    };

    /*
      Для Number<int> не работает: при вызове. Эта функция пытается найти int.value, которого не существует. Ошибку: часть else не удалена из функции (error: request for member ‘value’ in ‘t’, which is of non-class type ‘const int’).
    */
    namespace IS_ARITHMETIC
    {
        template<typename T>
        T Square(const T& number)
        {
            if (std::is_arithmetic<T>::value)
            {
                return number * number;
            }
            else
            {
                return number.value * number.value;
            }
        }
    }

    /*
    * C++14: Чтобы решить проблему Number<int>, нужны два шаблона функций, которые проверяют передаваемый тип на арифметическу.
    */
    namespace ENABLE_IF
    {
        template<typename T>
        typename std::enable_if<std::is_arithmetic<T>::value, T>::type Square(const T& number)
        {
            return number * number;
        }

        template<typename T>
        typename std::enable_if<!std::is_arithmetic<T>::value, T>::type Square(const T& number)
        {
            return number.value * number.value;
        }

        // template<typename T>
        // typename std::enable_if<std::has_begin<T>::value, T>::type Square(const T& number)
    }

    /*
      C++17: Здесь используется только один шаблон функции (что упрощает код вместо std::enable_if).
      Компилятор берет только ветку с истинным условием (true) и отбрасывает другие.
    */
    namespace CONSTEXPR
    {
        template<typename T>
        T Square(const T& number)
        {
            if constexpr (std::is_arithmetic<T>::value) // Проверка нужна для класса Number, иначе ошибка: не найден operator*
            {
                return number * number;
            }
            else
            {
                return number.value * number.value;
            }
        }
    }
    /*
      C++20: Концепты компилируются быстрее обычного SFINAE (std::enable_if и constexpr) и условия в них можно расширять. Версия C++20 вернулась обратно к двум функциям, но теперь код намного читабельнее, чем с std::enable_if.
    */
    namespace CONCEPT
    {
        template<typename T>
        concept Arithmetic = std::is_arithmetic<T>::value;

        template <typename T>
        concept has_member_value = requires (const T& t)
        {
            std::is_arithmetic<decltype(T::value)>::value;
        };

        template<Arithmetic T>
        T Square(const T& number)
        {
            return number * number;
        }

        template<has_member_value T>
        T Square(const T& number)
        {
            return number.value * number.value;
        }
    }
}

namespace VARIANT
{
    template<class... Ts>
    struct overloaded : Ts... { using Ts::operator()...; };

    // explicit deduction guide (not needed as of C++20)
    template<class... Ts>
    overloaded(Ts...) -> overloaded<Ts...>;
}

int main()
{
    setlocale(LC_ALL, "Russian"); // Нужно сохранить файл с кодировкой Кириллица (Windows) - кодовая страница 1251
    int value = 5;
    
    /// Декомпозиция std::tuple, std::pair, std::map, struct
    {
        // До C++17
        {
            /// Игнорирование одного из параметров при декомпозиции
            int number1;
            std::tie(number1, std::ignore) = std::tuple(2, 3);
        }
        // С++17
        {
            [[maybe_unused]] auto [a, b, c] = std::tuple(1, "hello", 0.1);
            [[maybe_unused]] auto [value1, value2] = std::pair{"hello", 1};
            //auto [a, b] = std::map{ "hello", 1 };
            [[maybe_unused]] auto [title, year] = Example();
        }
    }
    /// static inline member class
    {
        using namespace static_inline;
        // До C++17
        {
            [[maybe_unused]] C14::X x;
        }
        // С++17
        {
            [[maybe_unused]] C17::Y y;
        }
    }
    /*
     Fold expression (выражение свертки) - шаблон с заранее неизвестным числом аргументов (variadic template). Свертка – это функция, которая применяет заданную комбинирующую функцию к последовательным парам элементов в списке и возвращает результат. Любое выражение свёртки должно быть заключено в скобки и в общем виде выглядит так: (выражение содержащее пачку аргументов). Выражение внутри скобок должно содержать в себе нераскрытую пачку параметров и один из следующих операторов:
     +  -  *  /  %  ^  &  |  =  <  >  <<  >>
     +=  -=  *=  /=  %=  ^=  &=  |=  <<=  >>=
     ==  !=  <=  >=  &&  ||  ,  .*  ->*
     
     До C++17: для развёртывания пачки параметров требуется рекурсивное инстанциирование шаблонов с помощью stub-функции (заглушка)
     */
    {
        using namespace fold_expression;
        std::vector<int> numbers;
        
        [[maybe_unused]] auto sum_result1 = Sum(1, 2, 3);
        [[maybe_unused]] auto average_result = Average(1, 2, 3);
        [[maybe_unused]] auto norm_result = Norm(1, 2, 3);
        [[maybe_unused]] auto pow_sum_result = Pow_Sum(1, 2, 3);
        Push_To_Vector(numbers, 1, 2, 3, 4, 5);
        [[maybe_unused]] auto countArguments = CountArgs(1, "hello", 2.f);
        [[maybe_unused]] auto countTypes = CountTypes(1, "hello", 2.f);
        CheckTypes(int(1), std::string("hello"), double(2.0));
    }
    /*
     lambda - может быть constexpr, но с C++20 идет по-умолчанию, так что писать необязательно
     */
    {
        constexpr int y = 32;
        constexpr auto answer = [y]()
        {
            constexpr int x = 10;
            return x + y;
        };
        
        [[maybe_unused]] auto result = answer();
    }
    // SFINAE
    {
        using namespace SFINAE;
        
        int integer_num = 5;
        float floating_num = 5.0;
        bool boolean = true;
        Number<int> number_int(5);
        Number<float> number_float(5.0f);
        Number<float> number_boolean(true);
        
        // C++14: std::enable_if
        {
            using namespace SFINAE;
            
            [[maybe_unused]] auto square1 = ENABLE_IF::Square(integer_num); // Вызов int Square(int);
            [[maybe_unused]] auto square2 = ENABLE_IF::Square(floating_num); // Вызов float Square(float);
            [[maybe_unused]] auto square3 = ENABLE_IF::Square(boolean);  // Вызов bool Square(bool);
            [[maybe_unused]] auto square4 = ENABLE_IF::Square(number_int); // Вызов Number<int> Square(Number<int>);
            [[maybe_unused]] auto square5 = ENABLE_IF::Square(number_float); // Вызов Number<float> Square(Number<float>);
            [[maybe_unused]] auto square6 = ENABLE_IF::Square(number_boolean); // Вызов Number<bool> Square(Number<bool>);
        }
        // C++17: constexpr
        {
            [[maybe_unused]] auto square1 = CONSTEXPR::Square(integer_num); // Вызов int Square(int);
            [[maybe_unused]] auto square2 = CONSTEXPR::Square(floating_num); // Вызов float Square(float);
            [[maybe_unused]] auto square3 = CONSTEXPR::Square(boolean);  // Вызов bool Square(bool);
            [[maybe_unused]] auto square4 = CONSTEXPR::Square(number_int); // Вызов Number<int> Square(Number<int>);
            [[maybe_unused]] auto square5 = CONSTEXPR::Square(number_float); // Вызов Number<float> Square(Number<float>);
            [[maybe_unused]] auto square6 = CONSTEXPR::Square(number_boolean); // Вызов Number<bool> Square(Number<bool>);
        }
        // C++20: concept
        {
            [[maybe_unused]] auto square1 = CONCEPT::Square(integer_num); // Вызов int Square(int);
            [[maybe_unused]] auto square2 = CONCEPT::Square(floating_num); // Вызов float Square(float);
            [[maybe_unused]] auto square3 = CONCEPT::Square(boolean);  // Вызов bool Square(bool);
            [[maybe_unused]] auto square4 = CONCEPT::Square(number_int); // Вызов Number<int>
            [[maybe_unused]] auto square5 = CONCEPT::Square(number_float); // Вызов Number<float> Square(Number<float>);
            [[maybe_unused]] auto square6 = CONCEPT::Square(number_boolean); // Вызов Number<bool>Square(Number<bool>);
        }
    }
    /*
     Тип std::byte - является более типобезопасным, чем char, unsigned char или uint8_t. К std::byte можно применить только побитовые операции, а арифметические операции и неявные преобразования недоступны. Использовать std::byte при работе с 'сырой' памятью, когда хранилище представляет собой просто последовательность байтов, а не массив символов

     Определение типа std::byte:
     enum class byte : unsigned char {};
     */
    {
        std::cout << "std::byte" << std::endl;
        std::byte b {42};
        std::cout << "1. " << b << '\n';
        // b *= 2 // Ошибка
        b <<= 1;
        std::cout << "2. " << b << '\n';
        b >>= 1;
        std::cout << "3. " << b << '\n';
        std::cout << "4. " << (b << 1) << '\n';
        std::cout << "5. " << (b >> 1) << '\n';
    }
    /*
     std::string_view - это просто пара значений, указатель на последовательность и размер. string_view уместен везде, где не требуется копия. Так как string_view не является владельцем данных, то если строка исчезнет, то не будет информации, что std::string_view перестал быть валидным. Время жизни string_view не зависит от времени жизни строки, которую он представляет. Однако, если отображаемая строка выйдет за пределы области видимости, то string_view больше не сможет её отображать и при попытке доступа получится неопределенный результат. string_view не гарантирует наличие нулевого символа на конце. Можно использовать инициализировать во время компиляции constexpr, в отличии string
     remove_prefix и remove_suffix, которые отсекают от видимого диапазона string_view заданное число символов с начала или с конца; исходная строка не меняется, а меняется только наблюдаемый диапазон.
     В параметрах всех функций и методов вместо const string& следует использовать невладеющий string_view, но возвращать владеющий string.
     */
    {
        auto String = [&]() -> std::string_view
        {
            std::string str("hello");
            std::string_view str_view(str);
            return str_view;
        };
        
        auto String_View = [&]() -> std::string_view
        {
            std::string_view str_view("hello");
            return str_view;
        };
        
        std::string_view string_result = String(); // Выход за области видимости
        std::cout << string_result << std::endl; // Выведет мусор
        
        std::string_view string_view_result = String_View(); // string_view владеет строкой
        std::cout << string_view_result << std::endl; // Выведет hello
        
        std::string str("hello");
        std::string_view str_view(str);

        str_view.remove_prefix(1); // Игнорируем первый символ
        str_view.remove_suffix(2); // Игнорируем последние 2 символа

        str = "example"; // strView будет теперь хранить "xa" без первого и двух последних символов с учетом изменения размера с 5 до 2
    }
    /*
     std::variant - тип данных, который умеет хранить и объединять в себе несколько типов данных.
     Он позволяет переиспользовать одну и ту же область памяти для хранения разных полей типов данных без выделения дополнительной памяти.
     std::variant является более безопасным, чем union или enum т.к. задействован в stl, поэтому может предупреждать пользователя об ошибках.
     std::variant и std::optional не требует никаких дополнительных выделений памяти, но это вызвано тем, что типы хранимых в объекте данных заранее известны, std::any не имеет такой информации, поэтому может использовать дополнительную память.
     std::get<Type>(...) бросает исключение, если тип внутри variant не совпадает с ожидаемым, а иначе возвращает ссылку на запрошенный тип.
     std::get_if<Type>(...), которая возвращает указатель на запрошенный тип, если тип внутри variant совпадает с ожидаемым, и возвращает nullptr в противном случае.
     std::visit - позволяет извлекать данные из std::variant с помощью mixIn (примеси) + lambda или constexpr (SFINAE).
     Примеси (mixIns) - это variadic CRTP, где шаблонный класс с заранее неизвестным числом аргументов наследуется от них.
     */
    {
        [[maybe_unused]] std::variant<int, float> int_float {std::in_place_type<int>, 10.5}; // явное приведение double к int
        std::variant<std::vector<char>, std::string> str {std::in_place_index<0>, {'h', 'e', 'l', 'l', 'o'}}; // запись в нулевой индекс std::vector<char>
        std::variant<std::string, int> age; // возраст, который может быть реализован строковой датой или целочисленным значением
        age = value;
        std::vector<std::variant<int, double, std::string>> vec = {10, 10.0, "str"};
        
        /// Извлечение данных
        {
            /// 1 Способ: сложный
            {
                auto ageIntPointer = std::get_if<int>(&age); // указатель на тип int
                auto ageStringPointer = std::get_if<std::string>(&age); // указатель на тип std::string
                [[maybe_unused]] auto ageInt = std::get<int>(age); // нужно заранее знать какой тип: значение на тип int
                // auto ageString = std::get<std::string>(age); // ошибка, значение на тип std::string отсутствует

                if (ageIntPointer)
                {
                    
                }
                else if (ageStringPointer)
                {
                    
                }
            }
            /// 2 Способ: std::visit
            {
                using namespace VARIANT;
                /// lambda
                {
                    for (const auto& v : vec)
                    {
                        std::visit(overloaded{[](int number1, int number2) { std::cout << "int: " << number1 << " " << number2 << std::endl; },
                                              [](double number1, double number2) { std::cout << "double: " << number1 << " " << number2 << std::endl; },
                                              [](int number1, auto number2)  { std::cout << "float: " << number1 << " " << number2 << std::endl; },
                                              [](double number1, auto number2)  { std::cout << "float: " << number1 << " " << number2 << std::endl; },
                                              [&](auto... values) { std::cout << "other types: "; ((std::cout << values << " "), ...) << std::endl; }
                        }, v);
                    }
                }
                /// constexpr (SFINAE)
                {
                    for (const auto& v : vec)
                    {
                        std::visit([&](auto&& arg)
                        {
                            using T = std::decay_t<decltype(arg)>;
                            if constexpr (std::is_same_v<T, int>)
                            {
                                std::cout << "int: " << arg << std::endl;
                            }
                            else if constexpr (std::is_same_v<T, double>)
                            {
                                std::cout << "double: " << arg << std::endl;
                            }
                            else if constexpr (std::is_same_v<T, std::string>)
                            {
                                std::cout << "string: " << arg << std::endl;
                            }
                        }, v);
                    }
                }
            }
        }
    }
    /*
     std::optional - обертка, которая может содержать значение или быть нулем типом(nullopt). Не требует использование выделение динамической памяти.
     std::variant и std::optional не требует никаких дополнительных выделений памяти, но это вызвано тем, что типы хранимых в объекте данных заранее известны, std::any не имеет такой информации, поэтому может использовать дополнительную память.
     С помощью std::optional можно проверить содержится/не содержится значение с помощью неявного приведения к bool (true, false, nullptr), работает с объектами и использовать методы emplace, reset, swap и assign.
     has_value() - проверка значения на nullptr
     value() - значение std::optional
     emplace() - передает значение по ссылке
     reset() - сброс значения
     */
    {
        std::optional<int> flag = value;
        
        /// Пример 1
        {
            if (flag) // проверка nullptr
            {
                if (flag == 5) // проверка на значение 5
                    value = 10;
            }
        }
        /// Пример 2
        {
            if (flag.has_value()) // проверка nullptr
            {
                if (flag.value() == 5) // проверка на значение 5
                    value = 10;
            }
        }
        /// Пример 3. Сравнение с числом. Если есть число, то возвращает это число. Если его нет, то возвращает сравниваемое число
        {
            std::cout << flag.value_or(5) << std::endl; // вернет 10, число присутствует
            std::cout << flag.value_or(5) << std::endl; // вернет 10, число присутствует
            flag.reset();
            std::cout << flag.value_or(6) << std::endl; // вернет 11, число отсутсвует
        }
        /// Пример 4: для передачи значения по ссылке, если значение = nullptr
        {
            flag.reset();
            auto lambda = [](int& number)
            {
                number = 10;
            };

            lambda(flag.emplace());
        }
        /// Пример 5: для передачи значения по ссылке, если значение != nullptr
        {
            // flag.reset();
            auto lambda = [](int& number)
            {
                number = 5;
            };

            lambda(flag.value());
        }
    }
    /// Получение неконстантной ссылки на внутренние данные std::string с помощью метода data(). Однако data() вернет строку без нуля в конце
    {
        std::string str = "hello";
        /// Старый способ до C++17
        {
            assert(!str.empty()); // для пустой строки str[0] недопустим, поэтому требуем непустую строку
            [[maybe_unused]] char* data = &str[0];
        }
        /// Новый способ в C++17
        {
            [[maybe_unused]] char* data = str.data();
        }
    }
    // Хранение любого типа данных
    {
        /*
         до C++, void* — тип указателя, который может указывать на объекты любого типа данных. Однако, указатель типа void* сам не знает,
         на объект какого типа он будет указывать, разыменовать его напрямую не получится, для этого нужно будет явно преобразовать
         указатель типа void с помощью оператора static_cast или reinterpret_cast в другой тип данных, а затем уже его разыменовать.
         Является не типобезапасным указателем.
         */
        {
            void* any = static_cast<void*>(&value);
            value = *reinterpret_cast<int*>(any); // 1 способ приведения
            value = *static_cast<int*>(any); // 2 способ приведения
        }
        /*
         C++17: В отличие от void* std::any владеет и управляет жизнью объекта (создание, копирование, перемещение и уничтожение).
         std::any может хранить информацию любого типа и сообщать об ошибке (бросать исключение) при попытке получить доступ, указав не тот тип. Предпочтительнее использовать std::variant.
         std::variant и std::optional не требует никаких дополнительных выделений памяти, но это вызвано тем, что типы хранимых в объекте данных заранее известны, std::any не имеет такой информации, поэтому может использовать дополнительную память.
         std::any использовать только в тех случаях, когда надо хранить или передавать какие-то данные неизвестного типа.
         std::any_cast<Type>(...) - приведение к типу
         type() - тип
         val.type().name() - имя типа
         has_value() - проверка значения на nullptr
         reset() - сброс значения
        */
       {
           /// Пример 1: без проверки на тип
           {
               std::any any = 42;
               std::cout << std::any_cast<int>(any) << std::endl;
               any = 11.34f;
               std::cout << any.type().name() << std::any_cast<float>(any) << std::endl;
               any = "hello";
               try
               {
                   std::cout << std::any_cast<std::string>(any) << '\n';
               }
               catch (const std::bad_any_cast& exeption)
               {
                   std::cout << exeption.what() << std::endl;
               }
           }
           /// Пример 2: с проверкой на тип
           {
               std::map<std::string, std::any> map;
               map["integer"] = 10;
               map["string"] = std::string("Hello World");
               map["float"] = 1.0f;

               for (auto& [_, val] : map)
               {
                   if (val.type() == typeid(int))
                       std::cout << val.type().name() << ": " << std::any_cast<int>(val) << std::endl;
                   else if (val.type() == typeid(std::string))
                       std::cout << val.type().name() << ": " << std::any_cast<std::string>(val) << std::endl;
                   else if (val.type() == typeid(float))
                       std::cout << val.type().name() << ": " << std::any_cast<float>(val) << std::endl;
               }
           }
           /// Пример 3: с проверкой на тип с помощью привидения к указателю
           {
               auto a = std::any(12);
               if (auto s = std::any_cast<std::string>(&a))
               {
                   std::cout << "a is std::string: " << *s << std::endl;
               }
               else if (int* i = std::any_cast<int>(&a))
               {
                   std::cout << "a is int: " << *i << std::endl;
               }
               else
               {
                   std::cout << "a is another type or unset" << std::endl;
               }
           }
       }
    }
    /// Атрибуты nodiscard, fallthrough, maybe_unused
    {
        /// nodiscard - Атрибут, который заставляет использовать возвращаемые значения функций
        {
            /// 1 способ
            {
                //CreateNumber(); // warning C4834 : отмена возвращаемого значения функции с атрибутом "nodiscard"
                auto number = CreateNumber(); // присываеваем значение
            }
            /// 2 способ
            {
                //CreateExample(); // warning C4834 : отмена возвращаемого значения функции с атрибутом "nodiscard"
                auto example = CreateExample(); // присываеваем значение
            }

            /// 3 способ - игнорирование результата
            {
                std::ignore = CreateNumber();
                std::ignore = CreateExample();
            }
        }
        /// fallthrough - атрибут композиции switch case, который указывает, что оператор break был намеренно пропущен и это не является ошибкой
        {
            switch (value)
            {
                case 0:
                    [[fallthrough]]; // специально пропускаем
                case 50:
                    [[fallthrough]]; // специально пропускаем
                case 100:
                    /// какое-то условие
                    break;
                default:
                    break;
            }
        }
        /// maybe_unused - атрибут, подавляющий предупреждение о неиспользованной переменной
        {
            /// Старый способ
            {
                auto result = DoExmapleCall();
                (void)result; // гасим предупреждение об unused variable
                assert(result > 0);
            }

            /// Новый способ
            {
                [[maybe_unused]] auto result = DoExmapleCall();
                assert(result > 0);
            }
        }
    }
    /// std::clamp - обрезает значение, если оно не попадает в определенный диапазон либо сверху, либо снизу
    {
        const int min = 10;
        const int max = 100;
        std::cout << std::clamp(0, min, max) << std::endl; // не удовлетворяет условию >= 10 && <= 100, поэтому устанавливается минимальное значение
        std::cout << std::clamp(50, min, max) << std::endl; // удовлетворяет условию >= 10 && <= 100
        std::cout << std::clamp(120, min, max) << std::endl; // не удовлетворяет условию >= 10 && <= 100, поэтому устанавливается максимальное значение
    }
    /*
     std::to_chars/std::from_chars - функции аналогичны std::to_string/std::atoi,std::stoi, но не требующие выделении динамической памяти и поддерживающие обработку ошибок. Он более гибок со значениями с плавающей запятой(1e-09), чем std::string, но требующий заранее выделенный буфер.
     Возвращает указатель на конец записанных символов в буфере, std::errc - в случае ошибки
     */
    {
        /// Пример 1
        {
            std::array<char, 10> str;
            if (auto [p, ec] = std::to_chars(str.data(), str.data() + str.size(), value); ec == std::errc())
            {
                std::cout << std::string_view(str.data(), p - str.data());
            }
        }

        /// Пример 2
        {
            std::string str = "101";
            int result;
            if (auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), result); ec == std::errc())
            {
                std::cout << result << std::endl;
            }
        }
    }
    /*
    std::partition - Переупорядочивает элементы в диапазоне[first, last): при выполенении условия true смещаются влево, иначе false.
    return - Итератор к первому элементу второй группы
    */
    {
        std::vector<int> numbers = { 0,1,2,3,4,5,6,7,8,9 };
        std::cout << "Original vector:\n";
        for (int num : numbers) 
            std::cout << num << ' ';
        std::cout << std::endl;

        auto it = std::partition(numbers.begin(), numbers.end(), [](int i) {return i % 2 == 0; });
        std::cout << "Middle after Partition: " << *it << std::endl;
        for (int num : numbers) 
            std::cout << num << ' ';
        std::cout << std::endl;
    }
    // invoke & apply
    {
        using namespace invoke_apply;
        
        int number1 = 1;
        int number2 = 2;
        Print example {10};
        std::tuple tuple {1, 1.5, 'c', "str"};
        
        /*
         std::invoke - шаблонная функция, оборачивающая и вызывающая ЛЮБОЙ объект с аргументами (ссылки, указатели, члены/функции-члены класса/структуры), в отличие от std::function и без SFINAE (constexpr).
         */
        {
            std::cout << "std::invoke" << std::endl;
            
            // 1 Способ: обычный
            {
                std::cout << "1 Способ: обычный" << std::endl;
                
                std::invoke(print<int, int>, 1, 2);
                std::invoke(print<int, int>, number1, number2);
                std::invoke(Print(), 1, 2);
                std::invoke(Print(), number1, number2);
                std::invoke(&Print::print<int, int>, example, 1, 2);
                std::invoke(&Print::print<int, int>, example, number1, number2);
                std::invoke(&Print::SetValue, &example, number2);
                [[maybe_unused]] auto value = std::invoke(&Print::GetValue, example);
                
                std::cout << std::endl;
            }
            // 2 Способ: lambda
            {
                std::cout << "2 Способ: lambda" << std::endl;
                
                std::invoke([&](){ print(1, 2); });
                std::invoke([&](){ print(number1, number2); });
                std::invoke([&](){ Print()(1, 2); });
                std::invoke([&](){ Print()(number1, number2); });
                std::invoke([&](){ example.print(1, 2); });
                std::invoke([&](){ example.print(number1, number2); });
                std::invoke([&](){ example.SetValue(number2); });
                [[maybe_unused]] auto value = std::invoke([&](){ return example.GetValue(); });
                
                std::cout << std::endl;
            }
            // 3 Способ: in template function
            {
                std::cout << "3 Способ: in function" << std::endl;
                
                CallInvoke(print<int, int>, 1, 2);
                CallInvoke(print<int, int>, number1, number2);
                CallInvoke(Print(), 1, 2);
                CallInvoke(Print(), number1, number2);
                CallInvoke(&Print::print<int, int>, example, 1, 2);
                CallInvoke(&Print::print<int, int>, example, number1, number2);
                CallInvoke(&Print::SetValue, &example, number2);
                [[maybe_unused]] auto value = CallInvoke(&Print::GetValue, example);
                
                std::cout << std::endl;
            }
        }
        /*
         std::apply - шаблонная функция, аналог std::invoke, но принимающая аргументы в качестве кортежа (std::tuple) и разворачивающая с помощью функции std::get<> Использование: когда нужно сохранить элементы в кортеж и использовать их позже (Например, в классе сохранять).
         */
        {
            std::cout << "std::apply" << std::endl;
            
            // 1 Способ: обычный
            {
                std::cout << "1 Способ: обычный" << std::endl;
                
                std::apply(print<int, int>, std::tuple{1, 2});
                std::apply(print<int, int>, std::tuple{number1, number2});
                std::apply(print<int, double, char, std::string>, tuple);
                std::apply(Print(), std::tuple{1, 2});
                std::apply(Print(), std::tuple{number1, number2});
                std::apply(Print(), tuple);
                std::apply(&Print::print<int, int>, std::tuple{example, 1, 2});
                std::apply(&Print::print<int, int>, std::tuple{example, number1, number2});
                std::apply(&Print::SetValue, std::tuple{example, number2});
                [[maybe_unused]] auto value = std::tuple(&Print::GetValue, std::tuple{example});
                
                std::cout << std::endl;
            }
            // 2 Способ: lambda
            {
                std::cout << "2 Способ: lambda" << std::endl;
                
                std::apply([](auto&& ...args)
                {
                    ((std::cout << args << ", "), ...);
                    std::cout << std::endl;
                }, std::tuple{1, 2});
                
                std::apply([&](auto&& ...args)
                {
                    print(std::forward<decltype(args)>(args)...);
                }, std::tuple{number1, number2});
                
                std::apply([&](auto&& ...args)
                {
                    print(std::forward<decltype(args)>(args)...);
                }, tuple);
                
                std::apply([&](auto&& ...args){ Print()(std::forward<decltype(args)>(args)...); }, std::tuple{1, 2});
                std::apply([&](auto&& ...args){ Print()(std::forward<decltype(args)>(args)...); }, std::tuple{number1, number2});
                std::apply([&](auto&& ...args){ Print()(std::forward<decltype(args)>(args)...); }, tuple);
                std::apply([&](auto&& ...args){ example.print(std::forward<decltype(args)>(args)...); }, std::tuple{1, 2});
                std::apply([&](auto&& ...args){ example.print(std::forward<decltype(args)>(args)...); }, std::tuple{number1, number2});
                std::apply([&](auto&& ...args){ example.SetValue(std::forward<decltype(args)>(args)...); }, std::tuple{number2});
                [[maybe_unused]] auto value = std::tuple([&](){ return example.GetValue(); });
                
                std::cout << std::endl;
            }
            // 3 Способ: in function
            {
                std::cout << "3 Способ: in function" << std::endl;
                
                CallApply(print<int, int>, std::tuple{1, 2});
                CallApply(print<int, int>, std::tuple{number1, number2});
                CallApply(print<int, double, char, std::string>, tuple);
                CallApply(Print(), std::tuple{1, 2});
                CallApply(Print(), std::tuple{number1, number2});
                CallApply(Print(), tuple);
                CallApply(&Print::print<int, int>, std::tuple{&example, 1, 2});
                CallApply(&Print::print<int, int>, std::tuple{&example, number1, number2});
                CallApply(&Print::SetValue, std::tuple{&example, number2});
                [[maybe_unused]] auto value = CallApply(&Print::GetValue, std::tuple{&example});
                
                std::cout << std::endl;
            }
        }
    }
    /*
     std::scoped_lock - это улучшенная версия lock_guard, конструктор которого делает захват (lock) произвольного кол-во мьютексов в очередном порядке и высвобождает (unlock) при выходе из стека в деструкторе, использование идиомы RAII. Решает проблему deadlock (взаимной блокировки).
     */
    {
        std::cout << "Deadlock" << std::endl;
        std::mutex mutex1;
        std::mutex mutex2;
        
        auto function1 = [&]()
            {
                std::scoped_lock scoped_lock(mutex1, mutex2);
                std::this_thread::sleep_for(std::chrono::milliseconds(10)); // задержка, чтобы thread2 успел сделать lock в mutex2 в function2
            };
        auto function2 = [&]()
            {
                std::scoped_lock scoped_lock(mutex2, mutex1); // порядок неважен
                std::this_thread::sleep_for(std::chrono::milliseconds(10)); // задержка, чтобы thread1 успел сделать lock в mutex1
            };
        std::thread thread1(function1);
        std::thread thread2(function2);

        thread1.join();
        thread2.join();
    }

    return 0;
}
