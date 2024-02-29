#pragma once

#include "core/types.h"
#include "math/vecs/vector2.h"
#include "serialization/json.h"

namespace GameSettings
{
    // Rendering
    Vector2 render_scale;
    Vector2 padding;

    // Player Movement
    f32 player_move_speed;
    f32 player_region_height;

    // Player Bullets
    f32 player_shot_delay;
    Vector2 player_bullet_collider_size;
    Vector2 player_powered_shot_collider_size;

    u32 lazer_length;
    f32 lazer_duration;
    f32 lazer_speed;
    s32 lazer_streak_requirement;
    u32 lazer_power_requirement;

    // Player Physics
    Vector2 player_collider_size;

    // Enemy Movement
    Vector2 enemy_move_speed;
    Vector2 enemy_wiggle_speed;
    f32 enemy_move_range;
    f32 enemy_rearrange_delay;
    f32 enemy_kamikaze_delay;
    
    // Enemy Bullets
    f32 enemy_shot_delay;
    Vector2 enemy_bullet_collider_size;

    // Enemy Layout
    u64 enemy_row_start_count;
    u64 enemy_column_start_count;
    f32 enemy_vertical_gap;

    // Enemy Physics
    Vector2 enemy_collider_size;

    // Enemy Probabilities (Values are cumulative)
    f32 enemy_spawn_chances[3];
    s32 enemy_start_spawn_counts[3];
    s32 enemy_spawn_count_increase_intervals[3];
    s32 enemy_spawn_count_increments[3];

    // Bullet Speeds
    f32 player_bullet_speed;
    f32 enemy_bullet_speed;
    f32 pickup_drop_speed;

    // Pickup Physics
    Vector2 pickup_collider_size;

    // Pickup Probabilities (Values are cumulative)
    f32 pickup_drop_chance_health;
    f32 pickup_drop_chance_power_shot;
    f32 pickup_drop_chance_extra_shot;
    f32 pickup_drop_chance_skull;

    u64 pickup_deck_size;

    // Pickup Stats
    u32 power_shot_drop_ammo;
    u32 power_shot_max_ammo;
    u32 extra_shot_drop_ammo;
    u32 extra_shot_max_ammo;

    s32 max_lazer_drops;

    // Scoring
    u32 points_per_kill;
    f32 kill_streak_multiplier;
    s32 max_kill_streak_multipliers;
    f32 multi_kill_multiplier;
    u32 min_kills_for_multi_kill;
    f32 low_health_multiplier;

    // Background
    u64 background_star_count;
    f32 background_star_offset_multiplier;

    // UI
    f32 ui_blink_delay;
    f32 ui_background_alpha;
    f32 ui_tutorial_font_scale;
    f32 ui_fade_out_time;

    // Effects
    f32 screen_shake_amplitude_lazer;
    f32 screen_shake_amplitude_enemy;
    f32 screen_shake_enemy_kill_duration;
};