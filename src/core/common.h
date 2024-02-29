#pragma once

template <typename T>
struct Type {};

template <typename T, typename... Args>
inline T make(Type<T>, Args... args);

template <typename T, typename... Args>
inline T make(Args... args)
{
    return make(Type<T> {}, args...);
}

template <typename T>
inline T copy(const T& other)
{
    return other;
}

template <typename T>
inline void swap(T& a, T& b)
{
    T temp = a;
    a = b;
    b = temp;
}