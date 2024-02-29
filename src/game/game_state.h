#pragma once

#include "application/application.h"
#include "core/coroutines.h"
#include "engine/imgui.h"
#include "engine/sprite.h"
#include "math/vecs/vector2.h"
#include "player_settings.h"
#include "game_settings.h"

struct AnimationData
{
    u64 animation_index;
    Animation2D::Instance instance;
};

struct EntityData
{
    DynamicArray<Vector2> positions;
    DynamicArray<AnimationData> animations;
};

// All enum values correspond to the index of their corresponding animation
// This means the animation index can be used to determine the enum value

enum struct BulletType
{
    STANDARD      = 5,
    POWER_SHOT    = 6,
    LAZER         = 7
};

enum struct PickupType
{
    HEALTH       = 18,
    POWER_SHOT   = 19,
    EXTRA_SHOT   = 20,
    LAZER_CHARGE = 21,
    SKULL        = 22
};

enum struct PlayerState
{
    NORMAL  = 0,
    HURT    = 1,
    CHARGED = 2,
    LAZER_WIND_UP = 3,
    LAZER_SHOOT   = 4,
};

enum struct EnemyType
{
    FLYING,
    DROPPER,
    KAMIKAZE,

    NUM_TYPES
};

namespace GameScreen
{
    static constexpr u32 GAME          = (1 << 0);
    static constexpr u32 GAME_OVER     = (1 << 1);
    static constexpr u32 MAIN_MENU     = (1 << 2);
    static constexpr u32 PAUSE_MENU    = (1 << 3);
    static constexpr u32 SETTINGS_MENU = (1 << 4);
    static constexpr u32 HIGH_SCORE    = (1 << 5);
}

struct StageSettings
{
    u32 number;

    u32 enemy_column_count;
    u32 enemy_row_count;

    Vector2 enemy_move_speed;

    f32 enemy_rearrange_delay;
    f32 enemy_kamikaze_delay;
    f32 enemy_shot_delay;

    s32 enemy_spawn_counts[3];
};

struct GameState
{
    Coroutine state_co;

    DynamicArray<Animation2D> anims;

    s32 player_lives;
    s32 player_kill_streak;
    u32 player_score;

    Vector2 player_position;
    Vector2 player_size;
    f32 player_time_since_last_shot;
    u64 player_previous_animation_index;
    AnimationData player_animation;

    u32 player_bullets_per_shot;
    BulletType player_equipped_bullet_type;

    s32 player_power_shot_ammo;
    s32 player_extra_shot_ammo;
    s32 lazer_drops;
    
    f32 enemy_time_since_last_shot;
    f32 enemy_time_since_last_kamikaze;
    f32 enemy_time_since_last_rearrangement;

    Coroutine stage_co;
    StageSettings current_stage;

    DynamicArray<Vector2> enemy_slots[(u64) EnemyType::NUM_TYPES];
    DynamicArray<Vector2> empty_slots;
    f32 time_since_screen_shake_start;

    EntityData player_bullets;
    EntityData enemy_bullets;
    EntityData enemies[(u64) EnemyType::NUM_TYPES];
    EntityData explosions;
    EntityData power_shot_explosions;
    EntityData pickups;
    EntityData kamikaze_enemies;

    DynamicArray<Vector2> kamikaze_targets;

    DynamicArray<Vector3> star_positions;
    DynamicArray<u64> star_sprite_indices;

    AnimationData lazer_chunk;
    u32 lazer_charge;
    Vector2 lazer_position;
    u32 lazer_start;
    u32 lazer_end;
    Coroutine lazer_co;
    bool is_lazer_active;

    Rect    game_rect;
    Vector2 game_playground;

    u32 current_screen;
    Settings player_settings;
    bool new_high_score;
    bool is_debug;
};

void game_background_init(Application& app, GameState& state);

void game_state_init(Application& app, GameState& state);
void game_state_update(Application& app, GameState& state);
void game_state_render(Application& app, GameState& state, const Imgui::Font& font);

void game_state_window_resize(const Application& app, GameState& state);