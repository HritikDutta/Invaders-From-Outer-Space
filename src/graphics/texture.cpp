#include "texture.h"

#include "core/types.h"
#include "containers/string.h"
#include "containers/hash_table.h"

#include <stb_image.h>
#include <glad/glad.h>

static HashTable<String, Texture> loaded_textures = make<HashTable<String, Texture>>();

// OpenGL generates textureIDs sequentially so
// this way extra data about the texture can be accessed
// by using them as an index into an array rather than
// copying them everywhere.

struct TextureData
{
    s32 width, height, bytes_pp;
    String name;
};

constexpr u32 max_loaded_textures = 10;
static TextureData texture_data_table[max_loaded_textures] = {};

static inline Texture internal_create_texture()
{
    Texture texture;
    glGenTextures(1, &texture.id);
    return texture;
}

static inline void internal_set_pixels(Texture& texture, u8* pixels, s32 width, s32 height, s32 bytes_pp, const TextureSettings& settings)
{
    GLint internal_format, format;
    switch (bytes_pp)
    {
        case 1:
        {
            internal_format = GL_R8;
            format = GL_RED;
        } break;

        case 3:
        {
            internal_format = GL_RGB8;
            format = GL_RGB;
        } break;

        case 4:
        {
            internal_format = GL_RGBA8;
            format = GL_RGBA;
        } break;

        default:
        {
            gn_assert_with_message(false, "Bytes per pixel value is not currently supported! (bytes per pixel: %)", bytes_pp);
        } break;
    }

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glBindTexture(GL_TEXTURE_2D, texture.id);
    glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0, format, GL_UNSIGNED_BYTE, pixels);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (GLint) settings.min_filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (GLint) settings.max_filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, (GLint) settings.wrap_s);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, (GLint) settings.wrap_t);
}

static inline void internal_set_texture_data(const Texture& texture, const String name, s32 width, s32 height, s32 bytes_pp)
{
    texture_data_table[texture.id].width    = width;
    texture_data_table[texture.id].height   = height;
    texture_data_table[texture.id].bytes_pp = bytes_pp;
    texture_data_table[texture.id].name     = name;
}

Texture texture_load_file(const String filepath, const TextureSettings& settings, s32 desired_channels)
{
    stbi_set_flip_vertically_on_load(true);

    auto tex = find(loaded_textures, filepath);
    if (tex)
        return tex.value();
    
    s32 width, height, bytes_pp;
    u8* pixels = stbi_load(filepath.data, &width, &height, &bytes_pp, desired_channels);
    gn_assert_with_message(pixels, "Couldn't load image data! (filepath: \"%\")", filepath);

    if (desired_channels != 0)
        bytes_pp = desired_channels;

    Texture texture = internal_create_texture();
    internal_set_pixels(texture, pixels, width, height, bytes_pp, settings);
    internal_set_texture_data(texture, filepath, width, height, bytes_pp);
    put(loaded_textures, filepath, texture);

    stbi_image_free(pixels);
    return texture;
}

Texture texture_load_pixels(const String name, u8* pixels, s32 width, s32 height, s32 bytes_pp, const TextureSettings& settings)
{
    auto tex = find(loaded_textures, name);
    if (tex)
        return tex.value();

    Texture texture = internal_create_texture();
    internal_set_pixels(texture, pixels, width, height, bytes_pp, settings);
    internal_set_texture_data(texture, name, width, height, bytes_pp);
    put(loaded_textures, name, texture);   

    return texture;
}

void texture_set_pixels(Texture& texture, u8* pixels, s32 width, s32 height, s32 bytes_pp,  const TextureSettings& settings)
{
    internal_set_pixels(texture, pixels, width, height, bytes_pp, settings);
    internal_set_texture_data(texture, texture_get_name(texture), width, height, bytes_pp);
}

void free(Texture& texture)
{
    if (!texture.id)
        return;

    auto elem = find(loaded_textures, texture_data_table[texture.id].name);
    gn_assert_with_message((bool) elem, "Texture id is not 0 but hasn't been loaded properly!");
    remove(elem);

    glDeleteTextures(1, &texture.id);
    texture.id = 0;
}

// Assuming there are 32 texture slots in the GPU
static u32 bound_textures[32] = {};

void texture_bind(const Texture& texture, s32 slot)
{
    // Only bind if the texture wasn't bound before
    if (bound_textures[slot] != texture.id)
    {
        glActiveTexture(GL_TEXTURE0 + slot);
        glBindTexture(GL_TEXTURE_2D, texture.id);
        
        bound_textures[slot] = texture.id;
    }
}

s32 texture_get_width(const Texture& texture)
{
    return texture_data_table[texture.id].width;
}

s32 texture_get_height(const Texture& texture)
{
    return texture_data_table[texture.id].height;
}

s32 texture_get_bytes_pp(const Texture& texture)
{
    return texture_data_table[texture.id].bytes_pp;
}

const String texture_get_name(const Texture& texture)
{
    return texture_data_table[texture.id].name;
}

bool texture_get_existing(const String name, Texture& out_texture)
{
    auto tex = find(loaded_textures, name);
    if (!tex)
        return false;

    out_texture = tex.value();
    return true;
}