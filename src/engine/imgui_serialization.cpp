#include "imgui_serialization.h"

#include "containers/bytes.h"
#include "containers/string.h"
#include "core/types.h"
#include "serialization/binary.h"
#include "imgui.h"

#include <stb_image.h>

namespace Imgui
{
    
Bytes font_encode_to_bytes(const Font& font)
{
    s32 image_size = texture_get_width(font.atlas) * texture_get_height(font.atlas) * 4;

    DynamicArray<u8> bytes = make<DynamicArray<u8>>(sizeof(Font) + image_size);

    append(bytes, Binary::OBJECT_START);
    
    // Font Type
    append(bytes, Binary::INTEGER_U8);
    Binary::append_integer(bytes, (u8) font.type);

    {   // Font size
        if (font.size <= 0xff)
        {
            append(bytes, Binary::INTEGER_U8);
            Binary::append_integer(bytes, (u8) font.size);
        }
        else if (font.size <= 0xffff)
        {
            append(bytes, Binary::INTEGER_U16);
            Binary::append_integer(bytes, (u16) font.size);
        }
        else if (font.size <= 0xffffffff)
        {
            append(bytes, Binary::INTEGER_U32);
            Binary::append_integer(bytes, (u32) font.size);
        }
        // Font sizes won't go above this!
    }

    {   // Metrics
        append(bytes, Binary::FLOAT_32);
        Binary::append_float(bytes, font.line_height);
        
        append(bytes, Binary::FLOAT_32);
        Binary::append_float(bytes, font.ascender);
        
        append(bytes, Binary::FLOAT_32);
        Binary::append_float(bytes, font.descender);
    }

    {   // Glyph Data (encoded as an array of bytes)
        append(bytes, Binary::BYTE_ARRAY_2_BYTE);
        Binary::append_integer(bytes, (u16) sizeof(font.glyphs));
        append_many(bytes, (u8*) font.glyphs, sizeof(font.glyphs));
    }

    {   // Kerning
        u32 count = font.kerning_table.filled;
        append(bytes, Binary::ARRAY_2_BYTE);
        Binary::append_integer(bytes, (u16) (count * 2));

        for (u32 i = 0; count > 0 && i < font.kerning_table.capacity; i++)
        {
            if (font.kerning_table.states[i] == Font::KerningTable::State::ALIVE)
            {
                append(bytes, Binary::INTEGER_S32);
                Binary::append_integer(bytes, font.kerning_table.keys[i]);
                
                append(bytes, Binary::FLOAT_32);
                Binary::append_float(bytes, font.kerning_table.values[i]);

                count--;
            }
        }
    }

    {   // Texture Data
        stbi_set_flip_vertically_on_load(true);

        const String atlas_path = texture_get_name(font.atlas);

        s32 width, height, bytes_pp;
        u8* pixels = stbi_load(atlas_path.data, &width, &height, &bytes_pp, 0);

        Binary::append_image(bytes, atlas_path, pixels, width, height, bytes_pp);

        stbi_image_free(pixels);
    }

    append(bytes, Binary::OBJECT_END);

    if (bytes.size != bytes.capacity)
        resize(bytes, bytes.size);  // Shrink the array to free extra memory

    return Bytes { bytes.data, bytes.size };
}

} // namespace Imgui