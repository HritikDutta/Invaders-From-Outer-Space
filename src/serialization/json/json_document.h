#pragma once

#include "core/types.h"
#include "core/logger.h"
#include "containers/darray.h"
#include "containers/string.h"
#include "containers/hash_table.h"
#include "json_types.h"

namespace Json
{

using ResourceIndex = u64;
using ArrayNode = DynamicArray<ResourceIndex>;
using ObjectNode = HashTable<String, ResourceIndex>;

union Resource
{
    bool   boolean;
    s64    integer64;
    f64    float64;
    String string;
};

struct DependencyNode
{
    Type type;

    union
    {
        ResourceIndex index;
        ArrayNode     array;
        ObjectNode    object;
    };
};

const char* get_enum_name(Type type);

struct Value;

struct Document
{
    DynamicArray<DependencyNode> dependency_tree;
    DynamicArray<Resource>       resources;

    Value start() const;
};

struct Array
{
    const Document* document;
    ResourceIndex   tree_index;
    
    u64 size() const
    {
        const auto& node = document->dependency_tree[tree_index];
        return node.array.size;
    }

    Value operator[](u64 index) const;
};

struct Object
{
    const Document* document;
    ResourceIndex   tree_index;

    // Returns null if key isn't found
    Value operator[](const String& key) const;
};

struct Value
{
    const Document* document;
    ResourceIndex   tree_index;

    Type type() const
    {
        return document->dependency_tree[tree_index].type;
    }

    const s64 int64() const
    {
        const auto& node = document->dependency_tree[tree_index];
        gn_assert_with_message(node.type == Type::INTEGER,
                               "Value doesn't correspond to a INTEGER resource! (actual node type: %)",
                               get_enum_name(node.type));

        const auto& resource = document->resources[node.index];
        return resource.integer64;
    }

    // Can cast integer values to float
    const f64 float64() const
    {
        const auto& node = document->dependency_tree[tree_index];
        gn_assert_with_message(node.type == Type::FLOAT || node.type == Type::INTEGER,
                               "Value doesn't correspond to a FLOAT or INTEGER resource! (actual node type: %)",
                               get_enum_name(node.type));

        const auto& resource = document->resources[node.index];
        return (node.type == Type::FLOAT) ? resource.float64 : (f64) resource.integer64;
    }

    const bool boolean() const
    {
        const auto& node = document->dependency_tree[tree_index];
        gn_assert_with_message(node.type == Type::BOOLEAN,
                               "Value doesn't correspond to a BOOLEAN resource! (actual node type: %)",
                               get_enum_name(node.type));

        const auto& resource = document->resources[node.index];
        return resource.boolean;
    }
    
    const String string() const
    {
        const auto& node = document->dependency_tree[tree_index];
        gn_assert_with_message(node.type == Type::STRING,
                               "Value doesn't correspond to a STRING resource! (actual node type: %)",
                               get_enum_name(node.type));

        const auto& resource = document->resources[node.index];
        return resource.string;
    }

    Array array() const
    {
        const auto node_type = document->dependency_tree[tree_index].type;
        gn_assert_with_message(node_type == Type::ARRAY,
                               "Value doesn't correspond to a ARRAY resource! (actual node type: %)",
                               get_enum_name(node_type));

        // Since data layout is the same
        return *(Array*) (this);
    }

    Value operator[](u64 index) const
    {
        const auto& node = document->dependency_tree[tree_index];
        gn_assert_with_message(node.type == Type::ARRAY,
                               "Value doesn't correspond to a ARRAY resource! (actual node type: %)",
                               get_enum_name(node.type));

        return Value { document, node.array[index] };
    }
    
    Object object() const
    {
        const auto node_type = document->dependency_tree[tree_index].type;
        gn_assert_with_message(node_type == Type::OBJECT,
                               "Value doesn't correspond to a OBJECT resource! (actual node type: %)",
                               get_enum_name(node_type));

        // Since data layout is the same
        return *(Object*) (this);
    }
    
    // Returns null if key isn't found
    Value operator[](const String& key) const
    {
        DependencyNode node = document->dependency_tree[tree_index];
        gn_assert_with_message(node.type == Type::OBJECT,
                               "Value doesn't correspond to a OBJECT resource! (actual node type: %)",
                               get_enum_name(node.type));

        auto elem = find(node.object, key);

        // Return null if element was not found
        if (!elem)
            return Value { document, 0 };

        return Value { document, elem.value() };
    }
};

} // namespace Json

inline Json::Document make(Type<Json::Document>, u64 start_cap = 16)
{
    using namespace Json;

    Document document;

    document.resources = make<DynamicArray<Resource>>(start_cap);
    document.dependency_tree = make<DynamicArray<DependencyNode>>(start_cap);

    return document;
}

inline void free(Json::Document& document)
{
    for (u64 i = 0; i < document.dependency_tree.size; i++)
    {
        switch (document.dependency_tree[i].type)
        {
            case Json::Type::STRING:
            {
                Json::ResourceIndex index = document.dependency_tree[i].index;
                free(document.resources[index].string);
            } break;

            case Json::Type::ARRAY:
            {
                free(document.dependency_tree[i].array);
            } break;

            case Json::Type::OBJECT:
            {
                Json::ObjectNode node = document.dependency_tree[i].object;
                free_keys(node);    // Keys are all strings
                free(node);
            } break;
        }
    }

    free(document.dependency_tree);
    free(document.resources);
}