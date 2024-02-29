#pragma once

#include "core/types.h"
#include "graphics/shader.h"
#include "graphics/texture.h"

template <typename VertexType, u32 max_tex_count>
struct BatchData
{
    Shader shader;

    VertexType* elem_vertices_buffer, *elem_vertices_ptr;
    u32 elem_count;

    // Texture data
    Texture textures[max_tex_count];
    u32 next_active_tex_slot;  // Should always be lower than max allowed textures
};

template <typename VertexType, u32 max_tex_count>
inline void batch_begin(BatchData<VertexType, max_tex_count>& batch)
{
    batch.elem_vertices_ptr = batch.elem_vertices_buffer;
    batch.next_active_tex_slot = 0;
    batch.elem_count = 0;
}