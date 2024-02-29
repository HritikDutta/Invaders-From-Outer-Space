#include "imgui.h"

#include "application/application.h"
#include "core/types.h"
#include "core/logger.h"
#include "core/input.h"
#include "platform/platform.h"
#include "containers/bytes.h"
#include "containers/darray.h"
#include "containers/hash_table.h"
#include "graphics/texture.h"
#include "graphics/shader.h"
#include "math/math.h"
#include "serialization/json/json_document.h"
#include "serialization/binary.h"
#include "batch.h"
#include "rect.h"
#include "shader_paths.h"

#include <glad/glad.h>

#define imgui_invalid_id (ID { -1, -1 })

namespace Imgui
{

static const Application* active_app = nullptr;

static constexpr s32 max_quad_count = 500;
static constexpr s32 max_tex_count  = 10;
static s32 active_tex_slots[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };

static constexpr f32 z_offset = -0.0000001f;

struct Vertex
{
    Vector3 position;
    Vector2 tex_coord;
    Vector4 color;
    float tex_index;
};

using ImguiBatchData = BatchData<Vertex, max_tex_count>;

struct UIStateData
{
    ID hot, active;
    ID interacted;
};

static struct
{
    u32 vao, vbo, ibo;
    ImguiBatchData quad_batch;
    ImguiBatchData font_batch;

    Texture white_texture;

    UIStateData state_prev_frame;
    UIStateData state_current_frame;

    Vector4 offset_v2;  // x,z and y,w are the same
    Vector4 scale_v2;   // x,z and y,w are the same

    Vertex* batch_shared_buffer = nullptr;

    DynamicArray<Callback> button_callbacks;
    bool batch_begun = false;
} ui_data;

static void init_white_texture(int width, int height)
{
    // Use existing white texture if available
    if (texture_get_existing(ref("White Texture"), ui_data.white_texture))
        return;
    
    u8* pixels = (u8*) platform_allocate(width * height * 4);
    platform_set_memory(pixels, 0xFF, width * height * 4);

    TextureSettings settings;
    settings.min_filter = settings.max_filter = TextureSettings::Filter::NEAREST;
    ui_data.white_texture = texture_load_pixels(ref("White Texture"), pixels, width, height, 4, settings);

    platform_free(pixels);
}

static void init_batches()
{
    // Set up batching for elems
    constexpr size_t batch_size = 4 * max_quad_count;
    ui_data.batch_shared_buffer = (Vertex*) platform_allocate(2 * batch_size * sizeof(Vertex));

    {   // Init Quad Batch
        ui_data.quad_batch.elem_vertices_buffer = ui_data.batch_shared_buffer;

        // Compile Shaders
        gn_assert_with_message(
            shader_compile_from_file(ui_data.quad_batch.shader, ref(ui_quad_vert_shader_path), Shader::Type::VERTEX),
            "Failed to compile UI Quad Vertex Shader! (shader path: %)", ui_quad_vert_shader_path
        );

        gn_assert_with_message(
            shader_compile_from_file(ui_data.quad_batch.shader, ref(ui_quad_frag_shader_path), Shader::Type::FRAGMENT),
            "Failed to compile UI Quad Fragment Shader! (shader path: %)", ui_quad_frag_shader_path
        );

        gn_assert_with_message(
            shader_link(ui_data.quad_batch.shader),
            "Failed to link UI Quad Shader!"
        );
    }
    
    {   // Init Font Batch
        ui_data.font_batch.elem_vertices_buffer = ui_data.batch_shared_buffer + batch_size;

        // Compile Shaders
        gn_assert_with_message(
            shader_compile_from_file(ui_data.font_batch.shader, ref(ui_font_vert_shader_path), Shader::Type::VERTEX),
            "Failed to compile UI Font Vertex Shader! (shader path: %)", ui_font_vert_shader_path
        );

        gn_assert_with_message(
            shader_compile_from_file(ui_data.font_batch.shader, ref(ui_font_frag_shader_path), Shader::Type::FRAGMENT),
            "Failed to compile UI Font Fragment Shader! (shader path: %)", ui_font_frag_shader_path
        );

        gn_assert_with_message(
            shader_link(ui_data.font_batch.shader),
            "Failed to link UI Font Shader!"
        );
    }
}

void init(const Application& app)
{
    // Set currentlt active application
    gn_assert_with_message(active_app == nullptr, "Imgui was already initialized!");
    active_app = &app;

    glGenBuffers(1, &ui_data.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, ui_data.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * max_quad_count * 4, nullptr, GL_DYNAMIC_DRAW);

    glGenVertexArrays(1, &ui_data.vao);
    glBindVertexArray(ui_data.vao);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(Vertex), (const void*) offsetof(Vertex, position));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, false, sizeof(Vertex), (const void*) offsetof(Vertex, tex_coord));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, false, sizeof(Vertex), (const void*) offsetof(Vertex, color));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 1, GL_FLOAT, false, sizeof(Vertex), (const void*) offsetof(Vertex, tex_index));

    u32 indices[max_quad_count * 6];
    u32 offset = 0;
    for (int i = 0; i < max_quad_count * 6; i += 6)
    {
        indices[i + 0] = offset + 0;
        indices[i + 1] = offset + 1;
        indices[i + 2] = offset + 2;
        indices[i + 3] = offset + 2;
        indices[i + 4] = offset + 3;
        indices[i + 5] = offset + 0;
        offset += 4;
    }

    glGenBuffers(1, &ui_data.ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ui_data.ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    ui_data.state_current_frame.hot = ui_data.state_current_frame.active = ui_data.state_current_frame.interacted = imgui_invalid_id;
    ui_data.state_prev_frame.hot = ui_data.state_prev_frame.active = ui_data.state_prev_frame.interacted = imgui_invalid_id;

    init_batches();

    init_white_texture(4, 4);

    ui_data.batch_begun = false;

    ui_data.offset_v2 = Vector4(0.0f);
    ui_data.scale_v2  = Vector4(1.0f);

    ui_data.button_callbacks = make<DynamicArray<Callback>>();
}

void shutdown()
{
    gn_assert_with_message(active_app, "Imgui was never initialized!");

    free(ui_data.white_texture);

    platform_free(ui_data.batch_shared_buffer);

    glDeleteBuffers(1, &ui_data.vbo);
    glDeleteBuffers(1, &ui_data.ibo);
    glDeleteVertexArrays(1, &ui_data.vao);
}

void begin()
{
    gn_assert_with_message(active_app, "Imgui was never initialized!");

    batch_begin(ui_data.quad_batch);
    batch_begin(ui_data.font_batch);

    ui_data.batch_begun = true;
}

static void flush_batch(ImguiBatchData& batch)
{
    if (batch.elem_count == 0)
        return;
    
    shader_bind(batch.shader);

    // Set all textures for the batch
    for (u32 i = 0; i < batch.next_active_tex_slot; i++)
        texture_bind(batch.textures[i], i);
    
    shader_set_uniform_1iv(batch.shader, ref("u_textures"), batch.next_active_tex_slot, active_tex_slots);
    
    // Bind and Update Data
    GLsizeiptr size = (u8*) batch.elem_vertices_ptr - (u8*) batch.elem_vertices_buffer;
    glBufferSubData(GL_ARRAY_BUFFER, 0, size, batch.elem_vertices_buffer);

    // Draw Elements
    glDrawElements(GL_TRIANGLES, 6 * batch.elem_count, GL_UNSIGNED_INT, nullptr);
}

void end()
{
    gn_assert_with_message(active_app, "Imgui was never initialized!");

    // Both Quads and Font passes use the same vao, vbo, and ibo
    glBindVertexArray(ui_data.vao);
    glBindBuffer(GL_ARRAY_BUFFER, ui_data.vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ui_data.ibo);

    flush_batch(ui_data.quad_batch);
    flush_batch(ui_data.font_batch);

    ui_data.batch_begun = false;
}

void update()
{
    set_offset(0, 0);
    set_scale(1, 1);

    ui_data.state_prev_frame = ui_data.state_current_frame;
    ui_data.state_current_frame.hot = ui_data.state_current_frame.active = ui_data.state_current_frame.interacted = imgui_invalid_id;
    // That's pretty much it...
}


static inline s32 get_kerning_index(s32 a, s32 b)
{
    // Since all unicodes are less than 128
    return ((a << 8) | b);
}

static inline Font::Type get_font_type(const String type_string)
{
    if (type_string == ref("hardmask"))
        return Font::Type::HARDMASK;
    
    if (type_string == ref("softmask"))
        return Font::Type::SOFTMASK;

    gn_assert_with_message(type_string == ref("sdf", 3)  ||
                           type_string == ref("psdf", 4) ||
                           type_string == ref("msdf", 4) ||
                           type_string == ref("mtsdf", 5), "Font type not supported! (font type: %)", type_string);

    return Font::Type::SDF;
}

Font font_load_from_json(const Json::Document& document, const String atlas_path)
{
    Font font = {};

    // Load font altas
    font.atlas = texture_load_file(atlas_path, TextureSettings::default(), 4);

    // Load font data
    const Json::Value& data = document.start();

    const Json::Object& atlas = data[ref("atlas")].object();
    font.type = get_font_type(atlas[ref("type")].string());
    font.size = atlas[ref("size")].int64();
    const s32 texture_width  = atlas[ref("width")].int64();
    const s32 texture_height = atlas[ref("height")].int64();

    const Json::Object& metrics = data[ref("metrics")].object();
    font.line_height = metrics[ref("lineHeight")].float64();
    font.ascender    = metrics[ref("ascender")].float64();
    font.descender   = metrics[ref("descender")].float64();

    const Json::Array& glyphs = data[ref("glyphs")].array();
    for (u64 i = 0; i < glyphs.size(); i++)
    {
        const u32 unicode = glyphs[i][ref("unicode")].int64();
        
        Font::GlyphData& glyph_data = font.glyphs[unicode - ' '];

        glyph_data.advance = glyphs[i][ref("advance")].float64();

        {   // Plane bounds
            const Json::Value& plane_bounds = glyphs[i][ref("planeBounds")];

            if (plane_bounds.type() != Json::Type::NONE)
            {
                glyph_data.plane_bounds = Vector4 {
                    (f32) plane_bounds[ref("left")].float64(),
                    (f32) plane_bounds[ref("bottom")].float64(),
                    (f32) plane_bounds[ref("right")].float64(),
                    (f32) plane_bounds[ref("top")].float64()
                };
            }
        }

        {   // Atlas bounds
            const Json::Value& atlas_bounds = glyphs[i][ref("atlasBounds")];

            if (atlas_bounds.type() != Json::Type::NONE)
            {
                glyph_data.atlas_bounds = Vector4 {
                    (f32) atlas_bounds[ref("left")].float64()   / texture_width,
                    (f32) atlas_bounds[ref("top")].float64()    / texture_height,
                    (f32) atlas_bounds[ref("right")].float64()  / texture_width,
                    (f32) atlas_bounds[ref("bottom")].float64() / texture_height
                };
            }
        }
    }

    const Json::Array& kerning = data[ref("kerning")].array();
    font.kerning_table = make<Font::KerningTable>();
    for (u64 i = 0; i < kerning.size(); i++)
    {
        s32 k_index = get_kerning_index(kerning[i][(ref("unicode1"))].int64(), kerning[i][(ref("unicode2"))].int64());
        put(font.kerning_table, k_index, (f32) kerning[i][ref("advance")].float64());
    }

    return font;
}

Font font_load_from_bytes(const Bytes& bytes)
{
    Font font = {};

    u64 offset = 1; // Skip object start byte

    // Type
    font.type = (Font::Type) Binary::get<u8>(bytes, offset);

    // Size
    font.size = Binary::get<u32>(bytes, offset);

    // Metrics
    font.line_height = Binary::get<f32>(bytes, offset);
    font.ascender    = Binary::get<f32>(bytes, offset);
    font.descender   = Binary::get<f32>(bytes, offset);

    {   // Glyph Data
        Bytes glyph_data_bytes = Binary::get<Bytes>(bytes, offset);
        memcpy(font.glyphs, glyph_data_bytes.data, sizeof(font.glyphs));
    }

    {   // Kerning Data
        u32 num_kernings = Binary::get_next_uint(bytes, offset) / 2;

        const u32 kerning_table_size = 1.5f * (num_kernings);
        font.kerning_table = make<Font::KerningTable>(kerning_table_size);

        while (num_kernings--)
        {
            s32 key = Binary::get<s32>(bytes, offset);
            f32 advance = Binary::get<f32>(bytes, offset);
            put(font.kerning_table, key, advance);
        }
    }

    {   // Texture Data
        s32 width    = Binary::get<s32>(bytes, offset);
        s32 height   = Binary::get<s32>(bytes, offset);
        s32 bytes_pp = Binary::get<s32>(bytes, offset);

        String name = Binary::get<String>(bytes, offset);

        Bytes pixels = Binary::get<Bytes>(bytes, offset);

        font.atlas = texture_load_pixels(name, pixels.data, width, height, bytes_pp, TextureSettings::default());
    }

    gn_assert_with_message(offset == bytes.size - 1, "For some reason there's extra data in the font bytes! (file size: %, stopped parsing at: %)", bytes.size, offset);

    return font;
}

Vector2 get_rendered_text_size(const String text, const Font& font, f32 size)
{
    size = (size < 0.0f) ? font.size : size;

    Vector2 position = Vector2 {};
    position.y += size * font.ascender;

    Vector2 total_size = position;

    u64 line_start = 0;
    for (u64 i = 0; i < text.size; i++)
    {
        const char current_char = text[i];
        
        switch (current_char)        
        {
            case '\n':
            {
                total_size.y += size * font.line_height;
                position.y += size * font.line_height;

                total_size.x = max(total_size.x, position.x);
                position.x = 0.0f;

                line_start = i + 1;
            } break;

            case '\r':
            {
                position.x = 0.0f;
            } break;

            case '\t':
            {
                f32 x = size * font.glyphs[0].advance;
                position.x += x * (4 - ((i - line_start) % 4));
            } break;

            default:
            {
                if (i > 0)
                {
                    s32 kerning_index   = get_kerning_index(current_char, text[i - 1]);
                    const auto& kerning = find(font.kerning_table, kerning_index);
                    if (kerning)
                        position.x += size * kerning.value(); 
                }

                const Font::GlyphData& glyph = font.glyphs[current_char - ' '];
                position.x += size * glyph.advance;
            } break;
        }
    }

    total_size.x = max(total_size.x, position.x);
    return total_size;
}

Vector2 get_rendered_char_size(const char ch, const Font& font, f32 size)
{
    switch (ch)
    {
        case ' ' :
        case '\n':
        case '\r':
        case '\t':
            return Vector2 {};
    }

    size = (size < 0.0f) ? font.size : size;

    const Font::GlyphData& glyph = font.glyphs[ch - ' '];
    return Vector2 { size * glyph.advance, size * font.line_height };
}

static void push_ui_quad(ImguiBatchData& batch, const Rect& rect, f32 z, const Vector4& tex_coords, const Texture& texture, const Vector4& color)
{
    gn_assert_with_message(active_app, "Imgui was never initialized!");
    gn_assert_with_message(ui_data.batch_begun, "Imgui::begin() was never called!");

    if (batch.elem_count >= max_quad_count)
    {
        end();
        begin();
    }

    // Find if texture has already been set to active
    int texture_slot = batch.next_active_tex_slot;
    for (int i = 0; i < batch.next_active_tex_slot; i++)
    {
        if (batch.textures[i].id == texture.id)
        {
            texture_slot = i;
            break;
        }
    }

    if (texture_slot == batch.next_active_tex_slot)
    {
        // End the batch if all the texture slots are occupied
        if (texture_slot >= max_tex_count)
        {
            end();
            begin();

            texture_slot = 0;
        }

        batch.textures[texture_slot] = texture;
        batch.next_active_tex_slot++;
    }

    const Vector4 screen_size = Vector4 { (f32) active_app->window.ref_width, (f32) active_app->window.ref_height, (f32) active_app->window.ref_width, (f32) active_app->window.ref_height };
    Vector4 quad_positions = 2.0f * ((ui_data.scale_v2 * rect.v4 + ui_data.offset_v2) / screen_size) - Vector4(1);
    quad_positions *= Vector4 { 1.0f, -1.0f, 1.0f, -1.0f }; // Flip y axis coordinates

    batch.elem_vertices_ptr->position = Vector3(quad_positions.x, quad_positions.w, z);
    batch.elem_vertices_ptr->tex_coord = Vector2(tex_coords.s, tex_coords.v);
    batch.elem_vertices_ptr->color = color;
    batch.elem_vertices_ptr->tex_index = (f32) texture_slot;
    batch.elem_vertices_ptr++;

    batch.elem_vertices_ptr->position = Vector3(quad_positions.z, quad_positions.w, z);
    batch.elem_vertices_ptr->tex_coord = Vector2(tex_coords.u, tex_coords.v);
    batch.elem_vertices_ptr->color = color;
    batch.elem_vertices_ptr->tex_index = (f32) texture_slot;
    batch.elem_vertices_ptr++;

    batch.elem_vertices_ptr->position = Vector3(quad_positions.z, quad_positions.y, z);
    batch.elem_vertices_ptr->tex_coord = Vector2(tex_coords.u, tex_coords.t);
    batch.elem_vertices_ptr->color = color;
    batch.elem_vertices_ptr->tex_index = (f32) texture_slot;
    batch.elem_vertices_ptr++;

    batch.elem_vertices_ptr->position = Vector3(quad_positions.x, quad_positions.y, z);
    batch.elem_vertices_ptr->tex_coord = Vector2(tex_coords.s, tex_coords.t);
    batch.elem_vertices_ptr->color = color;
    batch.elem_vertices_ptr->tex_index = (f32) texture_slot;
    batch.elem_vertices_ptr++;

    batch.elem_count++;
}

void set_offset(f32 x, f32 y)
{
    ui_data.offset_v2 = Vector4 { x, y, x, y };
}

void set_scale(f32 x, f32 y)
{
    ui_data.scale_v2 = Vector4 { x, y, x, y };
}

void register_button_callback(const Callback& callback)
{
    append(ui_data.button_callbacks, callback);
}

void render_rect(const Rect& rect, f32 z, const Vector4& color)
{
    Vector4 tex_coords { 0.0f, 0.0f, 1.0f, 1.0f };
    push_ui_quad(ui_data.quad_batch, rect, z, tex_coords, ui_data.white_texture, color);
}

void render_overlap_rect(ID id, const Rect& rect, f32 z, const Vector4& color)
{
    Vector2 mpos  = Input::mouse_position();
    
    Rect adjusted_rect;
    adjusted_rect.v4 = ui_data.scale_v2 * rect.v4 + ui_data.offset_v2;

    if (mpos.x >= adjusted_rect.left && mpos.x <= adjusted_rect.right &&
        mpos.y >= adjusted_rect.top  && mpos.y <= adjusted_rect.bottom)
    {
        ui_data.state_current_frame.hot = id;
    }

    Vector4 tex_coords { 0.0f, 0.0f, 1.0f, 1.0f };
    push_ui_quad(ui_data.quad_batch, rect, z, tex_coords, ui_data.white_texture, color);
}

void render_image(const Image& image, const Vector2& top_left, f32 z, const Vector2& size, const Vector4& tint)
{
    const Vector4 tex_coords = Vector4 { 0.0f, 1.0f, 1.0f, 0.0f };

    Rect rect;

    rect.left   = top_left.x;
    rect.top    = top_left.y;
    rect.right  = top_left.x + ((size.x >= 0.0f) ? size.x : (f32) texture_get_width(image));
    rect.bottom = top_left.y + ((size.y >= 0.0f) ? size.y : (f32) texture_get_height(image));

    push_ui_quad(ui_data.quad_batch, rect, z, tex_coords, image, tint);
}

void render_sprite(const Sprite& sprite, const Vector2& position, f32 z, const Vector2& scale, const Vector4& tint)
{
    const Vector4 tex_coords = Vector4 { 0.0f, 1.0f, 1.0f, 0.0f };

    Rect sprite_rect;
    sprite_rect.left   = position.x - scale.x * sprite.size.x * sprite.pivot.x;
    sprite_rect.right  = position.x + scale.x * sprite.size.x * (1.0f - sprite.pivot.x);
    sprite_rect.top    = position.y - scale.y * sprite.size.y * (1.0f - sprite.pivot.y);
    sprite_rect.bottom = position.y + scale.y * sprite.size.y * sprite.pivot.y;

    push_ui_quad(ui_data.quad_batch, sprite_rect, z, sprite.tex_coords.v4, sprite.atlas, tint);
}

bool render_button(ID id, const Rect& rect, f32 z, const Vector4& default_color, const Vector4& hover_color, const Vector4& pressed_color)
{
    Vector4 color = default_color;
    Vector2 mpos  = Input::mouse_position();
    
    Rect adjusted_rect;
    adjusted_rect.v4 = ui_data.scale_v2 * rect.v4 + ui_data.offset_v2;

    // Calculations for the current frame
    if (mpos.x >= adjusted_rect.left && mpos.x <= adjusted_rect.right &&
        mpos.y >= adjusted_rect.top  && mpos.y <= adjusted_rect.bottom)
    {
        ui_data.state_current_frame.hot = id;

        if (Input::get_mouse_button_down(MouseButton::LEFT))
        {
            if (ui_data.state_prev_frame.active != id && ui_data.state_prev_frame.hot == id)
            {
                ui_data.state_current_frame.interacted = id;

                for (u64 i = 0; i < ui_data.button_callbacks.size; i++)
                    ui_data.button_callbacks[i](id);
            }

            ui_data.state_current_frame.active = id;
        }
    }

    if (ui_data.state_prev_frame.hot == id)
    {
        color = hover_color;
    }

    if (ui_data.state_prev_frame.active == id)
    {
        if (Input::get_mouse_button(MouseButton::LEFT))
            color = pressed_color;
    }

    render_rect(rect, z, color);

    // Return result from the previous frame
    return ui_data.state_prev_frame.interacted == id;
}

void render_text(const String text, const Font& font, const Vector2& top_left, f32 z, f32 size, const Vector4& tint)
{
    size = (size < 0.0f) ? font.size : size;

    Vector2 position = top_left;
    position.y += size * font.ascender * 0.85f;

    u64 line_start = 0;

    ImguiBatchData& batch = (font.type == Font::Type::SDF) ? ui_data.font_batch : ui_data.quad_batch;

    const Vector4 size_v4 = Vector4(size);
    for (u64 i = 0; i < text.size; i++)
    {
        const char current_char = text[i];

        switch (current_char)        
        {
            case '\n':
            {
                position.y += size * font.line_height;
                position.x = top_left.x;
                line_start = i + 1;
            } break;

            case '\r':
            {
                position.x = top_left.x;
            } break;

            case '\t':
            {
                f32 x = size * font.glyphs[0].advance;
                position.x += x * (4 - ((i - line_start) % 4));
            } break;
        }

        const Font::GlyphData& glyph = font.glyphs[current_char - ' '];
        const Vector4 position_v4 = Vector4 { position.x, position.y, position.x, position.y };

        Rect rect;
        rect.v4 = position_v4 + size_v4 * Vector4 { glyph.plane_bounds.s, -glyph.plane_bounds.v, glyph.plane_bounds.u, -glyph.plane_bounds.t };

        z += z_offset;

        if (i > 0)
        {
            s32 kerning_index   = get_kerning_index(current_char, text[i - 1]);
            const auto& kerning = find(font.kerning_table, kerning_index);
            if (kerning)
            {
                const f32 k = kerning.value();
                rect.left  += size * k;
                rect.right += size * k;
                position.x += size * k; 
            }
        }

        push_ui_quad(batch, rect, z, glyph.atlas_bounds, font.atlas, tint);

        position.x += size * glyph.advance;
    }
}

void render_char(const char ch, const Font& font, const Vector2& top_left, f32 z, f32 size, const Vector4& tint)
{
    switch (ch)
    {
        case ' ' :
        case '\n':
        case '\r':
        case '\t':
            return;
    }

    size = (size < 0.0f) ? font.size : size;

    Vector2 position = top_left;
    position.y += size * font.ascender;
    
    const Font::GlyphData& glyph = font.glyphs[ch - ' '];

    const Vector4 size_v4 = Vector4(size);
    const Vector4 position_v4 = Vector4 { position.x, position.y, position.x, position.y };
    
    Rect rect;
    rect.v4 = position_v4 + size_v4 * Vector4 { glyph.plane_bounds.s, -glyph.plane_bounds.v, glyph.plane_bounds.u, -glyph.plane_bounds.t };
    
    ImguiBatchData& batch = (font.type == Font::Type::SDF) ? ui_data.font_batch : ui_data.quad_batch;

    push_ui_quad(batch, rect, z, glyph.atlas_bounds, font.atlas, tint);
}

bool render_text_button(ID id, const Rect& rect, const String text, const Font& font, const Vector2 padding, f32 z, f32 size)
{
    const bool result = Imgui::render_button(id, rect, z);
    z += z_offset;

    const Vector2 top_left = Vector2 { rect.left + padding.x, rect.top + padding.y };
    Imgui::render_text(text, font, top_left, z, size);

    return result;
}

f32 render_slider_01(ID id, f32 value, const Rect& area, const Vector2& handle_size, f32 z, bool enabled, const Vector4& filled_color, const Vector4& disabled_color, const Vector4& bar_color, const Vector4& handle_color)
{
    Vector2 mpos  = Input::mouse_position();
    
    Rect adjusted_rect;
    adjusted_rect.v4 = ui_data.scale_v2 * area.v4 + ui_data.offset_v2;

    if (enabled)
    {
        // Calculations for the current frame
        if (mpos.x >= adjusted_rect.left && mpos.x <= adjusted_rect.right &&
            mpos.y >= adjusted_rect.top  && mpos.y <= adjusted_rect.bottom)
        {
            ui_data.state_current_frame.hot = id;

            if (Input::get_mouse_button_down(MouseButton::LEFT))
            {
                if (ui_data.state_prev_frame.active != id && ui_data.state_prev_frame.hot == id)
                    ui_data.state_current_frame.interacted = id;

                ui_data.state_current_frame.active = id;
            }
        }

        if (Input::get_mouse_button(MouseButton::LEFT) && ui_data.state_prev_frame.active == id)
        {
            value = inv_lerp(mpos.x, adjusted_rect.left, adjusted_rect.right);
            value = clamp(value, 0.0f, 1.0f);

            ui_data.state_current_frame.active = id;
        }
    }

    const f32 half_handle_width = 0.5f * handle_size.x;

    {   // Background
        Rect rect = area;
        rect.left  += half_handle_width;
        rect.right -= half_handle_width;

        render_rect(rect, z, bar_color);
        z += z_offset;
    }

    {   // Filled Background
        const f32 x_offset = (1.0f - value) * (area.right - area.left);

        Rect rect = area;
        rect.left  += half_handle_width;
        rect.right -= x_offset + half_handle_width;

        render_rect(rect, z, enabled ? filled_color : disabled_color);
        z += z_offset;
    }

    {   // Handle
        const f32 x_offset = value * (area.right - area.left);
        Rect rect = Rect { area.left + x_offset - half_handle_width, area.top, area.left + x_offset + half_handle_width, area.bottom };
        render_rect(rect, z, handle_color);

        z += z_offset;
    }

    return value;
}

f32 render_slider(ID id, f32 value, const f32 min, const f32 max, const Rect& area, const Vector2& handle_size, f32 z, bool enabled, const Vector4& filled_color, const Vector4& disabled_color, const Vector4& bar_color, const Vector4& handle_color)
{
    f32 val_01 = inv_lerp(value, min, max);
    val_01 = render_slider_01(id, val_01, area, handle_size, z, enabled, filled_color, disabled_color, bar_color, handle_color);
    return lerp(min, max, val_01);
}

} // namespace Imgui

void free(Imgui::Font& font)
{
    free(font.kerning_table);
}