#pragma once

#include "core/types.h"
#include "core/input.h"
#include "containers/string.h"

enum struct ControlScheme : u8
{
    ARROW_KEYS,
    WASD,
    
    NUM_SCHEMES
};

struct Settings
{
    ControlScheme control_scheme;
    bool dynamic_background;
    bool mute_audio;
    f32  volume;
    u32  high_score;
};

inline const String control_scheme_name(const ControlScheme scheme)
{
    constexpr char* names[(u32) ControlScheme::NUM_SCHEMES] = {
        "ARROW KEYS",
        "WASD"
    };

    return ref(names[(u32) scheme]);
}

enum struct Direction
{
    UP,
    LEFT,
    DOWN,
    RIGHT
};

inline bool get_direction_input(const Direction direction, const ControlScheme control_scheme)
{
    switch (control_scheme)
    {
        case ControlScheme::ARROW_KEYS:
        {
            constexpr Key keys[4] = { Key::UP, Key::LEFT, Key::DOWN, Key::RIGHT };
            return Input::get_key(keys[(u32) direction]);
        } break;
        
        case ControlScheme::WASD:
        {
            constexpr Key keys[4] = { Key::W, Key::A, Key::S, Key::D };
            return Input::get_key(keys[(u32) direction]);
        } break;
    }

    gn_assert_with_message(false, "Invalid control scheme! (scheme index: %)", (u32) control_scheme);
    return false;
}