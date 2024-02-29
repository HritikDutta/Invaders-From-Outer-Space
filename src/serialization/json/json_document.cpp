#include "json_document.h"

#include "core/types.h"
#include "core/logger.h"
#include "containers/darray.h"
#include "containers/string.h"
#include "containers/hash_table.h"

namespace Json
{

Value Document::start() const
{
    return Value { this, 3 };
}

Value Array::operator[](u64 index) const
{
    const auto& node = document->dependency_tree[tree_index];
    return Value { document, node.array[index] };
}

// Returns null if key isn't found
Value Object::operator[](const String& key) const
{
    DependencyNode node = document->dependency_tree[tree_index];
    auto elem = find(node.object, key);

    // Return null if element was not found
    if (!elem)
        return Value { document, 0 };

    return Value { document, elem.value() };
}

} // namespace Json