#pragma once

#include "core/types.h"
#include "containers/string.h"

#include <glad/glad.h>

struct TextureSettings
{
    enum struct Filter
    {
        NEAREST = GL_NEAREST,
        LINEAR  = GL_LINEAR
    };

    enum struct Wrapping
    {
        CLAMP           = GL_CLAMP,
        CLAMP_TO_BORDER = GL_CLAMP_TO_BORDER,
        CLAMP_TO_EDGE   = GL_CLAMP_TO_EDGE,
        MIRRORED_REPEAT = GL_MIRRORED_REPEAT,
        REPEAT          = GL_REPEAT
    };

    Filter min_filter = Filter::NEAREST;
    Filter max_filter = Filter::NEAREST;

    Wrapping wrap_s = Wrapping::REPEAT;
    Wrapping wrap_t = Wrapping::REPEAT;

    static TextureSettings default() { return TextureSettings(); }
};

struct Texture
{
    u32 id;
};

Texture texture_load_file(const String filepath, const TextureSettings& settings, s32 desired_channels = 0);
Texture texture_load_pixels(const String name, u8* pixels, s32 width, s32 height, s32 bytes_pp, const TextureSettings& settings);
void free(Texture& texture);

void texture_bind(const Texture& texture, s32 slot);

s32 texture_get_width(const Texture& texture);
s32 texture_get_height(const Texture& texture);
s32 texture_get_bytes_pp(const Texture& texture);
const String texture_get_name(const Texture& texture);

bool texture_get_existing(const String name, Texture& out_texture);

void texture_set_pixels(Texture& texture, u8* pixels, s32 width, s32 height, s32 bytes_pp,  const TextureSettings& settings);