#include "sprite_serialization.h"

#include "containers/darray.h"
#include "containers/string.h"
#include "containers/string_builder.h"
#include "core/logger.h"
#include "rect.h"
#include "sprite.h"

// If wrong name is encountered, then None is returned
static Animation2D::LoopType get_loop_type_from_string(const String type_string)
{
    Animation2D::LoopType type = Animation2D::LoopType::NONE;

    if (type_string == ref("Cycle"))
        type = Animation2D::LoopType::CYCLE;
    else if (type_string == ref("Ping Pong"))
        type = Animation2D::LoopType::PING_PONG;
    else
    {
        gn_warn_if(type_string != ref("None"), "Encountered wrong Animation Loop Type string! Returning None. (given string: %)", type_string);
    }

    return type;
}

bool animation_load_from_json(const Json::Document& document, DynamicArray<Animation2D>& anims)
{
    const auto& j_data = document.start();

    String filename;

    {   // Get filename
        StringBuilder builder = make<StringBuilder>(3Ui64);

        append(builder, j_data[ref("directory")].string());
        append(builder, ref("\\", 1));
        append(builder, j_data[ref("file")].string());
        append(builder, ref("", 1));    // null terminator

        filename = build_string(builder);

        free(builder);
    }

    Texture atlas = texture_load_file(filename, TextureSettings::default());

    // Load Animations
    const Json::Array& j_animations = j_data[ref("animations")].array();

    clear(anims);
    resize(anims, j_animations.size());

    for (u64 i = 0; i < j_animations.size(); i++)
    {
        const auto& j_anim_data = j_animations[i];

        Animation2D anim;

        anim.name       = j_anim_data[ref("name")].string();
        anim.frame_rate = 1.0f / j_anim_data[ref("frameRate")].float64();
        anim.loop_type  = get_loop_type_from_string(j_anim_data[ref("loopType")].string());

        const Json::Array& j_frames = j_anim_data[ref("frames")].array();
        anim.sprites = make<DynamicArray<Sprite>>(j_frames.size());

        for (u64 fi = 0; fi < j_frames.size(); fi++)
        {
            Sprite sprite;

            sprite.atlas = atlas;

            {   // Load Sprite Rect and Pivot
                const Json::Value& j_frame = j_frames[fi];

                s64 left   = j_frame[ref("left")].int64();
                s64 top    = j_frame[ref("top")].int64();
                s64 right  = j_frame[ref("right")].int64();
                s64 bottom = j_frame[ref("bottom")].int64();

                // Texture Coords
                sprite.tex_coords.left   = (f32) left   / (f32) texture_get_width(atlas);
                sprite.tex_coords.top    = (f32) top    / (f32) texture_get_height(atlas);
                sprite.tex_coords.right  = (f32) right  / (f32) texture_get_width(atlas);
                sprite.tex_coords.bottom = (f32) bottom / (f32) texture_get_height(atlas);

                // Size
                sprite.size.x = right - left;
                sprite.size.y = top - bottom;

                // Pivot
                sprite.pivot.x = j_frame[ref("pivot_x")].float64();
                sprite.pivot.y = j_frame[ref("pivot_y")].float64();
            }

            append(anim.sprites, sprite);
        }

        append(anims, anim);
    }

    free(filename);

    return true;
}