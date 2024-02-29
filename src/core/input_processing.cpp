#include "input.h"
#include "input_processing.h"
#include "types.h"
#include "application/application.h"
#include "application/application_internal.h"
#include "containers/darray.h"
#include "platform/platform.h"

struct KeyboardState
{
    bool keys[(int) Key::NUM_KEYS];
};

struct MouseState
{
    s32 x, y;
    bool buttons[(int) MouseButton::NUM_BUTTONS];
    bool center_cursor = false;
};

struct InputState
{
    KeyboardState keyboard_state;
    MouseState mouse_state;
};

struct InputEvents
{
    DynamicArray<KeyDownCallback> key_down_callbacks;
    DynamicArray<MouseScrollCallback> mouse_scroll_callbacks;
};

static InputState current_input_state  = {};
static InputState previous_input_state = {};
static InputEvents input_events;

static bool had_focus = true;

static inline s32 round_to_lower_even(s32 num)
{
    return num - (num % 2);
}

inline void input_get_state(Application& app)
{
    // IDK if this is a good solution or not...
    s32 width  = round_to_lower_even(app.window.ref_width);
    s32 height = round_to_lower_even(app.window.ref_height);

    // Update current mouse position
    if (had_focus)
    {
        platform_get_mouse_position(current_input_state.mouse_state.x, current_input_state.mouse_state.y);
        
        current_input_state.mouse_state.x -= app.window.x;
        current_input_state.mouse_state.y -= app.window.y;
        current_input_state.mouse_state.x *= (f32) app.window.ref_width  / (f32) app.window.width;
        current_input_state.mouse_state.y *= (f32) app.window.ref_height / (f32) app.window.height;
    }

    if (app.window.has_focus && current_input_state.mouse_state.center_cursor)
        platform_set_mouse_position(app.window.x + width / 2, app.window.y + height / 2);
}

inline void input_state_update(Application& app)
{
    platform_copy_memory(&previous_input_state, &current_input_state, sizeof(InputState));
    
    if (current_input_state.mouse_state.center_cursor)
    {
        previous_input_state.mouse_state.x = app.window.ref_width  / 2;
        previous_input_state.mouse_state.y = app.window.ref_height / 2;
    }

    had_focus = app.window.has_focus;
}

inline void input_process_key(Key key, bool pressed)
{
    Application& app = application_get_active();

    if (pressed && !current_input_state.keyboard_state.keys[(int) key])
    {
        for (int i = 0; i < input_events.key_down_callbacks.size; i++)
            input_events.key_down_callbacks[i](app, key);
    }

    current_input_state.keyboard_state.keys[(int) key] = pressed;
}

inline void input_process_mouse_button(MouseButton btn, bool pressed)
{
    current_input_state.mouse_state.buttons[(int) btn] = pressed;
}

inline void input_process_mouse_wheel(s32 z)
{
    Application& app = application_get_active();

    for (int i = 0; i < input_events.mouse_scroll_callbacks.size; i++)
        input_events.mouse_scroll_callbacks[i](app, z);
}

// Implementations for input.h

namespace Input
{

inline bool get_key(Key key)
{
    return current_input_state.keyboard_state.keys[(int) key];
}

inline bool get_key_down(Key key)
{
    return current_input_state.keyboard_state.keys[(int) key] &&
           !previous_input_state.keyboard_state.keys[(int) key];
}

inline bool get_key_up(Key key)
{
    return !current_input_state.keyboard_state.keys[(int) key] &&
           previous_input_state.keyboard_state.keys[(int) key];
}

inline bool get_mouse_button(MouseButton button)
{
    return current_input_state.mouse_state.buttons[(int) button];
}

inline bool get_mouse_button_down(MouseButton button)
{
    return current_input_state.mouse_state.buttons[(int) button] &&
           !previous_input_state.mouse_state.buttons[(int) button];
}

inline bool get_mouse_button_up(MouseButton button)
{
    return !current_input_state.mouse_state.buttons[(int) button] &&
           previous_input_state.mouse_state.buttons[(int) button];
}

inline Vector2 mouse_position()
{
    return Vector2(
        current_input_state.mouse_state.x,
        current_input_state.mouse_state.y
    );
}

inline Vector2 mouse_delta_position()
{
    s32 del_x = current_input_state.mouse_state.x - previous_input_state.mouse_state.x;
    s32 del_y = current_input_state.mouse_state.y - previous_input_state.mouse_state.y;
    return Vector2(del_x, del_y);
}

inline void register_key_down_event_callback(KeyDownCallback callback)
{
    append(input_events.key_down_callbacks, callback);
}

inline void register_mouse_scroll_event_callback(MouseScrollCallback callback)
{
    append(input_events.mouse_scroll_callbacks, callback);
}

inline void center_mouse(bool value)
{
    current_input_state.mouse_state.center_cursor = value;
}

} // namespace Input
