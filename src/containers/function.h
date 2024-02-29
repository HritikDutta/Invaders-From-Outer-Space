#pragma once

// Can't make a function with a simple type
template <typename Type>
struct Function
{
    Function() = delete;
};

template <typename RetType, typename... Args>
struct Function <RetType (Args...)>
{
    using FuncType = RetType (*)(Args...);
    FuncType _function;

    Function()
    {}
    
    Function(FuncType function)
    :   _function(function)
    {
    }

    Function(const Function& other)
    :   _function(other._function)
    {
    }
    
    Function(Function&& other)
    :   _function(other._function)
    {
    }

    RetType operator()(Args... args) const
    {
        return _function(args...);
    }

    Function& operator=(const Function& other)
    {
        _function = other._function;
        return *this;
    }

    Function& operator=(Function&& other)
    {
        _function = other._function;
        return *this;
    }

    // Conversion operator
    operator bool() const
    {
        return _function != nullptr;
    }

};