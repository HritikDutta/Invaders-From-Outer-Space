#include "game_settings.h"

#include "core/types.h"
#include "serialization/json.h"

void settings_load_from_json(const Json::Document& document)
{
    const auto& j_data = document.start();

    {   // Render Scale
        const auto& j_render_scale = j_data[ref("render_scale")];
        GameSettings::render_scale.x = j_render_scale[ref("x")].float64();
        GameSettings::render_scale.y = j_render_scale[ref("y")].float64();
    }

    {   // Player settings
        GameSettings::player_move_speed    = j_data[ref("player_move_speed")].float64();
        GameSettings::player_region_height = j_data[ref("player_region_height")].float64();
        GameSettings::player_shot_delay    = j_data[ref("player_shot_delay")].float64();

        GameSettings::lazer_length   = j_data[ref("lazer_length")].int64();
        GameSettings::lazer_duration = j_data[ref("lazer_duration")].float64();
        GameSettings::lazer_speed    = j_data[ref("lazer_speed")].float64();
        GameSettings::lazer_streak_requirement = j_data[ref("lazer_streak_requirement")].float64();
        GameSettings::lazer_power_requirement  = j_data[ref("lazer_power_requirement")].float64();
        
        {   // Bullet Collider
            const auto& j_collider_size = j_data[ref("player_bullet_collider_size")];
            GameSettings::player_bullet_collider_size.x = j_collider_size[ref("x")].float64();
            GameSettings::player_bullet_collider_size.y = j_collider_size[ref("y")].float64();
        }
        
        {   // Powered Shot Explosion Collider
            const auto& j_collider_size = j_data[ref("player_powered_shot_collider_size")];
            GameSettings::player_powered_shot_collider_size.x = j_collider_size[ref("x")].float64();
            GameSettings::player_powered_shot_collider_size.y = j_collider_size[ref("y")].float64();
        }

        {   // Collider
            const auto& j_collider_size = j_data[ref("player_collider_size")];
            GameSettings::player_collider_size.x = j_collider_size[ref("x")].float64();
            GameSettings::player_collider_size.y = j_collider_size[ref("y")].float64();
        }
    }
    
    {   // Padding
        const auto& j_padding = j_data[ref("padding")];
        GameSettings::padding.x = j_padding[ref("x")].float64();
        GameSettings::padding.y = j_padding[ref("y")].float64();
    }

    {   // Enemy settings
        {   // Move Speed
            const auto& j_move_speed = j_data[ref("enemy_move_speed")];
            GameSettings::enemy_move_speed.x = j_move_speed[ref("x")].float64();
            GameSettings::enemy_move_speed.y = j_move_speed[ref("y")].float64();
        }

        {   // Wiggle Speed
            const auto& j_wiggle_speed = j_data[ref("enemy_wiggle_speed")];
            GameSettings::enemy_wiggle_speed.x = j_wiggle_speed[ref("x")].float64();
            GameSettings::enemy_wiggle_speed.y = j_wiggle_speed[ref("y")].float64();
        }

        GameSettings::enemy_move_range      = j_data[ref("enemy_move_range")].float64();
        GameSettings::enemy_rearrange_delay = j_data[ref("enemy_rearrange_delay")].float64();
        GameSettings::enemy_kamikaze_delay  = j_data[ref("enemy_kamikaze_delay")].float64();
        GameSettings::enemy_shot_delay      = j_data[ref("enemy_shot_delay")].float64();

        {   // Bullet Collider
            const auto& j_collider_size = j_data[ref("enemy_bullet_collider_size")];
            GameSettings::enemy_bullet_collider_size.x = j_collider_size[ref("x")].float64();
            GameSettings::enemy_bullet_collider_size.y = j_collider_size[ref("y")].float64();
        }

        GameSettings::enemy_row_start_count    = j_data[ref("enemy_row_start_count")].int64();
        GameSettings::enemy_column_start_count = j_data[ref("enemy_column_start_count")].int64();
        GameSettings::enemy_vertical_gap = j_data[ref("enemy_vertical_gap")].float64();

        {   // Collider
            const auto& j_collider_size = j_data[ref("enemy_collider_size")];
            GameSettings::enemy_collider_size.x = j_collider_size[ref("x")].float64();
            GameSettings::enemy_collider_size.y = j_collider_size[ref("y")].float64();
        }
        
        {   // Spawn Properties
            const auto& j_max_spawn_count = j_data[ref("enemy_start_spawn_counts")];
            GameSettings::enemy_start_spawn_counts[0] = j_max_spawn_count[ref("flying")].int64();
            GameSettings::enemy_start_spawn_counts[1] = j_max_spawn_count[ref("dropper")].int64();
            GameSettings::enemy_start_spawn_counts[2] = j_max_spawn_count[ref("kamikaze")].int64();

            const auto& j_intervals = j_data[ref("enemy_spawn_count_increase_intervals")];
            GameSettings::enemy_spawn_count_increase_intervals[0] = j_intervals[ref("flying")].int64();
            GameSettings::enemy_spawn_count_increase_intervals[1] = j_intervals[ref("dropper")].int64();
            GameSettings::enemy_spawn_count_increase_intervals[2] = j_intervals[ref("kamikaze")].int64();
            
            const auto& j_increments = j_data[ref("enemy_spawn_count_increments")];
            GameSettings::enemy_spawn_count_increments[0] = j_increments[ref("flying")].int64();
            GameSettings::enemy_spawn_count_increments[1] = j_increments[ref("dropper")].int64();
            GameSettings::enemy_spawn_count_increments[2] = j_increments[ref("kamikaze")].int64();
        }
    }

    {   // Global Settings
        GameSettings::player_bullet_speed = j_data[ref("player_bullet_speed")].float64();
        GameSettings::enemy_bullet_speed = j_data[ref("enemy_bullet_speed")].float64();
        GameSettings::pickup_drop_speed = j_data[ref("pickup_drop_speed")].float64();
    }

    {   // Pickups
        {   // Probabilties
            const auto& j_drop_chances = j_data[ref("pickup_drop_chances")];
            GameSettings::pickup_drop_chance_health     = j_drop_chances[ref("health")].float64();
            GameSettings::pickup_drop_chance_power_shot = j_drop_chances[ref("power_shot")].float64() + GameSettings::pickup_drop_chance_health;
            GameSettings::pickup_drop_chance_extra_shot = j_drop_chances[ref("extra_shot")].float64() + GameSettings::pickup_drop_chance_power_shot;
            GameSettings::pickup_drop_chance_skull      = j_drop_chances[ref("skull")].float64() + GameSettings::pickup_drop_chance_extra_shot;
        }

        {   // Collider
            const auto& j_collider_size = j_data[ref("pickup_collider_size")];
            GameSettings::pickup_collider_size.x = j_collider_size[ref("x")].float64();
            GameSettings::pickup_collider_size.y = j_collider_size[ref("y")].float64();
        }

        {   // Stats
            GameSettings::power_shot_drop_ammo = j_data[ref("power_shot_drop_ammo")].int64();
            GameSettings::power_shot_max_ammo = j_data[ref("power_shot_max_ammo")].int64();
            GameSettings::extra_shot_drop_ammo = j_data[ref("extra_shot_drop_ammo")].int64();
            GameSettings::extra_shot_max_ammo = j_data[ref("extra_shot_max_ammo")].int64();

            GameSettings::max_lazer_drops = j_data[ref("max_lazer_drops")].int64();
        }

        GameSettings::pickup_deck_size = j_data[ref("pickup_deck_size")].int64();
    }

    {   // Scoring
        GameSettings::points_per_kill = j_data[ref("points_per_kill")].int64();
        GameSettings::kill_streak_multiplier = j_data[ref("kill_streak_multiplier")].float64();
        GameSettings::max_kill_streak_multipliers = j_data[ref("max_kill_streak_multipliers")].int64();
        GameSettings::multi_kill_multiplier = j_data[ref("multi_kill_multiplier")].float64();
        GameSettings::min_kills_for_multi_kill = j_data[ref("min_kills_for_multi_kill")].float64();
        GameSettings::low_health_multiplier = j_data[ref("low_health_multiplier")].float64();
    }

    {   // Background
        GameSettings::background_star_count = j_data[ref("background_star_count")].int64();
        GameSettings::background_star_offset_multiplier = j_data[ref("background_star_offset_multiplier")].float64();
    }

    {   // UI
        GameSettings::ui_blink_delay         = j_data[ref("ui_blink_delay")].float64();
        GameSettings::ui_background_alpha    = j_data[ref("ui_background_alpha")].float64();
        GameSettings::ui_tutorial_font_scale = j_data[ref("ui_tutorial_font_scale")].float64();
        GameSettings::ui_fade_out_time       = j_data[ref("ui_fade_out_time")].float64();
    }

    {   // Effects
        GameSettings::screen_shake_amplitude_lazer = j_data[ref("screen_shake_amplitude_lazer")].float64();
        GameSettings::screen_shake_amplitude_enemy = j_data[ref("screen_shake_amplitude_enemy")].float64();
        GameSettings::screen_shake_enemy_kill_duration = j_data[ref("screen_shake_enemy_kill_duration")].float64();
    }
}