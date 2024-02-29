#include "logger.h"

#include <cstdlib>
#include "core/types.h"
#include "containers/string.h"
#include "containers/bytes.h"
#include "utils.h"

#define LOGGER_TEMP_BUFFER_SIZE 64

template <>
void print_to_file(FILE* file, const bool& boolean)
{
    print_to_file(file, (boolean) ? "true" : "false");
}

template <>
void print_to_file(FILE* file, const String& str)
{
    for (u64 i = 0; i < str.size; i++)
        print_to_file(file, str.data[i]);
}

template <>
void print_to_file(FILE* file, const Bytes& bytes)
{
    for (u64 i = 0; i < bytes.size; i++)
        print_to_file(file, (char) bytes.data[i]);
}

template <>
void print_to_file(FILE* file, const s32& integer)
{
    char temp_buffer[LOGGER_TEMP_BUFFER_SIZE];
    String s = ref(temp_buffer, LOGGER_TEMP_BUFFER_SIZE);

    to_string(s, integer);
    print_to_file(file, s);
}

template <>
void print_to_file(FILE* file, const s64& integer)
{
    char temp_buffer[LOGGER_TEMP_BUFFER_SIZE];
    String s = ref(temp_buffer, LOGGER_TEMP_BUFFER_SIZE);

    to_string(s, integer);
    print_to_file(file, s);

    
}

template <>
void print_to_file(FILE* file, const u32& integer)
{
    char temp_buffer[LOGGER_TEMP_BUFFER_SIZE];
    String s = ref(temp_buffer, LOGGER_TEMP_BUFFER_SIZE);

    to_string(s, integer);
    print_to_file(file, s);
}

template <>
void print_to_file(FILE* file, const u64& integer)
{
    char temp_buffer[LOGGER_TEMP_BUFFER_SIZE];
    String s = ref(temp_buffer, LOGGER_TEMP_BUFFER_SIZE);

    to_string(s, integer);
    print_to_file(file, s);
}

template <>
void print_to_file(FILE* file, const f32& number)
{
    char temp_buffer[LOGGER_TEMP_BUFFER_SIZE];
    String s = ref(temp_buffer, LOGGER_TEMP_BUFFER_SIZE);

    to_string(s, number);
    print_to_file(file, s);
}

template <>
void print_to_file(FILE* file, const f64& number)
{
    char temp_buffer[LOGGER_TEMP_BUFFER_SIZE];
    String s = ref(temp_buffer, LOGGER_TEMP_BUFFER_SIZE);

    to_string(s, number);
    print_to_file(file, s);
}

void print_to_file(FILE* file, void* ptr)
{
    char temp_buffer[17];
    String s = ref(temp_buffer, 17);

    to_string(s, (u64) ptr, 16);
    print_to_file(file, "0x%", s);
}

#undef LOGGER_TEMP_BUFFER_SIZE