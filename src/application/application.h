#pragma once

#include "containers/string.h"
#include "containers/function.h"
#include "math/vecs/vector4.h"
#include "core/types.h"

enum struct WindowStyle
{
    WINDOWED,
    BORDERLESS,
    FULLSCREEN,
    NUM_STYLES
};

inline const String window_style_name(WindowStyle style)
{
    constexpr char* names[(u32) WindowStyle::NUM_STYLES] = {
        "Windowed",
        "Borderless",
        "Fullscreen",
    };

    return ref(names[(u32) style]);
}

struct WindowData
{
    String name;
    String icon_path;
    s32 x, y;
    s32 width, height;
    s32 ref_width, ref_height;
    bool has_focus;
    WindowStyle style;
};

struct Application
{
    WindowData window;
    void* data = nullptr;

    f32 time = 0.0f;
    f32 delta_time = 0.0f;

    Vector4 clear_color;

    bool is_running;

    Function<void(Application& app)> on_init     = [](Application&) {};
    Function<void(Application& app)> on_update   = [](Application&) {};
    Function<void(Application& app)> on_render   = [](Application&) {};
    Function<void(Application& app)> on_shutdown = [](Application&) {};
    Function<void(Application& app)> on_window_resize = [](Application&) {};
};

void application_set_active(Application& app);
Application& application_get_active();

void application_show_cursor(bool value);

void application_set_window_style(Application& app, WindowStyle style);