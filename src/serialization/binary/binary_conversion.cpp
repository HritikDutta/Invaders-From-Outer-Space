#include "binary_conversion.h"

#include <cfloat>

#include "core/types.h"
#include "core/logger.h"
#include "containers/darray.h"
#include "containers/bytes.h"
#include "containers/string.h"
#include "serialization/json.h"
#include "binary_types.h"
#include "binary_utils.h"

namespace Binary
{

static void encode_json_value_to_binary(DynamicArray<u8>& bytes, Json::Value value)
{
    switch (value.type())
    {
        case Json::Type::NONE:
        {
            append(bytes, NIL);
        } break;
        
        case Json::Type::BOOLEAN:
        {
            append(bytes, value.boolean() ? BOOLEAN_TRUE : BOOLEAN_FALSE);
        } break;

        case Json::Type::INTEGER:
        {
            const s64 int_value = value.int64();

            // Encode in the least number of bytes required
            if (int_value >= INT8_MIN && int_value <= INT8_MAX)
            {
                append(bytes, INTEGER_S8);
                append_integer(bytes, (s8) int_value);
            }
            else if (int_value >= INT16_MIN && int_value <= INT16_MAX)
            {
                append(bytes, INTEGER_S16);
                append_integer(bytes, (s16) int_value);
            }
            else if (int_value >= INT32_MIN && int_value <= INT32_MAX)
            {
                append(bytes, INTEGER_S32);
                append_integer(bytes, (s32) int_value);
            }
            else
            {
                append(bytes, INTEGER_S64);
                append_integer(bytes, int_value);
            }
        } break;

        case Json::Type::FLOAT:
        {
            const f64 float_value = value.float64();

            // Encode as float 32 if value is small
            if (float_value >= -FLT_MAX && float_value <= FLT_MAX)
            {
                append(bytes, FLOAT_32);
                append_float(bytes, (f32) float_value);
            }
            else
            {
                append(bytes, FLOAT_64);
                append_float(bytes, float_value);
            }
        } break;
        
        case Json::Type::STRING:
        {
            const String str = value.string();
            append_string(bytes, str);
        } break;
        
        case Json::Type::ARRAY:
        {
            const Json::Array array = value.array();

            // encode array length
            if (array.size() <= 0xffu)
            {
                append(bytes, ARRAY_1_BYTE);
                append_integer(bytes, (u8) array.size());
            }
            else if (array.size() <= 0xffffu)
            {
                append(bytes, ARRAY_2_BYTE);
                append_integer(bytes, (u16) array.size());
            }
            else if (array.size() <= 0xffffffffu)
            {
                append(bytes, ARRAY_4_BYTE);
                append_integer(bytes, (u32) array.size());
            }
            else // (array.size() <= 0xffffffffffffffffu) // can't go bigger than this anyways
            {
                append(bytes, ARRAY_8_BYTE);
                append_integer(bytes, (u64) array.size());
            }
            
            // encode array data
            for (u64 i = 0; i < array.size(); i++)
            {
                encode_json_value_to_binary(bytes, array[i]);
            }
        } break;
        
        case Json::Type::OBJECT:
        {
            append(bytes, OBJECT_START);

            const Json::Object object = value.object();
            const Json::Document* document = object.document;
            const Json::ObjectNode object_node = document->dependency_tree[object.tree_index].object;

            u32 encoded_count = 0;
            for (u32 i = 0; encoded_count < object_node.filled && i < object_node.capacity; i++)
            {
                if (object_node.states[i] == Json::ObjectNode::State::ALIVE)
                {
                    // append_string(bytes, object_node.keys[i]);
                    Json::Value property = { document, object_node.values[i] };
                    encode_json_value_to_binary(bytes, property);
                    encoded_count++;
                }
            }

            append(bytes, OBJECT_END);
        } break;
    }
}

Bytes json_document_to_binary(const Json::Document& document)
{
    DynamicArray<u8> output = make<DynamicArray<u8>>(1024ui64);

    encode_json_value_to_binary(output, document.start());

    resize(output, output.size);  // Shrink the array to free extra memory

    return Bytes { output.data, output.size };
}

} // namespace Binary