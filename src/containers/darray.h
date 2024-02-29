#pragma once

#include "core/common.h"
#include "core/logger.h"
#include "core/types.h"
#include "math/common.h"
#include "platform/platform.h"

template <typename T>
struct DynamicArray
{
    T*  data;
    u64 size;
    u64 capacity;

    T& operator[](const u64 index)
    {
        gn_assert_with_message(index < size, "Index out of bounds! (index: %, array size: %)", index, size);
        return data[index];
    }
    
    const T& operator[](const u64 index) const
    {
        gn_assert_with_message(index < size, "Index out of bounds! (index: %, array size: %)", index, size);
        return data[index];
    }
};

template <typename T>
inline DynamicArray<T> make(Type<DynamicArray<T>>, u64 start_cap = 16)
{
    DynamicArray<T> arr;

    arr.capacity = start_cap;
    arr.size = 0;
    arr.data = (T*) platform_allocate(arr.capacity * sizeof(T));
    gn_assert_with_message(arr.data, "Could not allocate data for array!");

    return arr;
}

template <typename T>
inline DynamicArray<T> copy(const DynamicArray<T>& other)
{
    DynamicArray<T> arr;

    arr.capacity = other.capacity;
    arr.size = other.size;
    arr.data = (T*) platform_allocate(arr.capacity * sizeof(T));
    gn_assert_with_message(arr.data, "Could not allocate data for array!");

    for (u64 i = 0; i < arr.size; i++)
        arr.data[i] = copy(other.data[i]);

    return arr;
}

template <typename T>
inline void free(DynamicArray<T>& arr)
{
    platform_free(arr.data);

    arr.data = nullptr;
    arr.capacity = arr.size = 0;
}

template <typename T>
inline void free_all(DynamicArray<T>& arr)
{
    for (u64 i = 0; i < arr.size; i++)
        free(arr.data[i]);

    free(arr);
}

template <typename T>
inline void clear(DynamicArray<T>& arr)
{
    arr.size = 0;
}

template <typename T>
inline void clear_with_free(DynamicArray<T>& arr)
{
    for (u64 i = 0; i < arr.size; i++)
        free(arr.data[i]);
        
    arr.size = 0;
}

template <typename T>
inline void resize(DynamicArray<T>& arr, u64 new_capacity)
{
    T* new_data = (T*) platform_reallocate(arr.data, new_capacity * sizeof(T));
    gn_assert_with_message(new_data, "Could not reallocate data for array!");

    arr.capacity = new_capacity;
    arr.data = new_data;
}

template <typename T>
inline DynamicArray<T>& append(DynamicArray<T>& arr, const T& elem)
{
    if (arr.size >= arr.capacity)
        resize(arr, max(2 * arr.capacity, 16ui64));
    
    arr.data[arr.size++] = elem;
    return arr;
}

template <typename T>
inline DynamicArray<T>& append_many(DynamicArray<T>& arr, const T* elems, u64 count)
{
    if ((arr.size + count) >= arr.capacity)
        resize(arr, max(2 * arr.capacity, 2 * (arr.size + count)));
    
    platform_copy_memory(arr.data + arr.size, elems, count * sizeof(T));
    arr.size += count;
    
    return arr;
}

template <typename T>
inline DynamicArray<T>& insert(DynamicArray<T>& arr, u64 index, const T& elem)
{
    gn_assert_with_message(index < arr.size,  "Trying to insert at an out of bounds index! (index: %, array size: %)", index, arr.size);

    if (arr.size >= arr.capacity)
        resize(arr, max(2 * arr.capacity, 16ui64));

    // Move all values ahead by 1 index    
    for (u64 i = arr.size; i > index; i--)
        arr.data[i] = arr.data[i - 1];

    arr.data[index] = elem;
    arr.size++;

    return arr;
}

template <typename T>
inline T pop(DynamicArray<T>& arr)
{
    gn_assert_with_message(arr.size > 0, "Trying to pop elements from an array that has 0 elements!");
    return arr.data[--arr.size];
}

template <typename T>
inline T remove(DynamicArray<T>& arr, u64 index)
{
    gn_assert_with_message(arr.size > 0, "Trying to remove elements from an array that has 0 elements!");
    gn_assert_with_message(index < arr.size,  "Trying to remove from an out of bounds index! (index: %, array size: %)", index, arr.size);

    T removed = arr.data[index];

    // Move all values back by 1 index
    for (u64 i = index; i < arr.size - 1; i++)
        arr.data[i] = arr.data[i + 1];
    
    arr.size--;

    return removed;
}

template <typename T>
inline T remove_swap(DynamicArray<T>& arr, u64 index)
{
    gn_assert_with_message(arr.size > 0, "Trying to remove elements from an array that has 0 elements!");
    gn_assert_with_message(index < arr.size,  "Trying to remove from an out of bounds index! (index: %, array size: %)", index, arr.size);

    T removed = arr.data[index];

    arr.size--;
    arr.data[index] = arr.data[arr.size];

    return removed;
}

template <typename T>
inline u64 find(const DynamicArray<T>& arr, const T& needle)
{
    for (u64 i = 0; i < arr.size; i++)
    {
        if (arr[i] == needle)
            return i;
    }

    return arr.size;
}

template <typename T>
inline void shuffle(DynamicArray<T>& arr)
{
    for (u64 i = 0; i < arr.size; i++)
    {
        u64 swap_with_index = (u64) Math::floor(arr.size * Math::random());
        swap(arr[i], arr[swap_with_index]);
    }
}

template <template <typename> typename ArrayType, typename T>
void print_to_file(FILE* file, const ArrayType<T>& array)
{
    print_to_file(file, "{ ");

    // Only display up to six items
    if (array.size <= 6)
    {
        for (u64 i = 0; i < array.size; i++)
        {
            print_to_file(file, array[i]);

            if (i < array.size - 1)
                print_to_file(file, ", ");
        }
    }
    else
    {
        // Display the first 3 elements
        for (u64 i = 0; i < 3; i++)
        {
            print_to_file(file, array[i]);
            print_to_file(file, ", ");
        }

        print_to_file(file, "..., ");

        // Display the last 3 elements
        for (u64 i = array.size - 3; i < array.size; i++)
        {
            print_to_file(file, array[i]);

            if (i < array.size - 1)
                print_to_file(file, ", ");
        }
    }

    print_to_file(file, " } (size: %, capacity: %)", array.size, array.capacity);
}