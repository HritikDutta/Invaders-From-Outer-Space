#include "application/application.h"
#include "audio/audio.h"
#include "core/input.h"
#include "engine/imgui.h"
#include "engine/sprite.h"
#include "engine/sprite_serialization.h"
#include "engine/imgui_serialization.h"
#include "fileio/fileio.h"
#include "game/game_state.h"
#include "serialization/json.h"

struct GameData
{
    Imgui::Font ui_font;
    GameState state;
};

void on_init(Application& app)
{
    GameData& data = *(GameData*) app.data;
    
    {   // Load Font
        String content = file_load_string(ref("assets/fonts/gamer.font.json"));

        Json::Document document = {};
        Json::parse_string(content, document);

        data.ui_font = Imgui::font_load_from_json(document, ref("assets/fonts/gamer.font.png"));

        free(document);
        free(content);
    }

    {   // Load Animations
        String json = file_load_string(ref("assets/art/Spritesheet.json"));

        Json::Document document = {};
        Json::parse_string(json, document); // Ignoring success value

        animation_load_from_json(document, data.state.anims);

        free(document);
        free(json);
    }

    {   // Load Settings
        String json = file_load_string(ref("assets/settings/game_settings.json"));

        Json::Document document = {};
        Json::parse_string(json, document); // Ignoring success value

        settings_load_from_json(document);

        free(document);
        free(json);
    }

    game_state_init(app, data.state);

    game_background_init(app, data.state);
}

void on_update(Application& app)
{
    GameData& data = *(GameData*) app.data;
    game_state_update(app, data.state);
}

void on_render(Application& app)
{
    GameData& data = *(GameData*) app.data;

    Imgui::begin();

    game_state_render(app, data.state, data.ui_font);

    #ifndef GN_RELEASE

    if (data.state.is_debug)
    {
        char buffer[256];
        sprintf(buffer, "Frame Rate: %f\nActive Bullets: %d\nActive Enemies: %d\nActive Explosions: %d\nTotal Sources: %d",
            1.0f / app.delta_time,
            (s32) data.state.player_bullets.positions.size,
            (s32) (data.state.enemies[0].positions.size + data.state.enemies[1].positions.size + data.state.enemies[2].positions.size),
            (s32) data.state.explosions.positions.size,
            Audio::get_total_source_count()
        );
        Imgui::render_text(ref(buffer), data.ui_font, Vector2 {}, 0);
    }

    #endif // GN_RELEASE

    Imgui::end();
}

void on_window_resize(Application& app)
{
    GameData& data = *(GameData*) app.data;
    game_state_window_resize(app, data.state);
}

void create_app(Application& app)
{
    app.window.x = 400;
    app.window.y = 100;

    constexpr f32 aspect_ratio = 224.0f / 256.0f;
    app.window.height = 900;
    app.window.width  = aspect_ratio * app.window.height;

    app.window.ref_height = 720;

    app.window.name = ref("Invaders from Outer Space!");
    app.window.icon_path = ref("assets/art/game_icon.ico");

    app.window.style = WindowStyle::FULLSCREEN;

    app.on_init   = on_init;
    app.on_update = on_update;
    app.on_render = on_render;
    app.on_window_resize = on_window_resize;

    app.data = platform_allocate(sizeof(GameData));
    *(GameData*) app.data = GameData {};
}