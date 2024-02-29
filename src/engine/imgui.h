#pragma once

#include "application/application.h"
#include "core/types.h"
#include "containers/bytes.h"
#include "containers/hash_table.h"
#include "containers/function.h"
#include "graphics/texture.h"
#include "math/math.h"
#include "serialization/json/json_document.h"
#include "rect.h"
#include "sprite.h"

namespace Imgui
{

// Initialization and Shutdown
void init(const Application& app);
void shutdown();

// Begin and End UI Pass
void begin();
void end();

// Required for proper functioning UI
void update();

struct ID
{
    s32 primary;
    s32 secondary;
    
    inline bool operator==(const ID& other)
    {
        return primary == other.primary &&
                secondary == other.secondary;
    }

    inline bool operator!=(const ID& other)
    {
        return primary != other.primary ||
                secondary != other.secondary;
    }
};

using Callback = Function<void(ID)>;
using Image = Texture;

struct Font
{
    struct GlyphData
    {
        f32 advance;
        Vector4 plane_bounds;
        Vector4 atlas_bounds;
    };

    enum struct Type
    {
        HARDMASK,
        SOFTMASK,
        SDF
    };

    using KerningTable = HashTable<s32, f32>;

    Texture atlas;
    Type type;

    u32 size;
    f32 line_height;
    f32 ascender, descender;

    GlyphData glyphs[127 - ' '];
    KerningTable kerning_table = {};
};

// Deserialization
Font font_load_from_json(const Json::Document& document, const String atlas_path);
Font font_load_from_bytes(const Bytes& bytes);

// Utility Functions
Vector2 get_rendered_text_size(const String text, const Font& font, f32 size = -1.0f);
Vector2 get_rendered_char_size(const char ch, const Font& font, f32 size = -1.0f);

void set_offset(f32 x, f32 y);
void set_scale(f32 x, f32 y);

// Set and Reset Callbacks
void register_button_callback(const Callback& callback);

// Rendering UI
void render_rect(const Rect& rect, f32 z, const Vector4& color);
void render_overlap_rect(ID id, const Rect& rect, f32 z, const Vector4& color);   // Overlap rect prevents items under it from being interacted with
void render_image(const Image& image, const Vector2& top_left, f32 z, const Vector2& size = Vector2(-1.0f), const Vector4& tint = Vector4(1.0f));
void render_sprite(const Sprite& sprite, const Vector2& position, f32 z, const Vector2& scale = Vector2(1.0f), const Vector4& tint = Vector4(1.0f));
bool render_button(ID id, const Rect& rect, f32 z, const Vector4& default_color = Vector4(0.5f, 0.5f, 0.5f, 0.0f), const Vector4& hover_color = Vector4(0.5f, 0.5f, 0.5f, 0.4f), const Vector4& pressed_color = Vector4(0.35f, 0.35f, 0.35f, 0.7f));

void render_text(const String text, const Font& font, const Vector2& top_left, f32 z, f32 size = -1.0f, const Vector4& tint = Vector4(1.0f));
void render_char(const char ch, const Font& font, const Vector2& top_left, f32 z, f32 size = -1.0f, const Vector4& tint = Vector4(1.0f));

// Uses default colors for now
bool render_text_button(ID id, const Rect& rect, const String text, const Font& font, const Vector2 padding, f32 z, f32 size = -1.0f);
f32 render_slider_01(ID id, f32 value, const Rect& area, const Vector2& handle_size, f32 z, bool enabled = true, const Vector4& filled_color = Vector4 { 0.2f, 0.1f, 1.0f, 1.0f }, const Vector4& disabled_color = Vector4 { 0.2f, 0.2f, 0.2f, 1.0f }, const Vector4& bar_color = Vector4 { 0.5f, 0.5f, 0.5f, 1.0f }, const Vector4& handle_color = Vector4 { 0.75f, 0.75f, 0.75f, 1.0f });
f32 render_slider(ID id, f32 value, const f32 min, const f32 max, const Rect& area, const Vector2& handle_size, f32 z, bool enabled = true, const Vector4& filled_color = Vector4 { 0.2f, 0.1f, 1.0f, 1.0f }, const Vector4& disabled_color = Vector4 { 0.2f, 0.2f, 0.2f, 1.0f }, const Vector4& bar_color = Vector4 { 0.5f, 0.5f, 0.5f, 1.0f }, const Vector4& handle_color = Vector4 { 0.75f, 0.75f, 0.75f, 1.0f });

} // namespace Imgui

void free(Imgui::Font& font);

#define imgui_gen_id() (Imgui::ID { __LINE__, 0 })
#define imgui_gen_id_with_secondary(sec) (Imgui::ID { __LINE__, (sec) })