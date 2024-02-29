#include "binary_lexer.h"

#include "binary_types.h"
#include "binary_utils.h"
#include "core/types.h"
#include "core/logger.h"
#include "containers/string.h"
#include "containers/bytes.h"

namespace Binary
{

static void pretty_print(const Bytes& bytes, u64& offset, u64 indent)
{
    constexpr u64 MAX_TAB_COUNT = 128;
    char tab_data[2 * MAX_TAB_COUNT];
    memset(tab_data, ' ', 2 * MAX_TAB_COUNT * sizeof(char));

    const String tabs = ref(tab_data, 2 * indent);

    switch (bytes[offset])
    {
        case NIL:
        {
            print("%NIL\n", tabs);
            offset++;
        } break;

        case BOOLEAN_FALSE:
        case BOOLEAN_TRUE:
        {
            const char* type_name = get_type_name(bytes[offset]);
            print("%", tabs);
            print("%: %\n", type_name, get<bool>(bytes, offset));
        } break;

        case INTEGER_U8 :
        case INTEGER_U16:
        case INTEGER_U32:
        case INTEGER_U64:
        {
            const char* type_name = get_type_name(bytes[offset]);
            print("%", tabs);
            print("%: %\n", type_name, get<u64>(bytes, offset));
        } break;

        case INTEGER_S8 :
        case INTEGER_S16:
        case INTEGER_S32:
        case INTEGER_S64:
        {
            const char* type_name = get_type_name(bytes[offset]);
            print("%", tabs);
            print("%: %\n", type_name, get<s64>(bytes, offset));
        } break;

        case FLOAT_32:
        case FLOAT_64:
        {
            const char* type_name = get_type_name(bytes[offset]);
            print("%", tabs);
            print("%: %\n", type_name, get<f64>(bytes, offset));
        } break;

        case STRING_1_BYTE:
        case STRING_2_BYTE:
        case STRING_4_BYTE:
        case STRING_8_BYTE:
        {
            const char* type_name = get_type_name(bytes[offset]);
            print("%", tabs);
            print("%: %\n", type_name, get<String>(bytes, offset));
        } break;

        case BYTE_ARRAY_1_BYTE:
        case BYTE_ARRAY_2_BYTE:
        case BYTE_ARRAY_4_BYTE:
        case BYTE_ARRAY_8_BYTE:
        {
            const char* type_name = get_type_name(bytes[offset]);
            print("%", tabs);
            print("%: %\n", type_name, get<Bytes>(bytes, offset));
        } break;

        case ARRAY_1_BYTE:
        case ARRAY_2_BYTE:
        case ARRAY_4_BYTE:
        case ARRAY_8_BYTE:
        {
            u64 count = get_next_uint(bytes, offset);

            print("%array start (size: %):\n", tabs, count);

            while (count--)
                pretty_print(bytes, offset, indent + 1);

            print("%array end\n", tabs);
        } break;

        case OBJECT_START:
        {
            offset++;

            print("%object start:\n", tabs);

            while (bytes[offset] != OBJECT_END)
                pretty_print(bytes, offset, indent + 1);

            print("%object end\n", tabs);
            offset++;
        } break;

        default:
        {
            print("Error byte! (byte: %, offset: %)\n", bytes[offset], offset);
            offset++;
        } break;
    }
}

void pretty_print(const Bytes& bytes)
{
    print("file start:\n");
    u64 offset = 0;
    pretty_print(bytes, offset, 1);
    print("file end\n");
}

} // namespace Binary