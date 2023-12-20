#include <iostream>
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

int main()
{
    setlocale(LC_ALL, "Russian"); // ����� ��������� ���� � ���������� ��������� (Windows) - ������� �������� 1251
    int value = 5;

    /// ������������ std::tuple, std::pair, std::map, struct
    auto [a, b, c] = std::tuple(1, "hello", 0.1);
    auto [value1, value2] = std::pair{"hello", 1};
    //auto [a, b] = std::map{ "hello", 1 };
    auto [title, year] = Example();

    /// ������������� ������ �� ���������� ��� ������������
    int number1;
    std::tie(number1, std::ignore) = std::tuple(2, 3);

    /*
    * ���������� ������������� (���������� ��� ����������������� ��������)  � ����� ������������� ������ ��� �������� � ����� ������� (����� �������� � �����������), �� ���������� ����������������:
    1) ���������� �������� ��� ���������� ������
    2) ���������� �������� ������������� ������������� ����� ���� �������� �� ��������� ��� ��������� �������������
    3) ���������� ������� �������
    4) ���������� ����������� �������-������, ��������� ������
    */
    {
        /// 1 ������: ������������� ���� ������ ���������
        {
            Location location{ "Moscow", "Russia" };
        }
        /// 2 ������: ������������� ������ ������� ����� ���������
        {
            Person person1{ "stas" };
        }
        /// 3 ������: ������������� ��������� ���������
        {
            Person person2{ "mike", 50, {"Newcastle", "UK"} };
        }
    }

    /// �������� nodiscard, fallthrough, maybe_unused
    {
        /// nodiscard - �������, ������� ���������� ������������ ������������ �������� �������
        {
            /// 1 ������
            {
                //CreateNumber(); // warning C4834 : ������ ������������� �������� ������� � ��������� "nodiscard"
                auto number = CreateNumber(); // ������������ ��������
            }
            /// 2 ������
            {
                //CreateExample(); // warning C4834 : ������ ������������� �������� ������� � ��������� "nodiscard"
                auto example = CreateExample(); // ������������ ��������
            }

            /// 3 ������ - ������������� ����������
            {
                std::ignore = CreateNumber();
                std::ignore = CreateExample();
            }
        }

        /// fallthrough - ������� ���������� switch case, ������� ���������, ��� �������� break ��� ���������� �������� � ��� �� �������� �������
        {
            switch (value)
            {
                case 0:
                    [[fallthrough]]; // ���������� ����������
                case 50:
                    [[fallthrough]]; // ���������� ����������
                case 100:
                    /// �����-�� �������
                    break;
                default:
                    break;
            }
        }

        /// maybe_unused - �������, ����������� �������������� � ���������������� ���������� 
        {
            /// ������ ������
            {
                auto result = DoExmapleCall();
                (void)result; // ����� �������������� �� unused variable
                assert(result > 0);
            }

            /// ����� ������
            {
                [[maybe_unused]] auto result = DoExmapleCall();
                assert(result > 0);
            }
        }
    }

    /// std::string_view �������� ����������� ������, � �� ��������� ������, ����������� ������(const std::string&). �� ������� �����, ��� �� ������ ������ ������������� ������ � �� ��������� �����. ����� ����� std::string_view �� ������� �� ������� ����� ������, ������� �� ������������. 
    /// ������, ���� ������������ ������ ������ �� ������� ������� ���������, �� std::string_view ������ �� ������ � ���������� � ��� ������� ������� ��������� �������������� ���������. std::string_view �� ������������� ������� �������� ������� �� �����.
    /// ������ string_view ����� ���� �������� ���������.
    /// remove_prefix � remove_suffix, ������� �������� �� �������� ��������� string_view �������� ����� �������� � ������ ��� � �����; �������� ������ �� ��������, � �������� ������ ����������� ��������.
    /// � ���������� ���� ������� � ������� ������ const string& ������� ������������ ����������� string_view, �� ���������� ��������� string.
    {
        std::string str{ "hello" };
        std::string_view strView{ str };

        strView.remove_prefix(1); // ���������� ������ ������
        strView.remove_suffix(2); // ���������� ��������� 2 �������

        str = "example"; // strView ����� ������ ������� "xa" ��� ������� � ���� ��������� �������� � ������ ��������� ������� � 5 �� 2
    }

    /// std::optional - �������, ������� ����� ��������� �������� ��� ���� ����� �����(nullopt). �� ������� ������������� ��������� ������������ ������.
    /// std::variant � std::optional �� ������� ������� �������������� ��������� ������, �� ��� ������� ���, ��� ���� �������� � ������� ������ ������� ��������. std::any �� ����� ����� ����������, ������� ����� ������������ �������������� ������.
    /// � ������� std::optional ����� ��������� ����������/�� ���������� �������� � ������� �������� ���������� � bool (true, false, nullptr), �������� � ��������� � ������������ ������ emplace, reset, swap � assign.
    /// has_value() - �������� �������� �� nullptr
    /// value() - �������� std::optional
    /// emplace() - �������� �������� �� ������
    /// reset() - ����� ��������
    {
        std::optional<int> flag = value;
        
        /// ������ 1
        {
            if (flag) // �������� nullptr
            {
                if (flag == 5) // �������� �� �������� 5
                    value = 10;
            }
        }

        /// ������ 2
        {
            if (flag.has_value()) // �������� nullptr
            {
                if (flag.value() == 5) // �������� �� �������� 5
                    value = 10;
            }
        }

        /// ������ 3. ��������� � ������. ���� ���� �����, �� ���������� ��� �����. ���� ��� ���, �� ���������� ������������ �����
        {
            std::cout << flag.value_or(5) << std::endl; // ������ 10, ����� ������������
            std::cout << flag.value_or(5) << std::endl; // ������ 10, ����� ������������
            flag.reset();
            std::cout << flag.value_or(6) << std::endl; // ������ 11, ����� ����������
        }

        /// ������ 4: ��� �������� �������� �� ������, ���� �������� = nullptr
        {
            flag.reset();
            auto lambda = [](int& number)
            {
                number = 10;
            };

            lambda(flag.emplace());
        }

        /// ������ 5: ��� �������� �������� �� ������, ���� �������� != nullptr
        {
            //flag.reset();
            auto lambda = [](int& number)
            {
                number = 5;
            };

            lambda(flag.value());
        }
    }

    /// std::variant - ��� ������, ������� ����� ������� � ���������� � ���� ��������� ����� ������. 
    /// �� ��������� ���������������� ���� � �� �� ������� ������ ��� �������� ������ ����� ����� ������ ��� ��������� �������������� ������. 
    /// std::variant �������� ����� ����������, ��� union ��� enum �.�. ������������ � stl, ������� ����� ������������� ������������ �� �������. 
    /// std::variant � std::optional �� ������� ������� �������������� ��������� ������, �� ��� ������� ���, ��� ���� �������� � ������� ������ ������� ��������. std::any �� ����� ����� ����������, ������� ����� ������������ �������������� ������.
    /// std::get<Type>(...) ������� ����������, ���� ��� ������ variant �� ��������� � ���������, � ����� ���������� ������ �� ����������� ���.
    /// std::get_if<Type>(...), ������� ���������� ��������� �� ����������� ���, ���� ��� ������ variant ��������� � ���������, � ���������� nullptr � ��������� ������.
    {
        std::variant<std::string, uint32_t> age; // �������, ������� ����� ���� ���������� ��������� ����� ��� ������������� ���������
        age = value;

        auto ageUintPointer = std::get_if<uint32_t>(&age); // ��������� �� ��� uint32_t
        auto ageStringPointer = std::get_if<std::string>(&age); // ��������� �� ��� std::string
        auto ageUint = std::get<uint32_t>(age); // �������� �� ��� uint32_t
        // auto ageString = std::get<std::string>(age); // ������, �������� �� ��� std::string �����������

        if (ageUintPointer)
        {

        }
        else if (ageStringPointer)
        {

        }
    }

    /// ��������� ������������� ������ �� ���������� ������ std::string � ������� ������ data(). ������ data() ������ ������ ��� ���� � �����
    {
        std::string str = "hello";
        /// ������ ������ �� C++17
        {
            assert(!str.empty()); // ��� ������ ������ str[0] ����������, ������� ������� �������� ������
            char* data = &str[0];
        }

        /// ����� ������ � C++17
        {
            char* data = str.data();
        }
    }

    /// std::clamp - �������� ��������, ���� ��� �� �������� � ������������ �������� ���� ������, ���� �����
    {
        const int min = 10;
        const int max = 100;
        std::cout << std::clamp(0, min, max) << std::endl; // �� ������������� ������� >= 10 && <= 100, ������� ��������������� ����������� ��������
        std::cout << std::clamp(50, min, max) << std::endl; // ������������� ������� >= 10 && <= 100
        std::cout << std::clamp(120, min, max) << std::endl; // �� ������������� ������� >= 10 && <= 100, ������� ��������������� ������������ ��������
    }

    /// std::to_chars/std::from_chars - ������� ���������� std::to_string/std::atoi,std::stoi, �� �� ��������� ��������� ������������ ������ � �������������� ��������� ������. �� ����� ����� �� ���������� � ��������� �������(1e-09), ��� std::string, �� ��������� ������� ���������� �����.
    /// ���������� ��������� �� ����� ���������� �������� � ������, std::errc - � ������ ������
    {
        /// ������ 1
        {
            std::array<char, 10> str;
            if (auto [p, ec] = std::to_chars(str.data(), str.data() + str.size(), value); ec == std::errc())
            {
                std::cout << std::string_view(str.data(), p - str.data());
            }
        }

        /// ������ 2
        {
            std::string str = "101";
            int result;
            if (auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), result); ec == std::errc())
            {
                std::cout << result << std::endl;
            }
        }
    }

    /// std::any ����� ������� ���������� ������ ���� � �������� �� ������ (������� ����������), ����� �� ��������� �������� ������ ������ �� ��� ���. ����������������� ������������ std::variant
    /// ��� �� ����� ������� ������� int, ����� float, � ����� std::string
    /// std::variant � std::optional �� ������� ������� �������������� ��������� ������, �� ��� ������� ���, ��� ���� �������� � ������� ������ ������� ��������. std::any �� ����� ����� ����������, ������� ����� ������������ �������������� ������.
    /// std::any ������������ ������ � ��� �������, ����� ���� ������� ��� ���������� �����-�� ������ ������������ ����.
    /// std::any_cast<Type>(...) - ���������� � ����
    /// type() - ���
    /// val.type().name() - ��� ����
    /// has_value() - �������� �������� �� nullptr
    /// reset() - ����� ��������
    {
        /// ������ 1: ��� �������� �� ���
        {
            std::any a = 42;
            std::cout << std::any_cast<int>(a) << std::endl;
            a = 11.34f;
            std::cout << a.type().name() << std::any_cast<float>(a) << std::endl;
            a = "hello";
            try
            {
                std::cout << std::any_cast<std::string>(a) << '\n';
            }
            catch (const std::bad_any_cast& e)
            {
                std::cout << e.what() << std::endl;
            }
        }

        /// ������ 2: � ��������� �� ���
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

        /// ������ 3: � ��������� �� ��� � ������� ���������� � ���������
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
    * std::partition - ����������������� �������� � ���������[first, last): ��� ����������� ������� true ��������� �����, ����� false.
    * return - �������� � ������� �������� ������ ������
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

    return 0;
}
