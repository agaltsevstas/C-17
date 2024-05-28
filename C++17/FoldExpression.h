#ifndef FoldExpression_h
#define FoldExpression_h

#include <iostream>
#include <vector>

/*
 Fold expression (выражение свертки) - шаблон с заранее неизвестным числом аргументов (variadic template). Свертка – это функция, которая применяет заданную комбинирующую функцию к последовательным парам элементов в списке и возвращает результат. Любое выражение свёртки должно быть заключено в скобки. Выражение внутри скобок должно содержать в себе нераскрытую пачку параметров и один из следующих операторов:
 +  -  *  /  %  ^  &  |  =  <  >  <<  >>
 +=  -=  *=  /=  %=  ^=  &=  |=  <<=  >>=
 ==  !=  <=  >=  &&  ||  ,  .*  ->*
 
 Унарная правоассоциативная свёртка: (args + …), раскрывается: (1 + (2 + (3 + (4 + 5))))
 Унарная левоассоциативная свёртка: (… + args), раскрывается: ((((1 + 2) + 3) + 4) + 5)
 Бинарная правоассоциативная свертка: (args + ... + init), раскрывается: (1 + (2 + (3 + (4 + (5 + 100))))
 Бинарная левоассоциативная свертка: (init + ... + args), раскрывается: (((((100 + 1) + 2) + 3) + 4) + 5)
 До C++17: для развёртывания пачки параметров требуется рекурсивное инстанциирование шаблонов с помощью stub-функции (заглушка)
 */

namespace fold_expression
{
    /// 1 Способ: Унарная правоассоциативная свёртка
    template<typename... Args>
    inline constexpr auto Sum(Args... args)
    {
        return (args + ...); // (arg1+ arg2 + arg3 + ...)
    }

    /// 1 Способ: Унарная левоассоциативная свёртка
    /*
     template<typename... Args>
     inline constexpr auto Sum(Args... args)
     {
         return (... + args); // (... + arg1 + arg2 + arg3)
     }
     */

    /// 1 Способ: Бинарная правоассоциативная свертка
    /*
     template<typename... Args>
     inline constexpr auto Sum(Args... args)
     {
         return (args + ... + 100); // (arg1 + arg2 + arg3 + ... + 100)
     }
     */

    /// 1 Способ: Бинарная левоассоциативная свертка
    /*
     template<typename... Args>
     inline constexpr auto Sum(Args... args)
     {
         return (100 + ... + args); // (100 + ... + arg1 + arg2 + arg3)
     }
     */
    
    template<typename... Args>
    inline constexpr auto Average(Args... args) // Сокращенный шаблон
    {
        auto s =  Sum(args...);
        return s / sizeof...(args);
    }

    template<typename... Args>
    inline constexpr auto Norm(Args... args) // Сокращенный шаблон
    {
        return std::sqrt((( args * args) + ...)); // std::sqrt(arg1 * arg1 + arg2 * arg2 + ...)
    }

    template<typename... Args>
    inline constexpr auto Pow_Sum(Args... args) // Сокращенный шаблон
    {
        return (std::pow(args, 2) + ...); // arg1 * arg1 + arg2 * arg2 + ...
    }

    template<typename T, typename... Args>
    void Push_To_Vector(std::vector<T>& v, Args&&... args)
    {
        //Раскрывается в последовательность выражений через запятую вида:
        //v.push_back(std::forward<Args_1>(arg1)),
        //v.push_back(std::forward<Args_2>(arg2)),
        //....
        
        (v.push_back(std::forward<Args>(args)), ...);
    }

    template<typename ...Args>
    inline constexpr int CountArgs(Args&& ...args)
    {
        return sizeof...(args);
    }

    template<typename ...TArgs>
    inline constexpr int CountTypes(TArgs&& ...args)
    {
        return sizeof...(TArgs);
    }

    // 1 Способ
    template<typename TType, typename ...TArgs>
    inline constexpr std::integral_constant<unsigned, sizeof ...(TArgs)> CountArgsFunction(TType(*function)(TArgs ...))
    {
       return std::integral_constant<unsigned, sizeof ...(TArgs)>{};
    }

    // 2 Способ
    /*
     template<typename TType, typename ...TArgs>
     constexpr unsigned CountArgsFunction( TType(*function)(TArgs ...))
     {
         return sizeof...(TArgs);
     }
     */

    template<typename... Args>
    inline constexpr void CheckTypes(Args&& ...args) // Сокращенный шаблон
    {
        std::cout << "check types: ";
        ((std::cout << args,
          std::is_same_v<int, decltype(args)> ? std::cout << " - is int," : std::cout << " - not int,",
          std::is_same_v<double, decltype(args)> ? std::cout << " is double," : std::cout << " not double,",
          std::is_same_v<std::string, decltype(args)> ? std::cout << " is string" : std::cout << " not string",
         std::cout << std::endl), ...);
        std::cout << std::endl;
    }

    // C++17
    template <typename ...TArgs>
    inline void Print(const TArgs&... args)
    {
        ((std::cout << args << ", "), ...);
        std::cout << std::endl;
    }
}

#endif /* FoldExpression_h */
