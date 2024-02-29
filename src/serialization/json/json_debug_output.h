#pragma once

#ifdef GN_LOG_SERIALIZATION

#include "core/types.h"
#include "core/logger.h"
#include "json_lexer.h"
#include "json_document.h"

namespace Json
{

static void lexer_debug_output(const DynamicArray<Token>& tokens)
{
    print("LEXER OUTPUT (token count: %)\n", tokens.size);

    for (u64 i = 0; i < tokens.size; i++)
        print("    type_id: %, value: '%'\n", (int) tokens[i].type, tokens[i].value);
}

static u64 print_node_info(const Document& document, u64 index)
{
    const DependencyNode& node = document.dependency_tree[index];

    switch (node.type)
    {
        case Type::STRING:
        {
            const Resource& res = document.resources[node.index];
            print("String \"%\"\n", res.string);
        } break;
        
        case Type::INTEGER:
        {
            const Resource& res = document.resources[node.index];
            print("Integer '%'\n", res.integer64);
        } break;
        
        case Type::FLOAT:
        {
            const Resource& res = document.resources[node.index];
            print("Float '%'\n", res.float64);
        } break;
        
        case Type::BOOLEAN:
        {
            const Resource& res = document.resources[node.index];
            print("Boolean '%'\n", res.boolean);
        } break;
        
        case Type::NONE:
        {
            print("null\n");
        } break;

        case Type::ARRAY:
        {
            print("::Array Start::\n");

            for (u64 i = 0; i < node.array.size; i++)
                index = print_node_info(document, node.array[i]);

            print("::Array End::\n");
        } break;

        case Type::OBJECT:
        {
            print("::Object Start::\n");

            for (u64 i = 0; i < node.object.capacity; i++)
            {
                if (node.object.states[i] == ObjectNode::State::ALIVE)
                {
                    print("%: ", node.object.keys[i]);
                    index = print_node_info(document, node.object.values[i]);
                }
            }

            print("::Object End::\n");
        } break;
    }

    return index + 1;
}

static void document_debug_output(const Document& document)
{
    print("PARSER OUTPUT (tree size: %)\n", document.dependency_tree.size);
    print_node_info(document, 3);
}

} // namespace Json

#endif // GN_LOG_SERIALIZATION