#pragma once

#include <cstdio>
#include "core/types.h"

void print_to_file(FILE* file, void* ptr);

inline void print_to_file(FILE* file, const char* cstring)
{
    fputs(cstring, file);
}

inline void print_to_file(FILE* file, char* cstring)
{
    fputs(cstring, file);
}

inline void print_to_file(FILE* file, char c)
{
    fputc(c, file);
}

inline void print_to_file(FILE* file, u8 c)
{
    fputc(c, file);
}

template <typename T>
void print_to_file(FILE* file, const T&);

template <typename T>
void print_to_file(FILE* file, T* ptr)
{
    print_to_file(file, "% (%)", (void*) ptr, *ptr);
}

template <typename T, typename... Types>
void print_to_file(FILE* file, const char* format, const T& item, Types... args)
{
    u64 offset = 0;
    while (format[offset] != '\0')
    {
        if (format[offset] != '%')
        {
            fputc(format[offset], file);
            offset++;
            continue;
        }
        
        // Encountered a %
        if (format[offset + 1] != '%')  // print item if not followed by another %
        {
            print_to_file(file, item);
            print_to_file(file, format + offset + 1, args...);
            break;
        }

        // Encountered another %
        fputc('%', file);
        offset += 2;
    }
}

template <typename... Types>
void print(const char* format, Types... args)
{
    print_to_file(stdout, format, args...);
}

template <typename... Types>
void print_error(const char* format, Types... args)
{
    print_to_file(stderr, format, args...);
}

#ifndef GN_RELEASE

template <typename... Args>
inline static void debug_msg_internal(FILE* filestream, const char* label, const char* file, const char* function, const int line, const char* message_fmt, Args... args)
{
    print_to_file(filestream, "%: ", label);
    print_to_file(filestream, message_fmt, args...);
    print_to_file(filestream, "\nFile: %\nFunction: %\nLine: %\n", file, function, line);
}

// Defining a compiler agnostic way for haulting the program
#ifdef _MSC_VER
#include <intrin.h>
#define gn_break_point() __debugbreak()
#else
#define gn_break_point() __builtin_trap()
#endif

#define gn_assert(x)                        if (!(x)) { debug_msg_internal(stderr, "ASSERTION FAILED", __FILE__, __FUNCSIG__, __LINE__, #x); gn_break_point(); }
#define gn_assert_with_message(x, msg, ...) if (!(x)) { debug_msg_internal(stderr, "ASSERTION FAILED", __FILE__, __FUNCSIG__, __LINE__, msg, __VA_ARGS__); gn_break_point(); }
#define gn_assert_not_implemented()         { debug_msg_internal(stderr, "ASSERTION FAILED", __FILE__, __FUNCSIG__, __LINE__, "Function not implemented!"); gn_break_point(); }

#define gn_warn(msg, ...)           debug_msg_internal(stdout, "WARNING", __FILE__, __FUNCSIG__, __LINE__, msg, __VA_ARGS__)
#define gn_warn_if(cond, msg, ...)  if ((cond)) { debug_msg_internal(stdout, "WARNING", __FILE__, __FUNCSIG__, __LINE__, msg, __VA_ARGS__); }

#else

#define gn_assert(x)                        (x)
#define gn_assert_with_message(x, msg, ...) (x)
#define gn_assert_not_implemented()

#define gn_warn(msg, ...)
#define gn_warn_if(cond, msg, ...)          (cond)

#endif // GN_RELEASE