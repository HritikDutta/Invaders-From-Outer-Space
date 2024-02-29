#pragma once

#include "core/types.h"
#include "math/vecs/vector2.h"
#include "serialization/json.h"

namespace GameSettings
{
    // Rendering
    extern Vector2 render_scale;
    extern Vector2 padding;

    // Player Movement
    extern f32 player_move_speed;
    extern f32 player_region_height;

    // Player Bullets
    extern f32 player_shot_delay;
    extern Vector2 player_bullet_collider_size;
    extern Vector2 player_powered_shot_collider_size;

    extern u32 lazer_length;
    extern f32 lazer_duration;
    extern f32 lazer_speed;
    extern s32 lazer_streak_requirement;
    extern u32 lazer_power_requirement;

    // Player Physics
    extern Vector2 player_collider_size;

    // Enemy Movement
    extern Vector2 enemy_move_speed;
    extern Vector2 enemy_wiggle_speed;
    extern f32 enemy_move_range;
    extern f32 enemy_rearrange_delay;
    extern f32 enemy_kamikaze_delay;
    
    // Enemy Bullets
    extern f32 enemy_shot_delay;
    extern Vector2 enemy_bullet_collider_size;

    // Enemy Layout
    extern u64 enemy_row_start_count;
    extern u64 enemy_column_start_count;
    extern f32 enemy_vertical_gap;

    // Enemy Physics
    extern Vector2 enemy_collider_size;

    // Enemy Probabilities (Values are cumulative)
    extern f32 enemy_spawn_chances[3];
    extern s32 enemy_start_spawn_counts[3];
    extern s32 enemy_spawn_count_increase_intervals[3];
    extern s32 enemy_spawn_count_increments[3];

    // Bullet Speeds
    extern f32 player_bullet_speed;
    extern f32 enemy_bullet_speed;
    extern f32 pickup_drop_speed;

    // Pickup Physics
    extern Vector2 pickup_collider_size;

    // Pickup Probabilities (Values are cumulative)
    extern f32 pickup_drop_chance_health;
    extern f32 pickup_drop_chance_power_shot;
    extern f32 pickup_drop_chance_extra_shot;
    extern f32 pickup_drop_chance_skull;

    extern u64 pickup_deck_size;

    // Pickup Stats
    extern u32 power_shot_drop_ammo;
    extern u32 power_shot_max_ammo;
    extern u32 extra_shot_drop_ammo;
    extern u32 extra_shot_max_ammo;

    extern s32 max_lazer_drops;

    // Scoring
    extern u32 points_per_kill;
    extern f32 kill_streak_multiplier;
    extern s32 max_kill_streak_multipliers;
    extern f32 multi_kill_multiplier;
    extern u32 min_kills_for_multi_kill;
    extern f32 low_health_multiplier;

    // Background
    extern u64 background_star_count;
    extern f32 background_star_offset_multiplier;

    // UI
    extern f32 ui_blink_delay;
    extern f32 ui_background_alpha;
    extern f32 ui_tutorial_font_scale;
    extern f32 ui_fade_out_time;

    // Effects
    extern f32 screen_shake_amplitude_lazer;
    extern f32 screen_shake_amplitude_enemy;
    extern f32 screen_shake_enemy_kill_duration;
};

void settings_load_from_json(const Json::Document& document);