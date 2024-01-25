#include <iostream>
#include <bitset>
#include <cassert>
#include <optional>
#include <variant>
#include <algorithm>
#include <array>
#include <any>
#include <charconv>
#include <map>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>


// Сайты: http://scrutator.me/post/2018/05/25/cpp17_lang_features_p4.aspx


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


int main()
{
    setlocale(LC_ALL, "Russian"); // Нужно сохранить файл с кодировкой Кириллица (Windows) - кодовая страница 1251
    int value = 5;
    
    /// Декомпозиция std::tuple, std::pair, std::map, struct
    auto [a, b, c] = std::tuple(1, "hello", 0.1);
    auto [value1, value2] = std::pair{"hello", 1};
    //auto [a, b] = std::map{ "hello", 1 };
    auto [title, year] = Example();
    
    /// Игнорирование одного из параметров при декомпозиции
    int number1;
    std::tie(number1, std::ignore) = std::tuple(2, 3);
    
    /*
     Тип std::byte - является более типобезопасным, чем char, unsigned char или uint8_t. К std::byte можно применить только побитовые операции, а арифметические операции и неявные преобразования недоступны. Использовать std::byte при работе с 'сырой' памятью, когда хранилище представляет собой просто последовательность байтов, а не массив символов

     Определение типа std::byte:
     enum class byte : unsigned char {};
     */
    {
        std::cout << "std::byte" << std::endl;
        std::byte b {42};
        std::cout << "1. " << b << '\n';
//         b *= 2 // Ошибка
        b <<= 1;
        std::cout << "2. " << b << '\n';
        b >>= 1;
        std::cout << "3. " << b << '\n';
        std::cout << "4. " << (b << 1) << '\n';
        std::cout << "5. " << (b >> 1) << '\n';
    }

    /*
    * Агрегатная инициализация (композиция или Структурированные привязки)  — форма инициализации списка для массивов и типов классов (часто структур и объединений), со следующими характеристиками:
    1) Отсутствие закрытых или защищенных членов
    2) Отсутствие заданных пользователем конструкторов кроме явно заданных по умолчанию или удаленных конструкторов
    3) Отсутствие базовых классов
    4) Отсутствие виртуальных функций-членов, статичных членов
    */
    {
        /// 1 способ: инициализация всех членов структуры
        {
            Location location{ "Moscow", "Russia" };
        }
        /// 2 способ: инициализация только первого члена структуры
        {
            Person person1{ "stas" };
        }
        /// 3 способ: инициализация вложенной структуры
        {
            Person person2{ "mike", 50, {"Newcastle", "UK"} };
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

        /// fallthrough - атрибут композиции switch case, который указывает, что оператор break был намееренно пропущен и это не является ошибкой
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

    /// std::string_view является интерфейсом строки, а не владением данных, аналогичный ссылки(const std::string&). Он уместен везде, где вы будете просто анализировать строку и не требуется копия. Время жизни std::string_view не зависит от времени жизни строки, которую он представляет. 
    /// Однако, если отображаемая строка выйдет за пределы области видимости, то std::string_view больше не сможет её отображать и при попытке доступа получится неопределенный результат. std::string_view не гарантируется наличие нулевого символа на конце.
    /// Размер string_view равен двум размерам указателя.
    /// remove_prefix и remove_suffix, которые отсекают от видимого диапазона string_view заданное число символов с начала или с конца; исходная строка не меняется, а меняется только наблюдаемый диапазон.
    /// В параметрах всех функций и методов вместо const string& следует использовать невладеющий string_view, но возвращать владеющий string.
    {
        std::string str{ "hello" };
        std::string_view strView{ str };

        strView.remove_prefix(1); // Игнорируем первый символ
        strView.remove_suffix(2); // Игнорируем последние 2 символа

        str = "example"; // strView будет теперь хранить "xa" без первого и двух последних символов с учетом изменения размера с 5 до 2
    }

    /// std::optional - обертка, которая может содержать значение или быть нулем типом(nullopt). Не требует использование выделение динамической памяти.
    /// std::variant и std::optional не требует никаких дополнительных выделений памяти, но это вызвано тем, что типы хранимых в объекте данных заранее известны. std::any не имеет такой информации, поэтому может использовать дополнительную память.
    /// С помощью std::optional можно проверить содержится/не содержится значение с помощью неявного приведения к bool (true, false, nullptr), работает с объектами и использовать методы emplace, reset, swap и assign.
    /// has_value() - проверка значения на nullptr
    /// value() - значение std::optional
    /// emplace() - передает значение по ссылке
    /// reset() - сброс значения
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
            //flag.reset();
            auto lambda = [](int& number)
            {
                number = 5;
            };

            lambda(flag.value());
        }
    }

    /// std::variant - тип данных, который умеет хранить и объединять в себе несколько типов данных. 
    /// Он позволяет переиспользовать одну и ту же область памяти для хранения разных полей типов данных без выделения дополнительной памяти. 
    /// std::variant является более безопасным, чем union или enum т.к. задействован в stl, поэтому может предупреждать пользователя об ошибках. 
    /// std::variant и std::optional не требует никаких дополнительных выделений памяти, но это вызвано тем, что типы хранимых в объекте данных заранее известны. std::any не имеет такой информации, поэтому может использовать дополнительную память.
    /// std::get<Type>(...) бросает исключение, если тип внутри variant не совпадает с ожидаемым, а иначе возвращает ссылку на запрошенный тип.
    /// std::get_if<Type>(...), которая возвращает указатель на запрошенный тип, если тип внутри variant совпадает с ожидаемым, и возвращает nullptr в противном случае.
    {
        std::variant<std::string, int> age; // возраст, который может быть реализован строковой датой или целочисленным значением
        age = value;

        auto ageUintPointer = std::get_if<int>(&age); // указатель на тип int
        auto ageStringPointer = std::get_if<std::string>(&age); // указатель на тип std::string
        auto ageUint = std::get<int>(age); // значение на тип int
        // auto ageString = std::get<std::string>(age); // Ошибка, значение на тип std::string отсутствует

        if (ageUintPointer)
        {

        }
        else if (ageStringPointer)
        {

        }
    }

    /// Получение неконстантной ссылки на внутренние данные std::string с помощью метода data(). Однако data() вернет строку без нуля в конце
    {
        std::string str = "hello";
        /// Старый способ до C++17
        {
            assert(!str.empty()); // для пустой строки str[0] недопустим, поэтому требуем непустую строку
            char* data = &str[0];
        }

        /// Новый способ в C++17
        {
            char* data = str.data();
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

    /// std::to_chars/std::from_chars - функции аналогичны std::to_string/std::atoi,std::stoi, но не требующие выделении динамической памяти и поддерживающие обработку ошибок. Он более гибок со значениями с плавающей запятой(1e-09), чем std::string, но требующий заранее выделенный буфер.
    /// Возвращает указатель на конец записанных символов в буфере, std::errc - в случае ошибки
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
    
    // Хранение любого типа
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
    }

    
    /* C++ В отличие от void* std::any владеет и управляет жизнью объекта (создание, копирование, перемещение и уничтожение)
     std::any может хранить информацию любого типа  и сообщает об ошибке (бросает исключение), когда вы пытаетесь получить доступ указав не тот тип. Предподчтительнее использовать std::variant
     Так он может сначала хранить int, затем float, а потом std::string
     std::variant и std::optional не требует никаких дополнительных выделений памяти, но это вызвано тем, что типы хранимых в объекте данных заранее известны. std::any не имеет такой информации, поэтому может использовать дополнительную память.
     std::any использовать только в тех случахя, когда надо хранить или передавать какие-то данные неизвестного типа.
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
            std::map<std::string, std::any> m;
            m["integer"] = 10;
            m["string"] = std::string("Hello World");
            m["float"] = 1.0f;

            for (auto& [_, val] : m)
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

    /*
    * std::partition - Переупорядочивает элементы в диапазоне[first, last): при выполенении условия true смещаются влево, иначе false.
    * return - Итератор к первому элементу второй группы
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
    /*
     lambda - могут быть constexpr, но с C++20 идет по-умолчанию, так что писать необязательно
     */
    {
        constexpr int y = 32;
        constexpr auto answer = [y]()
        {
            constexpr int x = 10;
            return x + y;
        };
        
        auto result = answer();
    }

    return 0;
}
