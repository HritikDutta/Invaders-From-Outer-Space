#include "application.h"
#include "application_internal.h"
#include "platform/platform.h"
#include "core/logger.h"

static Application* active_app = nullptr;

void application_set_active(Application& app)
{
    active_app = &app;
    app.is_running = true;
}

Application& application_get_active()
{
    gn_assert_with_message(active_app, "No active application found! Call application_set_active() before running the app.");
    return *active_app;
}

void application_show_cursor(bool value)
{
    platform_show_mouse_cursor(value);
}

void application_window_move_callback(s32 x, s32 y)
{
    gn_assert_with_message(active_app, "No active application found! Call application_set_active() before running the app.");
    
    active_app->window.x = x;
    active_app->window.y = y;
}

void application_window_resize_callback(s32 width, s32 height)
{
    gn_assert_with_message(active_app, "No active application found! Call application_set_active() before running the app.");

    active_app->window.width = width;
    active_app->window.height = height;
    active_app->window.ref_width = (f32) width / (f32) height * active_app->window.ref_height;

    active_app->on_window_resize(*active_app);
}

void application_set_window_style(Application& app, WindowStyle style)
{
    if (app.window.style == style)
        return;

    platform_set_window_style(style);
    app.window.style = style;
}