#ifndef GN_CUSTOM_MAIN

#include "core/types.h"
#include "core/logger.h"
#include "application/application.h"
#include "graphics/graphics.h"
#include "core/input_processing.h"
#include "platform/platform.h"
#include "engine/imgui.h"
#include "audio/audio.h"

extern void create_app(Application& app);

int main()
{
    Application app = {};
    create_app(app);

    if (app.window.ref_height == 0)
    {
        app.window.ref_height = app.window.height;
        app.window.ref_width  = app.window.width;
    }
    else
    {
        const f32 aspect_ratio = (f32) app.window.width / (f32) app.window.height;
        app.window.ref_width = aspect_ratio * app.window.ref_height;
    }

    application_set_active(app);

    PlatformState pstate;

    // Null icon_path means the default app icon will be used
    const char* icon_path = (app.window.icon_path.size != 0) ? app.window.icon_path.data : nullptr;
    if (!platform_window_startup(pstate,
                                 app.window.name.data,
                                 app.window.x, app.window.y,
                                 app.window.width, app.window.height,
                                 icon_path,
                                 app.window.style))
    {
        print_error("Error: Couldn't create application window!\n");
        return 1;
    }

    graphics_set_vsync(true);
    graphics_set_clear_color(app.clear_color.r, app.clear_color.g, app.clear_color.b, app.clear_color.a);

    // Initialize engine stuff

    srand((u32) platform_get_time_absolute());

    Imgui::init(app);
    Audio::init();

    // In case on_init needs time for some reason
    app.time = platform_get_time();

    app.on_init(app);

    f32 prev_time = platform_get_time();
    while (app.is_running)
    {
        app.time = platform_get_time();
        app.delta_time = min(app.time - prev_time, 0.2f);   // Max frame time is 0.2 secs
        prev_time = app.time;

        platform_pump_messages();
        graphics_clear_canvas();

        input_get_state(app);

        app.on_update(app);
        app.on_render(app);

        graphics_swap_buffers(pstate);
        input_state_update(app);

        Imgui::update();
        Audio::pool_sources();
    }

    app.on_shutdown(app);

    Audio::shutdown();
    Imgui::shutdown();

    // Shutdown engine stuff

    platform_window_shutdown(pstate);
}

#endif // GN_CUSTOM_MAIN