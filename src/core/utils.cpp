#include "utils.h"

#include <cstdlib>
#include <cstring>
#include "core/logger.h"
#include "core/types.h"
#include "containers/string.h"
#include "math/common.h"

void to_string(String& str, s32 integer, u32 radix)
{
    gn_assert_with_message(str.data, "Destination string for integer to string conversion points to null!");
    gn_assert_with_message(radix >= 2 && radix <= 16, "Radix value for converting number to string is not valid! (radix: %)", radix);

    constexpr char digits[] = "0123456789abcdef";

    if (integer == 0)
    {
        str.data[0] = '0';
        str.size = 1;
        return;
    }

    s32 num = Math::abs(integer);
    u64 num_digits = (log(num) / log(radix)) + 1;

    u64 i = 0;
    while (num)
    {
        s32 index = num % radix;
        str.data[i++] = digits[index];
        num /= radix;
    }

    if (integer < 0)
        str.data[i++] = '-';
    
    str.size = i;
    reverse(str);
}

void to_string(String& str, s64 integer, u32 radix)
{
    gn_assert_with_message(str.data, "Destination string for integer to string conversion points to null!");
    gn_assert_with_message(radix >= 2 && radix <= 16, "Radix value for converting number to string is not valid! (radix: %)", radix);

    constexpr char digits[] = "0123456789abcdef";

    if (integer == 0)
    {
        str.data[0] = '0';
        str.size = 1;
        return;
    }

    s64 num = Math::abs(integer);
    u64 num_digits = (log(num) / log(radix)) + 1;

    u64 i = 0;
    while (num)
    {
        s64 index = num % radix;
        str.data[i++] = digits[index];
        num /= radix;
    }

    if (integer < 0)
        str.data[i++] = '-';
    
    str.size = i;
    reverse(str);
}

void to_string(String& str, u32 integer, u32 radix)
{
    gn_assert_with_message(str.data, "Destination string for integer to string conversion points to null!");
    gn_assert_with_message(radix >= 2 && radix <= 16, "Radix value for converting number to string is not valid! (radix: %)", radix);

    constexpr char digits[] = "0123456789abcdef";

    if (integer == 0)
    {
        str.data[0] = '0';
        str.size = 1;
        return;
    }

    u32 num = integer;
    u64 num_digits = (log(num) / log(radix)) + 1;

    u64 i = 0;
    while (num)
    {
        u32 index = num % radix;
        str.data[i++] = digits[index];
        num /= radix;
    }

    str.size = i;
    reverse(str);
}

void to_string(String& str, u64 integer, u32 radix)
{
    gn_assert_with_message(str.data, "Destination string for integer to string conversion points to null!");
    gn_assert_with_message(radix >= 2 && radix <= 16, "Radix value for converting number to string is not valid! (radix: %)", radix);

    constexpr char digits[] = "0123456789abcdef";

    if (integer == 0)
    {
        str.data[0] = '0';
        str.size = 1;
        return;
    }

    u64 num = integer;
    u64 num_digits = (log(num) / log(radix)) + 1;

    u64 i = 0;
    while (num)
    {
        u64 index = num % radix;
        str.data[i++] = digits[index];
        num /= radix;
    }

    str.size = i;
    reverse(str);
}

void to_string(String& str, f32 number, u32 after_decimal)
{
    gn_assert_with_message(str.data, "Destination string for float to string conversion points to null!");

    str.size = 0;
    
    String mantissa = String { str.data, 0 };
    if (number < 0)
    {
        str.data[0] = '-';
        str.size = 1;
        
        mantissa.data = str.data + 1;
    }

    s32 integer = Math::abs((s32) number);
    f32 fractional = number - (f32) integer;

    to_string(mantissa, integer);
    fractional = Math::abs(fractional);
    
    str.size += mantissa.size;

    if (after_decimal > 0)
    {
        str.data[str.size] = '.';
        String fraction_string = { str.data + str.size + 1, 0 };

        constexpr char digits[] = "0123456789abcdef";
        while (after_decimal)
        {
            fractional *= 10;
            s32 index = ((s32) fractional) % 10;
            fraction_string.data[fraction_string.size++] = digits[index];
            after_decimal--;
        }

        str.size += fraction_string.size + 1;
    }
}
void to_string(String& str, f64 number, u32 after_decimal)
{
    gn_assert_with_message(str.data, "Destination string for float to string conversion points to null!");

    str.size = 0;
    
    String mantissa = String { str.data, 0 };
    if (number < 0)
    {
        str.data[0] = '-';
        str.size = 1;
        
        mantissa.data = str.data + 1;
    }

    u64 integer = Math::abs((s64) number);
    f64 fractional = number - (f64) integer;

    to_string(str, integer);
    fractional = Math::abs(fractional);

    str.size += mantissa.size;

    if (after_decimal > 0)
    {
        str.data[str.size] = '.';
        String fraction_string = { str.data + str.size + 1, 0 };

        constexpr char digits[] = "0123456789abcdef";
        while (after_decimal)
        {
            fractional *= 10;
            s32 index = ((s32) fractional) % 10;
            fraction_string.data[fraction_string.size++] = digits[index];
            after_decimal--;
        }

        str.size += fraction_string.size + 1;
    }
}