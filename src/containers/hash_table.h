#pragma once

#include <cstdlib>
#include "core/types.h"
#include "core/common.h"
#include "math/common.h"
#include "hash.h"

#define HASH_TABLE_TEMPLATE template <typename KeyType, typename ValueType, typename Hasher = Hasher<KeyType>>
#define HASH_TABLE_MAX_LOAD_FACTOR 0.75f

HASH_TABLE_TEMPLATE
struct HashTable
{
    enum struct State : u8
    {
        EMPTY,
        TOMBSTONE,
        ALIVE
    };

    State*     states;
    Hash*      hashes;
    KeyType*   keys;
    ValueType* values;

    u32 filled;
    u32 capacity;

    Hasher hasher;
};

HASH_TABLE_TEMPLATE
struct HashTableElement
{
    const HashTable<KeyType, ValueType, Hasher>* table;
    u32 index;
    
    // Conversions
    inline operator bool() const
    {
        using HashTable = HashTable<KeyType, ValueType, Hasher>;
        using State     = HashTable::State;

        gn_assert_with_message(table, "Element doesn't point to a valid hash table!");
        return index < table->capacity && table->states[index] == State::ALIVE;
    }

    // Getters
    inline KeyType& key() const
    {
        using HashTable = HashTable<KeyType, ValueType, Hasher>;
        using State     = HashTable::State;

        gn_assert_with_message(table, "Element doesn't point to a valid hash table!");
        gn_assert_with_message(index < table->capacity, "Element not valid!");
        gn_assert_with_message(table->states[index] == State::ALIVE, "Element at index % is not alive! (status %)", index, (u32) table->states[index]);
        return table->keys[index];
    }

    inline ValueType& value() const
    {
        using HashTable = HashTable<KeyType, ValueType, Hasher>;
        using State     = HashTable::State;

        gn_assert_with_message(table, "Element doesn't point to a valid hash table!");
        gn_assert_with_message(index < table->capacity, "Element not valid!");
        gn_assert_with_message(table->states[index] == State::ALIVE, "Element at index % is not alive! (status %)", index, (u32) table->states[index]);
        return table->values[index];
    }
};

HASH_TABLE_TEMPLATE
inline HashTable<KeyType, ValueType, Hasher> make(Type<HashTable<KeyType, ValueType, Hasher>>, u32 start_cap = 32)
{
    using HashTable = HashTable<KeyType, ValueType, Hasher>;
    using State     = HashTable::State;

    HashTable table;

    table.capacity = max(start_cap, 2Ui32);
    table.filled   = 0;
    
    const u64 size_in_bytes = table.capacity * (sizeof(State) + sizeof(Hash) + sizeof(KeyType) + sizeof(ValueType));
    void* allocation = platform_allocate(size_in_bytes);
    gn_assert_with_message(allocation, "Could not allocate data for hash table!");

    table.states = (State*)     (allocation);
    table.hashes = (Hash*)      (table.states + table.capacity);
    table.keys   = (KeyType*)   (table.hashes + table.capacity);
    table.values = (ValueType*) (table.keys   + table.capacity);

    platform_set_memory(table.states, (int) State::EMPTY, table.capacity * sizeof(State));

    return table;
}

HASH_TABLE_TEMPLATE
inline HashTable<KeyType, ValueType, Hasher> copy(const HashTable<KeyType, ValueType, Hasher>& other)
{
    using HashTable = HashTable<KeyType, ValueType, Hasher>;
    using State     = HashTable::State;

    HashTable table;

    table.capacity = other.capacity;
    table.filled   = other.filled;
    
    const u64 size_in_bytes = table.capacity * (sizeof(State) + sizeof(Hash) + sizeof(KeyType) + sizeof(ValueType));
    void* allocation = platform_allocate(size_in_bytes);
    gn_assert_with_message(allocation, "Could not allocate data for hash table!");

    table.states = (State*)     (allocation);
    table.hashes = (Hash*)      (table.states + table.capacity);
    table.keys   = (KeyType*)   (table.hashes + table.capacity);
    table.values = (ValueType*) (table.keys   + table.capacity);

    // Copy states and hashes
    platform_copy_memory(table.states, other.states, table.capacity * (sizeof(State) + sizeof(Hash)));

    u32 remaining = table.filled;
    for (u32 i = 0; remaining > 0 && i < table.capacity; i++)
    {
        // Copy keys and values when required
        if (table.states[i] == State::ALIVE)
        {
            table.keys[i]   = copy(other.keys[i]);
            table.values[i] = copy(other.values[i]);
            remaining--;
        }
    }

    return table;
}

HASH_TABLE_TEMPLATE
inline void free(HashTable<KeyType, ValueType, Hasher>& table)
{
    platform_free(table.states);

    table.states = nullptr;
    table.hashes = nullptr;
    table.keys   = nullptr;
    table.values = nullptr;
    table.capacity = table.filled = 0;
}

HASH_TABLE_TEMPLATE
inline void free_keys(HashTable<KeyType, ValueType, Hasher>& table)
{
    using HashTable = HashTable<KeyType, ValueType, Hasher>;

    u32 remaining = table.filled;
    for (u32 i = 0; remaining > 0 && i < table.capacity; i++)
    {
        if (table.states[i] == HashTable::State::ALIVE)
        {
            free(table.keys[i]);
            remaining--;
        }
    }
}

HASH_TABLE_TEMPLATE
inline void free_values(HashTable<KeyType, ValueType, Hasher>& table)
{
    using HashTable = HashTable<KeyType, ValueType, Hasher>;
    
    u32 remaining = table.filled;
    for (u32 i = 0; remaining > 0 && i < table.capacity; i++)
    {
        if (table.states[i] == HashTable::State::ALIVE)
        {
            free(table.values[i]);
            remaining--;
        }
    }
}

HASH_TABLE_TEMPLATE
inline void free_all(HashTable<KeyType, ValueType, Hasher>& table)
{
    using HashTable = HashTable<KeyType, ValueType, Hasher>;

    // Free keys and values
    u32 remaining = table.filled;
    for (u32 i = 0; remaining > 0 && i < table.capacity; i++)
    {
        if (table.states[i] == HashTable::State::ALIVE)
        {
            free(table.keys[i]);
            free(table.values[i]);
            remaining--;
        }
    }

    free(table);
}

HASH_TABLE_TEMPLATE
void resize(HashTable<KeyType, ValueType, Hasher>& table, u32 new_capacity)
{
    gn_assert_with_message(new_capacity > table.capacity, "Table can't be resized to be smaller than before! (new_capacity: %, old_capacity: %)", new_capacity, table.capacity);

    using HashTable = HashTable<KeyType, ValueType, Hasher>;
    using State     = HashTable::State;

    HashTable new_table;
    new_table.capacity = new_capacity;
    new_table.filled   = table.filled;

    const u64 size_in_bytes = new_table.capacity * (sizeof(State) + sizeof(Hash) + sizeof(KeyType) + sizeof(ValueType));
    void* allocation = platform_allocate(size_in_bytes);
    gn_assert_with_message(allocation, "Could not allocate data for resizing hash table!");

    new_table.states = (State*)     (allocation);
    new_table.hashes = (Hash*)      (new_table.states + new_table.capacity);
    new_table.keys   = (KeyType*)   (new_table.hashes + new_table.capacity);
    new_table.values = (ValueType*) (new_table.keys   + new_table.capacity);

    platform_set_memory(new_table.states, (int) State::EMPTY, new_table.capacity * sizeof(State));

    u32 elements_to_copy = table.filled;
    for (u32 old_index = 0; elements_to_copy > 0 && old_index < table.capacity; old_index++)
    {
        if (table.states[old_index] != State::ALIVE)
            continue;

        // Need to copy element
        elements_to_copy--;

        const Hash hash = table.hashes[old_index];
        const u32 end_index   = hash % new_table.capacity;
        const u32 start_index = (end_index + 1) % new_table.capacity;

        for (u32 i = start_index; i != end_index; i = (i + 1) % new_table.capacity)
        {
            if (new_table.states[i] == State::EMPTY)
            {
                new_table.states[i] = State::ALIVE;
                new_table.hashes[i] = hash;
                new_table.keys[i]   = table.keys[old_index];
                new_table.values[i] = table.values[old_index];

                break;
            }
        }
    }

    gn_assert_with_message(elements_to_copy == 0, "Not all elements were copied when resizing hash table! (elements left to copy: %)", elements_to_copy);

    free(table.states);
    table = new_table;
}

HASH_TABLE_TEMPLATE
HashTableElement<KeyType, ValueType, Hasher> find(const HashTable<KeyType, ValueType, Hasher>& table, const KeyType& key)
{
    using HashTable        = HashTable<KeyType, ValueType, Hasher>;
    using HashTableElement = HashTableElement<KeyType, ValueType, Hasher>;
    using State            = HashTable::State;

    const Hash hash = table.hasher(key);
    const u32 end_index   = hash % table.capacity;
    const u32 start_index = (end_index + 1) % table.capacity;

    for (u32 i = start_index; i != end_index; i = (i + 1) % table.capacity)
    {
        switch (table.states[i])
        {
            case State::EMPTY:
                return HashTableElement { &table, table.capacity };
            
            case State::ALIVE:
            {
                if (hash == table.hashes[i] &&
                    key  == table.keys[i])
                {
                    return HashTableElement { &table, i };
                }
            } break;
        }
    }

    gn_assert_with_message(false, "Reached end of hash table without finding an empty or valid element!");
    return HashTableElement { &table, table.capacity };
}

HASH_TABLE_TEMPLATE
HashTableElement<KeyType, ValueType, Hasher> put(HashTable<KeyType, ValueType, Hasher>& table, const KeyType& key, const ValueType& value)
{
    using HashTable        = HashTable<KeyType, ValueType, Hasher>;
    using HashTableElement = HashTableElement<KeyType, ValueType, Hasher>;
    using State            = HashTable::State;

    const float load = (float) table.filled / (float) table.capacity;
    if (load >= HASH_TABLE_MAX_LOAD_FACTOR)
        resize(table, table.capacity * 2);

    const Hash hash = table.hasher(key);
    const u32 end_index   = hash % table.capacity;
    const u32 start_index = (end_index + 1) % table.capacity;

    for (u32 i = start_index; i != end_index; i = (i + 1) % table.capacity)
    {
        switch (table.states[i])
        {
            case State::EMPTY:
            case State::TOMBSTONE:
            {
                table.filled++;

                table.states[i] = State::ALIVE;
                table.hashes[i] = hash;
                table.keys[i]   = key;
                table.values[i] = value;
                return HashTableElement { &table, i };
            }
            
            case State::ALIVE:
            {
                if (hash == table.hashes[i] &&
                    key  == table.keys[i])
                {
                    return HashTableElement { &table, i };
                }
            } break;
        }
    }

    // Never reached
    gn_assert_with_message(false, "Ran out of entries in hash table to place element!");
    return HashTableElement { &table, table.capacity };
}

HASH_TABLE_TEMPLATE
inline void remove(HashTableElement<KeyType, ValueType, Hasher>& element)
{
    using HashTable        = HashTable<KeyType, ValueType, Hasher>;
    using HashTableElement = HashTableElement<KeyType, ValueType, Hasher>;
    using State            = HashTable::State;

    HashTable& table = *(HashTable*)element.table;

    if (element.index >= table.capacity || table.states[element.index] != State::ALIVE)
    {
        gn_assert_with_message(false, "Trying to delete a non existing element in hash table! (table index: %)", element.index);
        return;
    }
    
    table.states[element.index] = State::TOMBSTONE;
    table.filled--;
    
    element.index = table.capacity;
}

#undef HASH_TABLE_MAX_LOAD_FACTOR
#undef HASH_TABLE_TEMPLATE