#include "game_state.h"

#include "application/application.h"
#include "audio/audio.h"
#include "core/coroutines.h"
#include "core/input.h"
#include "core/utils.h"
#include "containers/bytes.h"
#include "containers/darray.h"
#include "fileio/fileio.h"
#include "engine/imgui.h"
#include "engine/sprite.h"
#include "math/math.h"
#include "serialization/json.h"
#include "player_settings.h"
#include "game_settings.h"
#include "game_serialization.h"

#include <xmmintrin.h>

constexpr u64 bullet_enemy_animation_index = 8;
constexpr u64 player_explosion_animation_index = 9;
constexpr u64 enemy_explosion_animation_index = 10;
constexpr u64 power_shot_explosion_animation_index = 23;
constexpr u64 stars_animation_index = 24;

constexpr f32 min_volume = 0.0f;
constexpr f32 max_volume = 100.0f;

static const Vector4 heading_color = Vector4 { 1.0f, 0.0f, 0.0f, 1.0f };
static const Vector4 high_score_color = Vector4 { 1.0f, 1.0f, 0.0f, 1.0f };

static Audio::Sound sound_bullet;
static Audio::Sound sound_enemy_bullet;
static Audio::Sound sound_explosion;
static Audio::Sound sound_player_hurt;
static Audio::Sound sound_player_lost;
static Audio::Sound sound_pickup_good;
static Audio::Sound sound_pickup_bad;
static Audio::Sound sound_lazer_charged;
static Audio::Sound sound_lazer_wind_up;
static Audio::Sound sound_lazer_shoot;
static Audio::Sound sound_main_menu;
static Audio::Sound sound_button_press;
static Audio::Sound sound_kamikaze;

static Audio::Source source_lazer_charged;
static Audio::Source source_lazer;
static Audio::Source source_main_menu;

static DynamicArray<u64> enemy_list = {};           // For spawning enemies
static DynamicArray<PickupType> pickup_deck = {};   // For spawning pickups
static u64 pickup_deck_index = 0;

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
static void entity_init(EntityData& entities)
{
    entities.animations = make<DynamicArray<AnimationData>>();
    entities.positions  = make<DynamicArray<Vector2>>();
}

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
static void entity_add(EntityData& entities, const Vector2 position, u64 animation_index, f32 time)
{
    append(entities.positions, position);
    
    Animation2D::Instance instance;
    animation_start_instance(instance, time);

    append(entities.animations, AnimationData { animation_index, instance});
}

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
static void entity_remove(EntityData& entities, u64 index)
{
    remove_swap(entities.animations, index);
    remove_swap(entities.positions, index);
}

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
static void entity_clear(EntityData& entities)
{
    clear(entities.animations);
    clear(entities.positions);
}

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
static void entity_animation_step(const DynamicArray<Animation2D>& anims, EntityData& entities, f32 time)
{
    for (u64 i = 0; i < entities.animations.size; i++)
    {
        Animation2D::Instance& instance = entities.animations[i].instance;
        animation_step_instance(anims[entities.animations[i].animation_index], instance, time);
    }
}

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
static void entity_render(const GameState& state, const EntityData& entities, f32& z)
{
    constexpr f32 z_offset = -0.001f;

    for (u64 i = 0; i < entities.positions.size; i++)
    {
        const AnimationData& anim_data = entities.animations[i];

        const Sprite& sprite = state.anims[anim_data.animation_index].sprites[anim_data.instance.current_frame_index];
        Imgui::render_sprite(sprite, entities.positions[i], z, GameSettings::render_scale);

        z += z_offset;
    }
}

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
static void stage_init(Coroutine& co, StageSettings& stage)
{
    coroutine_start(co);

    stage.number = 1;
    stage.enemy_column_count = GameSettings::enemy_column_start_count;
    stage.enemy_row_count = GameSettings::enemy_row_start_count;

    stage.enemy_move_speed = GameSettings::enemy_move_speed;

    stage.enemy_rearrange_delay = GameSettings::enemy_rearrange_delay;
    stage.enemy_kamikaze_delay = GameSettings::enemy_kamikaze_delay;
    stage.enemy_shot_delay = GameSettings::enemy_shot_delay;

    stage.enemy_spawn_counts[0] = GameSettings::enemy_start_spawn_counts[0];
    stage.enemy_spawn_counts[1] = GameSettings::enemy_start_spawn_counts[1];
    stage.enemy_spawn_counts[2] = GameSettings::enemy_start_spawn_counts[2];

    coroutine_yield(co);

    while (true)
    {
        // Increase Rows
        stage.enemy_row_count = min(stage.enemy_row_count + 1, 7u);
        stage.number++;
        coroutine_yield(co);
        
        // Reduce Shot Delay
        stage.enemy_shot_delay = max(stage.enemy_shot_delay - 0.05f, GameSettings::player_shot_delay - 0.15f);
        stage.number++;
        coroutine_yield(co);

        // Reduce Kamikaze Delay
        stage.enemy_kamikaze_delay = max(stage.enemy_kamikaze_delay - 0.05f, 0.25f);
        stage.number++;
        coroutine_yield(co);
        
        // Increase Columns
        stage.enemy_column_count = min(stage.enemy_column_count + 1, 7u);
        stage.number++;
        coroutine_yield(co);
        
        // Increase Move speed and Reduce Rearrange Delay
        stage.enemy_move_speed += Vector2 { 0.15f, 0.25f };
        stage.enemy_rearrange_delay = max(stage.enemy_rearrange_delay - 0.15f, 0.25f);
        stage.number++;
        coroutine_yield(co);
    }

    coroutine_end(co);

    stage.enemy_spawn_counts[0] += GameSettings::enemy_spawn_count_increments[0] * (((stage.number) % GameSettings::enemy_spawn_count_increase_intervals[0]) == 0);
    stage.enemy_spawn_counts[1] += GameSettings::enemy_spawn_count_increments[1] * (((stage.number) % GameSettings::enemy_spawn_count_increase_intervals[1]) == 0);
    stage.enemy_spawn_counts[2] += GameSettings::enemy_spawn_count_increments[2] * (((stage.number) % GameSettings::enemy_spawn_count_increase_intervals[2]) == 0);
}

static inline void fill_pickup_deck(DynamicArray<PickupType>& pickup_deck, GameState& state)
{
    const u64 deck_size = GameSettings::pickup_deck_size;

    clear(pickup_deck);
    resize(pickup_deck, deck_size);

    const u64 health_end     = deck_size * GameSettings::pickup_drop_chance_health;
    const u64 power_shot_end = deck_size * GameSettings::pickup_drop_chance_power_shot;
    const u64 extra_shot_end = deck_size * GameSettings::pickup_drop_chance_extra_shot;
    const u64 skull_end      = deck_size; // Rest all are skulls

    u64 filled = 0;

    for (; filled < health_end; filled++)
        append(pickup_deck, PickupType::HEALTH);
        
    for (; filled < power_shot_end; filled++)
        append(pickup_deck, PickupType::POWER_SHOT);
        
    for (; filled < extra_shot_end; filled++)
        append(pickup_deck, PickupType::EXTRA_SHOT);
        
    for (; filled < skull_end; filled++)
        append(pickup_deck, PickupType::SKULL);
}

static inline EnemyType get_enemy_type_from_animation_index(u64 index)
{
    switch (index)
    {
        case 11: case 12:
            return EnemyType::FLYING;
            
        case 13: case 14:
            return EnemyType::KAMIKAZE;
            
        case 15: case 16: case 17:
            return EnemyType::DROPPER;
    }

    gn_assert_with_message(false, "Animation index doesn't correspond to any valid enemy! (index: %)", index);
    return EnemyType::NUM_TYPES;
}

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
static u64 get_random_enemy_index_of_type(EnemyType type)
{
    const f32 num = Math::random();

    switch (type)
    {
        case EnemyType::FLYING:
        {
            constexpr u64 start = 11;
            constexpr u64 end   = 12;
            return Math::floor((end - start) * num + 0.5f) + start;
        }

        case EnemyType::KAMIKAZE:
        {
            constexpr u64 start = 13;
            constexpr u64 end   = 14;
            return Math::floor((end - start) * num + 0.5f) + start;
        }

        case EnemyType::DROPPER:
        {
            constexpr u64 start = 15;
            constexpr u64 end   = 17;
            return Math::floor((end - start) * num + 0.5f) + start;
        }
    }
 
    gn_assert_with_message(false, "Unsupported enemy type! (type index: %)", (u64) type);
    return 0;
}

static inline void fill_enemy_list(DynamicArray<u64>& enemy_list, const GameState& state, u64 num_enemies)
{
    clear(enemy_list);
    resize(enemy_list, num_enemies);

    const StageSettings& stage = state.current_stage;
    
    {   // Fill Enemy List
        const u64 kamikaze_end = stage.enemy_spawn_counts[(u64) EnemyType::KAMIKAZE];
        const u64 dropper_end  = stage.enemy_spawn_counts[(u64) EnemyType::DROPPER] + kamikaze_end;
        const u64 flying_end   = num_enemies;

        u64 filled = 0;

        // Fill Kamikaze
        for (; filled < kamikaze_end; filled++)
            append(enemy_list, get_random_enemy_index_of_type(EnemyType::KAMIKAZE));
        
        // Fill Droppers
        for (; filled < dropper_end; filled++)
            append(enemy_list, get_random_enemy_index_of_type(EnemyType::DROPPER));
        
        // Fill Flying
        for (; filled < flying_end; filled++)
            append(enemy_list, get_random_enemy_index_of_type(EnemyType::FLYING));
    }

    shuffle(enemy_list);
}

static void init_enemies(Application& app, GameState& state)
{
    gn_assert_with_message(state.enemies[0].positions.size == 0, "Not all enemies were killed before initializing next wave! (enemies left: %)", state.enemies[0].positions.size);
    gn_assert_with_message(state.enemies[1].positions.size == 0, "Not all enemies were killed before initializing next wave! (enemies left: %)", state.enemies[1].positions.size);
    gn_assert_with_message(state.enemies[2].positions.size == 0, "Not all enemies were killed before initializing next wave! (enemies left: %)", state.enemies[2].positions.size);

    constexpr u64 enemy_animation_start_index = 11;
    constexpr u64 enemy_options = 7;

    const StageSettings& stage = state.current_stage;

    const f32 x_offset = min(state.game_playground.x / stage.enemy_column_count, 72.0f);
    const f32 x_start  = 0.5f * (state.game_playground.x - (stage.enemy_column_count - 1) * x_offset);
    Vector2 position = Vector2 { x_start, 75.0f };

    const f32 start_height = -0.5f * GameSettings::enemy_move_speed.y;

    clear(state.enemy_slots[0]);
    clear(state.enemy_slots[1]);
    clear(state.enemy_slots[2]);

    const u64 num_enemies = stage.enemy_row_count * stage.enemy_column_count;
    fill_enemy_list(enemy_list, state, num_enemies);

    for (u64 y = 0; y < stage.enemy_row_count; y++)
    {
        for (u64 x = 0; x < stage.enemy_column_count; x++)
        {
            const u64 index = y * stage.enemy_column_count + x;

            const u64 random_enemy = enemy_list[index];
            const u64 enemy_type_index = (u64) get_enemy_type_from_animation_index(random_enemy);

            append(state.enemy_slots[enemy_type_index], position);

            const f32 x_mult = 2.0f * Math::random() - 1.0f;
            entity_add(state.enemies[enemy_type_index], Vector2 { position.x, start_height + position.y }, random_enemy, app.time);
            position.x += x_offset;
        }

        position.y += GameSettings::enemy_vertical_gap;
        position.x = x_start;
    }

    clear(state.empty_slots);

    state.enemy_time_since_last_kamikaze = state.enemy_time_since_last_rearrangement = state.enemy_time_since_last_shot = 0.0f;
}

void game_background_init(Application& app, GameState& state)
{
    state.star_positions      = make<DynamicArray<Vector3>>(GameSettings::background_star_count);
    state.star_sprite_indices = make<DynamicArray<u64>>(GameSettings::background_star_count);

    for (s32 i = 0; i < GameSettings::background_star_count; i++)
    {
        Vector3 position = Vector3 {
            Math::random(),
            Math::random(),
            Math::random()
        };

        append(state.star_positions, position);

        u64 sprite_index = state.anims[stars_animation_index].sprites.size * Math::random();
        append(state.star_sprite_indices, sprite_index);
    }
}

void game_state_reset(Application& app, GameState& state)
{
    {   // Stage
        coroutine_reset(state.stage_co);
        stage_init(state.stage_co, state.current_stage);
    }

    {   // Initialize Player Data
        state.player_position = Vector2 { state.game_playground.x / 2.0f, state.game_playground.y - (GameSettings::player_region_height / 2.0f) };
        state.player_size = state.anims[(u64) PlayerState::NORMAL].sprites[0].size;

        state.player_animation.animation_index = (u64) PlayerState::NORMAL;
        animation_start_instance(state.player_animation.instance, app.time);

        state.player_lives = 3;
        state.player_score = state.player_kill_streak = 0;
        state.is_lazer_active = false;

        state.player_equipped_bullet_type = BulletType::STANDARD;
        state.player_bullets_per_shot = 1;
        state.lazer_drops = state.player_extra_shot_ammo = state.player_power_shot_ammo  = 0;
    }

    {   // Clear all entities
        entity_clear(state.player_bullets);
        entity_clear(state.enemy_bullets);
        entity_clear(state.explosions);
        entity_clear(state.power_shot_explosions);
        entity_clear(state.pickups);
        entity_clear(state.kamikaze_enemies);
        
        entity_clear(state.enemies[0]);
        entity_clear(state.enemies[1]);
        entity_clear(state.enemies[2]);
    }

    {   // Timers
        state.player_time_since_last_shot = Math::infinity;
        state.time_since_screen_shake_start = Math::infinity;
        state.lazer_charge = 0;
    }

    {   // Initialize Stuff
        init_enemies(app, state);
    }

    coroutine_reset(state.state_co);

    state.new_high_score = false;
}

static void button_callback_play_sound(Imgui::ID id)
{
    Audio::play_sound(sound_button_press, false);
}

void game_state_init(Application& app, GameState& state)
{
    game_state_window_resize(app, state);
    state.game_playground = Vector2 { state.game_rect.right - state.game_rect.left, state.game_rect.bottom - state.game_rect.top };

    {   // Initialize Bullets
        entity_init(state.player_bullets);
        entity_init(state.enemy_bullets);

        state.lazer_chunk.animation_index = (u64) BulletType::LAZER;
    }

    {   // Intialize Enemies
        entity_init(state.enemies[0]);
        entity_init(state.enemies[1]);
        entity_init(state.enemies[2]);
        entity_init(state.kamikaze_enemies);
        state.enemy_slots[0] = make<DynamicArray<Vector2>>();
        state.enemy_slots[1] = make<DynamicArray<Vector2>>();
        state.enemy_slots[2] = make<DynamicArray<Vector2>>();
        state.kamikaze_targets = make<DynamicArray<Vector2>>();
    }

    {   // Initialize Explosions
        entity_init(state.explosions);
        entity_init(state.power_shot_explosions);
    }

    {   // Initialize Pickups
        entity_init(state.pickups);
        fill_pickup_deck(pickup_deck, state);
        shuffle(pickup_deck);
        pickup_deck_index = 0;
    }

    {   // Load sounds
        {   // Shoot
            Bytes bytes = file_load_bytes(ref("assets/audio/mixed/player_bullet.wav"));
            Audio::load_from_bytes(bytes, sound_bullet);
            free(bytes);
        }

        {   // Enemy Bullet
            Bytes bytes = file_load_bytes(ref("assets/audio/mixed/enemy_bullet.wav"));
            Audio::load_from_bytes(bytes, sound_enemy_bullet);
            free(bytes);
        }

        {   // Explosion
            Bytes bytes = file_load_bytes(ref("assets/audio/mixed/explosion.wav"));
            Audio::load_from_bytes(bytes, sound_explosion);
            free(bytes);
        }

        {   // Player Hurt
            Bytes bytes = file_load_bytes(ref("assets/audio/mixed/player_hurt.wav"));
            Audio::load_from_bytes(bytes, sound_player_hurt);
            free(bytes);
        }

        {   // Player Lost
            Bytes bytes = file_load_bytes(ref("assets/audio/mixed/you_died.wav"));
            Audio::load_from_bytes(bytes, sound_player_lost);
            free(bytes);
        }

        {   // Pickup Good
            Bytes bytes = file_load_bytes(ref("assets/audio/mixed/good_pickup.wav"));
            Audio::load_from_bytes(bytes, sound_pickup_good);
            free(bytes);
        }

        {   // Pickup Bad
            Bytes bytes = file_load_bytes(ref("assets/audio/mixed/bad_pickup.wav"));
            Audio::load_from_bytes(bytes, sound_pickup_bad);
            free(bytes);
        }

        {   // Lazer Wind Up
            Bytes bytes = file_load_bytes(ref("assets/audio/mixed/lazer_wind_up.wav"));
            Audio::load_from_bytes(bytes, sound_lazer_wind_up);
            free(bytes);
        }
        
        {   // Lazer Charged
            Bytes bytes = file_load_bytes(ref("assets/audio/mixed/lazer_charged.wav"));
            Audio::load_from_bytes(bytes, sound_lazer_charged);
            free(bytes);

            source_lazer_charged = Audio::source_create(sound_lazer_charged.fmt);
        }
        
        {   // Lazer Shoot
            Bytes bytes = file_load_bytes(ref("assets/audio/mixed/lazer_shoot.wav"));
            Audio::load_from_bytes(bytes, sound_lazer_shoot);
            free(bytes);

            source_lazer = Audio::source_create(sound_lazer_shoot.fmt);
        }
        
        {   // Main Menu
            Bytes bytes = file_load_bytes(ref("assets/audio/mixed/main_menu.wav"));
            Audio::load_from_bytes(bytes, sound_main_menu);
            free(bytes);

            source_main_menu = Audio::source_create(sound_main_menu.fmt);
        }
        
        {   // Button Press
            Bytes bytes = file_load_bytes(ref("assets/audio/mixed/button_press.wav"));
            Audio::load_from_bytes(bytes, sound_button_press);
            free(bytes);

            Imgui::register_button_callback(button_callback_play_sound);
        }
        
        {   // Kamikaze Start
            Bytes bytes = file_load_bytes(ref("assets/audio/mixed/kamikaze.wav"));
            Audio::load_from_bytes(bytes, sound_kamikaze);
            free(bytes);
        }
    }

    game_state_reset(app, state);

    state.current_screen = GameScreen::MAIN_MENU;
    state.is_debug = false;

    {   // Load Settings
        String json = file_load_string(ref(settings_file_name, settings_file_name_size));
        
        Json::Document document = {};
        bool success = Json::parse_string(json, document);
        gn_assert_with_message(success, "Error parsing document");

        WindowStyle window_style;
        load_settings_from_json(document, state.player_settings, window_style);
        
        {   // Apply Settings
            const f32 volume = state.player_settings.mute_audio ? 0.0f : inv_lerp(state.player_settings.volume, min_volume, max_volume);
            Audio::set_master_volume(volume);
            application_set_window_style(app, window_style);
        }

        free(document);
        free(json);
    }
}

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
static bool test_aabb_vs_aabb(const Vector4& aabb1, const Vector4& aabb2)
{
    const Vector4 res = (aabb2 - aabb1) * Vector4 { 1, 1, -1, -1 };
    s32 mask = _mm_movemask_ps(_mm_cmpge_ps(res._sse, _mm_setzero_ps()));
    return (mask == 0xF);
}

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
static PickupType get_random_pickup_type(GameState& state)
{
    if (pickup_deck_index >= pickup_deck.size)
    {
        pickup_deck_index = 0;
        shuffle(pickup_deck);
    }
    
    return pickup_deck[pickup_deck_index++];
}

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
static void spawn_explosion(GameState& state, Vector2 position, f32 time)
{
    entity_add(state.explosions, position, enemy_explosion_animation_index, time);
    Audio::play_sound(sound_explosion, false);

    // Screen Shake
    state.time_since_screen_shake_start = 0.0f;
}

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
static void remove_enemy(GameState& state, u64 type_index, u64 index, f32 time)
{
    EntityData& enemies = state.enemies[type_index];
    const Vector2 enemy_position = enemies.positions[index];

    // Remove Enemy
    entity_remove(enemies, index);
    const Vector2 slot_position = remove_swap(state.enemy_slots[type_index], index);

    // Don't rearrange immediately
    if (state.empty_slots.size == 0)
        state.enemy_time_since_last_rearrangement = 0.0f;

    append(state.empty_slots, slot_position);

    spawn_explosion(state, enemy_position, time);
}

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
static void spawn_pickup(GameState& state, Vector2 position, f32 time)
{
    PickupType pickup_type = get_random_pickup_type(state);
    entity_add(state.pickups, position, (u64) pickup_type, time);
}

static void update_lazer(GameState& state, f32 time)
{
    f32& last_update_time = coroutine_stack_variable<f32>(state.lazer_co);
    s32 x;

    coroutine_start(state.lazer_co);

    if (state.player_lives > 0)
    {
        state.player_animation.animation_index = (u64) PlayerState::LAZER_WIND_UP;
        animation_start_instance(state.player_animation.instance, time);
    }

    Audio::play_sound(sound_lazer_wind_up, false);

    coroutine_wait_until(state.lazer_co, state.player_animation.instance.loop_count > 0);
    
    if (state.player_lives > 0)
    {
        Audio::source_stop(source_lazer_charged);
        state.player_animation.animation_index = (u64) PlayerState::LAZER_SHOOT;
        animation_start_instance(state.player_animation.instance, time);
    }

    Audio::play_buffer(source_lazer, sound_lazer_shoot.buffer, true, false);
    
    last_update_time = time;
    while (state.lazer_end < GameSettings::lazer_length)
    {
        x = Math::floor(GameSettings::lazer_speed * (time - last_update_time));
        if (x > 0)
        {
            state.lazer_end = min(state.lazer_end + x, GameSettings::lazer_length);
            last_update_time = time;
        }

        coroutine_yield(state.lazer_co);
    }

    coroutine_wait_seconds(state.lazer_co, GameSettings::lazer_duration);
    
    Audio::source_stop(source_lazer);

    if (state.player_lives > 0)
    {
        state.player_animation.animation_index = (u64) PlayerState::NORMAL;
        animation_start_instance(state.player_animation.instance, time);
    }
    
    last_update_time = time;
    while (state.lazer_start < state.lazer_end)
    {
        x = Math::floor(GameSettings::lazer_speed * (time - last_update_time));
        if (x > 0)
        {
            state.lazer_start = min(state.lazer_start + x, state.lazer_end);
            last_update_time = time;
        }
        coroutine_yield(state.lazer_co);
    }

    state.is_lazer_active = false;
    state.lazer_charge = 0;

    coroutine_end(state.lazer_co);
}

static void damage_player(GameState& state, f32 time)
{
    state.player_kill_streak = 0;
    state.player_lives--;

    if (state.player_animation.animation_index != (u64) PlayerState::HURT &&
        state.player_animation.animation_index != player_explosion_animation_index) 
        state.player_previous_animation_index = state.player_animation.animation_index;

    state.player_animation.animation_index = (state.player_lives > 0) ? (u64) PlayerState::HURT : player_explosion_animation_index;
    animation_start_instance(state.player_animation.instance, time);

    Audio::play_sound(sound_player_hurt, false);

    if (state.player_lives <= 0)
    {
        Audio::source_stop(source_lazer_charged);
        Audio::play_sound(sound_player_lost, false);
    }
}

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
static void add_score(GameState& state, u32 kill_count)
{
    if (state.player_lives <= 0)
        return;

    s32 kill_streak_start = state.player_kill_streak - kill_count;

    for (u32 i = 0; i < kill_count; i++)
    {
        // Kill streak multiplier
        const s32 kill_streaks = min((s32) Math::floor((kill_streak_start + i) / GameSettings::lazer_streak_requirement), GameSettings::max_kill_streak_multipliers);
        f32 multiplier = Math::pow(GameSettings::kill_streak_multiplier, kill_streaks);

        // TODO: Give visual and audio feedback for multi kill
        multiplier *= (kill_count >= GameSettings::min_kills_for_multi_kill) ? GameSettings::multi_kill_multiplier : 1.0f;
        multiplier *= (state.player_lives == 1) ? GameSettings::low_health_multiplier : 1.0f;

        state.player_score += multiplier * GameSettings::points_per_kill;
    }
}

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
static Vector2 move_towards(const Vector2 source, const Vector2 destination, const Vector2 speed, f32 delta_time)
{
    const Vector2 direction = destination - source;
    const f32 len = length(direction);

    if (len == 0.0f)
        return source;

    const Vector2 modified_speed = lerp(0.0f, speed, clamp(len / 100.0f, 0.0f, 1.0f));
    return source + delta_time * modified_speed * (direction / len);
}

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
static void screen_clear_and_switch_to(GameState& state, u64 screen)
{
    state.current_screen = screen;
}

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
static void screen_switch_to(GameState& state, u64 screen)
{
    state.current_screen |= screen;
}

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
static void screen_switch_off(GameState& state, u64 screen)
{
    state.current_screen &= ~screen;
}

static void internal_state_update_gameplay(Application& app, GameState& state)
{
    if (!(state.current_screen & ~(GameScreen::GAME | GameScreen::GAME_OVER)))
        state.time_since_screen_shake_start += app.delta_time;

    constexpr f32 bullet_spawn_offset = 10.0f;

    if (state.player_lives > 0)
    {
        {   // Player Movement
            const ControlScheme scheme = state.player_settings.control_scheme;
            Vector2 input = Vector2 {
                (f32) get_direction_input(Direction::RIGHT, scheme) - (f32) get_direction_input(Direction::LEFT, scheme),
                (f32) get_direction_input(Direction::DOWN, scheme)  - (f32) get_direction_input(Direction::UP, scheme),
            };

            state.player_position += app.delta_time * GameSettings::player_move_speed * normalize(input);

            const Vector2 player_half_size = 0.5f * GameSettings::render_scale * state.player_size;
            state.player_position.x = clamp(state.player_position.x, player_half_size.x, state.game_playground.x - player_half_size.x);
            state.player_position.y = clamp(state.player_position.y, state.game_playground.y - GameSettings::player_region_height + player_half_size.y, state.game_playground.y - player_half_size.y);
        }
        
        // Player Shooting
        if (Input::get_key(Key::Z) && !state.is_lazer_active && state.lazer_charge >= GameSettings::lazer_power_requirement)
        {
            animation_start_instance(state.lazer_chunk.instance, app.time);
            coroutine_reset(state.lazer_co);

            state.lazer_start = state.lazer_end = 0;
            state.is_lazer_active = true;
        }

        if (!state.is_lazer_active)
        {
            state.player_time_since_last_shot += app.delta_time;

            constexpr f32 x_offsets[][3] = { {}, { -15.0f, 15.0f }, { -20.0f, 0.0f, 20.0f } };
            if (Input::get_key(Key::SPACE) && state.player_time_since_last_shot >= GameSettings::player_shot_delay)
            {
                for (u32 i = 0; i < state.player_bullets_per_shot; i++)
                {
                    Vector2 bullet_position = state.player_position + Vector2 { 0.0f, -GameSettings::render_scale.y * bullet_spawn_offset };
                    bullet_position.x += x_offsets[state.player_bullets_per_shot - 1][i];
                    entity_add(state.player_bullets, bullet_position, (u64) state.player_equipped_bullet_type, app.time);
                }
                
                if (state.player_equipped_bullet_type == BulletType::POWER_SHOT)
                {
                    state.player_power_shot_ammo--;

                    if (state.player_power_shot_ammo <= 0)
                        state.player_equipped_bullet_type = BulletType::STANDARD;
                }

                if (state.player_bullets_per_shot > 1)
                {
                    state.player_extra_shot_ammo--;

                    if (state.player_extra_shot_ammo <= 0)
                        state.player_bullets_per_shot = 1;
                }

                Audio::play_sound(sound_bullet, false);
                state.player_time_since_last_shot = 0;
            }
        }
    }

    if (!(state.current_screen & GameScreen::MAIN_MENU))
    {
        {   // Enemy Rearrangement  (Only Dropper can rearrange)
            constexpr u64 type_index = (u64) EnemyType::DROPPER;
            const EntityData& enemies = state.enemies[type_index];

            state.enemy_time_since_last_rearrangement += app.delta_time;

            if (state.player_lives > 0 &&
                state.empty_slots.size > 0 && enemies.positions.size > 0 &&
                state.enemy_time_since_last_rearrangement >= state.current_stage.enemy_rearrange_delay)
            {
                u64 enemy_index = Math::random() * enemies.positions.size;

                Vector2 old_slot = state.enemy_slots[type_index][enemy_index];
                state.enemy_slots[type_index][enemy_index] = remove(state.empty_slots, 0);
                append(state.empty_slots, old_slot);

                state.enemy_time_since_last_rearrangement = 0.0f;
            }
        }

        {   // Enemy Movement
            const f32 x_offset = GameSettings::enemy_move_range * Math::sin(app.time * GameSettings::enemy_wiggle_speed.x);
            const f32 y_offset = GameSettings::enemy_move_range * Math::cos(app.time * GameSettings::enemy_wiggle_speed.y);

            for (u64 enemy_type = 0; enemy_type < (u64) EnemyType::NUM_TYPES; enemy_type++)
            {
                EntityData& enemies = state.enemies[enemy_type];

                for (u64 i = 0; i < enemies.positions.size; i++)
                {
                    constexpr f32 range = 100.0f;
                    const f32 y_gitter = (range * Math::random() - (range / 2.0f));

                    const Vector2 destination = Vector2 { state.enemy_slots[enemy_type][i].x + x_offset, state.enemy_slots[enemy_type][i].y + y_offset + y_gitter };
                    enemies.positions[i] = move_towards(enemies.positions[i], destination, GameSettings::enemy_move_speed, app.delta_time);
                }
            }
        }

        {   // Kamikaze Enemy Movement
            for (u64 i = 0; i < state.kamikaze_enemies.positions.size; i++)
            {
                if (state.kamikaze_enemies.positions[i].y <= state.game_playground.y - GameSettings::player_region_height)
                    state.kamikaze_targets[i] = state.player_position;
                else
                {
                    const Vector2 direction = state.kamikaze_targets[i] - state.kamikaze_enemies.positions[i];
                    state.kamikaze_targets[i] = 400.0f * normalize(direction) + state.kamikaze_enemies.positions[i];
                }

                state.kamikaze_enemies.positions[i] = move_towards(state.kamikaze_enemies.positions[i], state.kamikaze_targets[i], GameSettings::enemy_move_speed, app.delta_time);

                // Remove enemy if it's offscreen (It can't go up)
                const Vector2 half_sprite_size = Vector2 { 30.0f, 30.0f }; // Hard coded for now
                if (state.kamikaze_enemies.positions[i].y - half_sprite_size.y >= state.game_playground.y ||
                    state.kamikaze_enemies.positions[i].x - half_sprite_size.x >= state.game_playground.x ||
                    state.kamikaze_enemies.positions[i].x + half_sprite_size.x <= 0.0f)
                    entity_remove(state.kamikaze_enemies, i);
            }
        }

        if (state.player_lives > 0)
        {
            {   // Enemy Shooting
                u64 random_enemy_index = Math::random() * (state.enemies[(u64) EnemyType::FLYING].positions.size + state.enemies[(u64) EnemyType::KAMIKAZE].positions.size);

                const u64 type_index = (random_enemy_index < state.enemies[0].positions.size) ? (u64) EnemyType::FLYING : (u64) EnemyType::KAMIKAZE;
                const EntityData& enemies = state.enemies[type_index];

                if (type_index > 0)
                    random_enemy_index -= state.enemies[0].positions.size;

                state.enemy_time_since_last_shot += app.delta_time;

                if (enemies.positions.size > 0 && state.enemy_time_since_last_shot >= state.current_stage.enemy_shot_delay)
                {
                    const Vector2 position = enemies.positions[random_enemy_index] + Vector2 { 0.0f, GameSettings::render_scale.y * bullet_spawn_offset };

                    entity_add(state.enemy_bullets, position, bullet_enemy_animation_index, app.time);
                    Audio::play_sound(sound_enemy_bullet, false);
                    state.enemy_time_since_last_shot = 0;
                }
            }

            {   // Enemy Kamikaze
                constexpr u64 type_index = (u64) EnemyType::KAMIKAZE;
                EntityData& enemies = state.enemies[type_index];
                DynamicArray<Vector2>& slots = state.enemy_slots[type_index];

                state.enemy_time_since_last_kamikaze += app.delta_time;

                if (enemies.positions.size > 0 && state.enemy_time_since_last_kamikaze >= state.current_stage.enemy_kamikaze_delay)
                {
                    const u64 selected_enemy_index = enemies.positions.size - 1;
                    entity_add(state.kamikaze_enemies, enemies.positions[selected_enemy_index], enemies.animations[selected_enemy_index].animation_index, app.time);
                    append(state.kamikaze_targets, state.player_position);

                    // Don't rearrange immediately
                    if (state.empty_slots.size == 0)
                        state.enemy_time_since_last_rearrangement = 0.0f;

                    Vector2 old_slot = remove_swap(state.enemy_slots[type_index], selected_enemy_index);
                    append(state.empty_slots, old_slot);

                    entity_remove(enemies, selected_enemy_index);

                    state.enemy_time_since_last_kamikaze = 0.0f;

                    Audio::play_sound(sound_kamikaze, false);
                }
            }
        }
    }

    {   // Update Bullets
        for (s64 i = state.player_bullets.positions.size - 1; i >= 0; i--)
        {
            Vector2& position = state.player_bullets.positions[i];
            position += Vector2 { 0.0f, -GameSettings::player_bullet_speed * app.delta_time };

            // Remove bullet if it's offscreen
            if (position.y <= 0.0f)
            {
                entity_remove(state.player_bullets, i);
                state.player_kill_streak = 0;
            }
        }

        for (s64 i = state.enemy_bullets.positions.size - 1; i >= 0; i--)
        {
            Vector2& position = state.enemy_bullets.positions[i];
            position += Vector2 { 0.0f, GameSettings::enemy_bullet_speed * app.delta_time };

            // Remove bullet if it's offscreen
            if (position.y >= state.game_playground.y)
                entity_remove(state.enemy_bullets, i);
        }

        if (state.is_lazer_active)
        {
            const Vector2& player_size = GameSettings::render_scale * state.anims[(u64) PlayerState::NORMAL].sprites[0].size;
            state.lazer_position = (state.lazer_start == 0) ? state.player_position - Vector2 { 0.0f, (0.5f * player_size.y) } : state.lazer_position;
            update_lazer(state, app.time);
        }
    }

    {   // Update pickups
        for (s64 i = state.pickups.positions.size - 1; i >= 0; i--)
        {
            Vector2& position = state.pickups.positions[i];
            position += Vector2 { 0.0f, GameSettings::pickup_drop_speed * app.delta_time };

            // Remove pickup if it's offscreen
            if (position.y >= state.game_playground.y)
            {
                state.lazer_drops -= (state.pickups.animations[i].animation_index == (u64) PickupType::LAZER_CHARGE);
                entity_remove(state.pickups, i);
            }
        }
    }

    {   // Remove explosions if they have finished playing
        for (s64 i = state.explosions.animations.size - 1; i >= 0; i--)
        {
            const AnimationData& anim_data = state.explosions.animations[i];
            if (anim_data.instance.loop_count >= 1)
                entity_remove(state.explosions, i);
        }

        for (s64 i = state.power_shot_explosions.animations.size - 1; i >= 0; i--)
        {
            const AnimationData& anim_data = state.power_shot_explosions.animations[i];
            if (anim_data.instance.loop_count >= 1)
                entity_remove(state.power_shot_explosions, i);
        }
    }

    if (!(state.current_screen & GameScreen::MAIN_MENU))
    {
        {   // Test Lazer vs Enemies
            if (state.is_lazer_active)
            {
                const Vector2 sprite_size = GameSettings::render_scale * state.anims[(u64) BulletType::LAZER].sprites[0].size;
                const Vector4 lazer_aabb  = Vector4 {
                    state.lazer_position.x - sprite_size.x / 2.0f,              // left
                    state.lazer_position.y - state.lazer_end   * sprite_size.y, // top
                    state.lazer_position.x + sprite_size.x / 2.0f,              // right
                    state.lazer_position.y - state.lazer_start * sprite_size.y  // bottom
                };
                
                // Coords are swizzled for aabb test
                const Vector4 enemy_aabb_coord = Vector4 {
                    GameSettings::render_scale.x *  0.5f * GameSettings::enemy_collider_size.x,   // right
                    GameSettings::render_scale.y *  0.5f * GameSettings::enemy_collider_size.y,   // bottom
                    GameSettings::render_scale.x * -0.5f * GameSettings::enemy_collider_size.x,   // left
                    GameSettings::render_scale.y * -0.5f * GameSettings::enemy_collider_size.y    // top
                };

                u32 kill_count = 0;

                for (u64 enemy_type = 0; enemy_type < (u64) EnemyType::NUM_TYPES; enemy_type++)
                {
                    EntityData& enemies = state.enemies[enemy_type];
                    
                    for (s64 enemy_i = enemies.positions.size - 1; enemy_i >= 0; enemy_i--)
                    {
                        const Vector2 enemy_position = enemies.positions[enemy_i];
                        const Vector4 enemy_aabb = enemy_aabb_coord + Vector4 { enemy_position.x, enemy_position.y, enemy_position.x, enemy_position.y };

                        if (test_aabb_vs_aabb(lazer_aabb, enemy_aabb))
                        {
                            remove_enemy(state, enemy_type, enemy_i, app.time);

                            if (enemy_type == (u64) EnemyType::DROPPER)
                                spawn_pickup(state, enemy_position, app.time);

                            kill_count++;
                        }
                    }
                }

                // Kamikaze enemies
                for (s64 enemy_i = state.kamikaze_enemies.positions.size - 1; enemy_i >= 0; enemy_i--)
                {
                    const Vector2 enemy_position = state.kamikaze_enemies.positions[enemy_i];
                    const Vector4 enemy_aabb = enemy_aabb_coord + Vector4 { enemy_position.x, enemy_position.y, enemy_position.x, enemy_position.y };

                    if (test_aabb_vs_aabb(lazer_aabb, enemy_aabb))
                    {
                        spawn_explosion(state, state.kamikaze_enemies.positions[enemy_i], app.time);
                        entity_remove(state.kamikaze_enemies, enemy_i);
                        kill_count++;
                    }
                }

                add_score(state, kill_count);
            }
        }

        {   // Test Powered Shot Explosion vs Enemies
            const Vector4 explosion_aabb_coord = Vector4 {
                GameSettings::render_scale.x * -0.5f * GameSettings::player_powered_shot_collider_size.x,   // left
                GameSettings::render_scale.y * -0.5f * GameSettings::player_powered_shot_collider_size.y,   // top
                GameSettings::render_scale.x *  0.5f * GameSettings::player_powered_shot_collider_size.x,   // right
                GameSettings::render_scale.y *  0.5f * GameSettings::player_powered_shot_collider_size.y    // bottom
            };
            
            // Coords are swizzled for aabb test
            const Vector4 enemy_aabb_coord = Vector4 {
                GameSettings::render_scale.x *  0.5f * GameSettings::enemy_collider_size.x,   // right
                GameSettings::render_scale.y *  0.5f * GameSettings::enemy_collider_size.y,   // bottom
                GameSettings::render_scale.x * -0.5f * GameSettings::enemy_collider_size.x,   // left
                GameSettings::render_scale.y * -0.5f * GameSettings::enemy_collider_size.y    // top
            };

            u32 kill_count = 0;

            for (u64 i = 0; i < state.power_shot_explosions.positions.size; i++)
            {
                const Vector2 explosion_position = state.power_shot_explosions.positions[i];
                const Vector4 explosion_aabb = explosion_aabb_coord + Vector4 { explosion_position.x, explosion_position.y, explosion_position.x, explosion_position.y };
                
                for (u64 enemy_type = 0; enemy_type < (u64) EnemyType::NUM_TYPES; enemy_type++)
                {
                    EntityData& enemies = state.enemies[enemy_type];

                    for (s64 enemy_i = enemies.positions.size - 1; enemy_i >= 0; enemy_i--)
                    {
                        const Vector2 enemy_position = enemies.positions[enemy_i];
                        const Vector4 enemy_aabb = enemy_aabb_coord + Vector4 { enemy_position.x, enemy_position.y, enemy_position.x, enemy_position.y };

                        if (test_aabb_vs_aabb(explosion_aabb, enemy_aabb))
                        {
                            remove_enemy(state, enemy_type, enemy_i, app.time);
                            
                            if (enemy_type == (u64) EnemyType::DROPPER)
                                spawn_pickup(state, enemy_position, app.time);
                                
                            kill_count++;
                        }
                    }
                }
                
                // Kamikaze enemies
                for (s64 enemy_i = state.kamikaze_enemies.positions.size - 1; enemy_i >= 0; enemy_i--)
                {
                    const Vector2 enemy_position = state.kamikaze_enemies.positions[enemy_i];
                    const Vector4 enemy_aabb = enemy_aabb_coord + Vector4 { enemy_position.x, enemy_position.y, enemy_position.x, enemy_position.y };

                    if (test_aabb_vs_aabb(explosion_aabb, enemy_aabb))
                    {
                        spawn_explosion(state, state.kamikaze_enemies.positions[enemy_i], app.time);
                        entity_remove(state.kamikaze_enemies, enemy_i);
                        kill_count++;
                    }
                }
            }

            add_score(state, kill_count);
        }

        {   // Test Player Bullets vs Enemies
            const Vector4 bullet_aabb_coord = Vector4 {
                GameSettings::render_scale.x * -0.5f * GameSettings::player_bullet_collider_size.x,   // left
                GameSettings::render_scale.y *  0.0f * GameSettings::player_bullet_collider_size.y,   // top
                GameSettings::render_scale.x *  0.5f * GameSettings::player_bullet_collider_size.x,   // right
                GameSettings::render_scale.y *  1.0f * GameSettings::player_bullet_collider_size.y    // bottom
            };
            
            // Coords are swizzled for aabb test
            const Vector4 enemy_aabb_coord = Vector4 {
                GameSettings::render_scale.x *  0.5f * GameSettings::enemy_collider_size.x,   // right
                GameSettings::render_scale.y *  0.5f * GameSettings::enemy_collider_size.y,   // bottom
                GameSettings::render_scale.x * -0.5f * GameSettings::enemy_collider_size.x,   // left
                GameSettings::render_scale.y * -0.5f * GameSettings::enemy_collider_size.y    // top
            };

            u32 kill_count = 0;

            for (s64 bullet_i = state.player_bullets.positions.size - 1; bullet_i >= 0; bullet_i--)
            {
                const Vector2 bullet_position = state.player_bullets.positions[bullet_i];
                const Vector4 bullet_aabb = bullet_aabb_coord + Vector4 { bullet_position.x, bullet_position.y, bullet_position.x, bullet_position.y };
                
                for (u64 enemy_type = 0; enemy_type < (u64) EnemyType::NUM_TYPES; enemy_type++)
                {
                    EntityData& enemies = state.enemies[enemy_type];

                    for (s64 enemy_i = enemies.positions.size - 1; enemy_i >= 0; enemy_i--)
                    {
                        const Vector2 enemy_position = enemies.positions[enemy_i];
                        const Vector4 enemy_aabb = enemy_aabb_coord + Vector4 { enemy_position.x, enemy_position.y, enemy_position.x, enemy_position.y };

                        if (test_aabb_vs_aabb(bullet_aabb, enemy_aabb))
                        {
                            // Trigger explosion if the bullet is a powered shot
                            if (state.player_bullets.animations[bullet_i].animation_index == (u64) BulletType::POWER_SHOT)
                                entity_add(state.power_shot_explosions, enemy_position, power_shot_explosion_animation_index, app.time);

                            state.player_kill_streak++;

                            // Remove Bullet and Enemy
                            entity_remove(state.player_bullets, bullet_i);
                            remove_enemy(state, enemy_type, enemy_i, app.time);
                            
                            if (enemy_type == (u64) EnemyType::DROPPER)
                                spawn_pickup(state, enemy_position, app.time);
                            else if (!state.is_lazer_active && state.lazer_drops < GameSettings::max_lazer_drops &&
                                     state.lazer_charge < GameSettings::lazer_power_requirement &&
                                     (state.player_kill_streak % GameSettings::lazer_streak_requirement) == 0)
                            {
                                // Drop lazer if it was the last kill of the current streak
                                entity_add(state.pickups, enemy_position, (u64) PickupType::LAZER_CHARGE, app.time);
                                state.lazer_drops++;
                            }

                            kill_count++;

                            // Only need to hit one enemy with one bullet
                            break;
                        }
                    }
                }
                
                // Kamikaze enemies
                for (s64 enemy_i = state.kamikaze_enemies.positions.size - 1; enemy_i >= 0; enemy_i--)
                {
                    const Vector2 enemy_position = state.kamikaze_enemies.positions[enemy_i];
                    const Vector4 enemy_aabb = enemy_aabb_coord + Vector4 { enemy_position.x, enemy_position.y, enemy_position.x, enemy_position.y };

                    if (test_aabb_vs_aabb(bullet_aabb, enemy_aabb))
                    {
                        spawn_explosion(state, state.kamikaze_enemies.positions[enemy_i], app.time);

                        entity_remove(state.player_bullets, bullet_i);
                        entity_remove(state.kamikaze_enemies, enemy_i);

                        kill_count++;
                        
                        // Only need to hit one enemy with one bullet
                        break;
                    }
                }
            }

            add_score(state, kill_count);
        }
    }

    if (state.player_lives > 0)
    {
        // Coords are swizzled for aabb test
        const Vector4 player_aabb_coord = Vector4 {
            GameSettings::render_scale.x *  0.5f * GameSettings::player_collider_size.x,  // right
            GameSettings::render_scale.y *  0.5f * GameSettings::player_collider_size.y,  // bottom
            GameSettings::render_scale.x * -0.5f * GameSettings::player_collider_size.x,  // left
            GameSettings::render_scale.y * -0.5f * GameSettings::player_collider_size.y   // top
        };

        const Vector4 player_aabb = player_aabb_coord + Vector4 { state.player_position.x, state.player_position.y, state.player_position.x, state.player_position.y };

        {   // Test Pickups vs Player
            const Vector4 pickup_aabb_coord = Vector4 {
                GameSettings::render_scale.x * -0.5f * GameSettings::pickup_collider_size.x,    // left
                GameSettings::render_scale.y * -0.5f * GameSettings::pickup_collider_size.y,    // top
                GameSettings::render_scale.x *  0.5f * GameSettings::pickup_collider_size.x,    // right
                GameSettings::render_scale.y *  0.5f * GameSettings::pickup_collider_size.y     // bottom
            };
            
            for (s64 pickup_i = state.pickups.positions.size - 1; pickup_i >= 0; pickup_i--)
            {
                const Vector2 pickup_position = state.pickups.positions[pickup_i];
                const Vector4 pickup_aabb = pickup_aabb_coord + Vector4 { pickup_position.x, pickup_position.y, pickup_position.x, pickup_position.y };

                if (test_aabb_vs_aabb(pickup_aabb, player_aabb))
                {
                    PickupType pickup_type = (PickupType) state.pickups.animations[pickup_i].animation_index;
                    switch (pickup_type)
                    {
                        case PickupType::HEALTH:
                        {
                            state.player_lives = min(state.player_lives + 1, 5);
                        } break;
                        
                        case PickupType::POWER_SHOT:
                        {
                            state.player_equipped_bullet_type = BulletType::POWER_SHOT;
                            state.player_power_shot_ammo = min(state.player_power_shot_ammo + GameSettings::power_shot_drop_ammo, GameSettings::power_shot_max_ammo);
                        } break;
                        
                        case PickupType::EXTRA_SHOT:
                        {
                            state.player_bullets_per_shot = min(state.player_bullets_per_shot + 1, 3u);
                            state.player_extra_shot_ammo  = min(state.player_extra_shot_ammo + GameSettings::extra_shot_drop_ammo, GameSettings::extra_shot_max_ammo);
                        } break;

                        case PickupType::LAZER_CHARGE:
                        {
                            state.lazer_charge++;
                            state.lazer_drops--;

                            if (state.lazer_charge >= GameSettings::lazer_power_requirement && state.player_lives > 0)
                            {
                                state.player_animation.animation_index = (u64) PlayerState::CHARGED;
                                animation_start_instance(state.player_animation.instance, app.time);
                                state.lazer_charge = GameSettings::lazer_power_requirement;

                                Audio::play_buffer(source_lazer_charged, sound_lazer_charged.buffer, true, false);
                            }
                        } break;

                        case PickupType::SKULL:
                        {
                            damage_player(state, app.time);
                        } break;
                    }

                    entity_remove(state.pickups, pickup_i);

                    Audio::play_sound((pickup_type == PickupType::SKULL) ? sound_pickup_bad : sound_pickup_good, false);
                }
            }
        }

        {   // Test Enemy Bullets vs Player
            const Vector4 bullet_aabb_coord = Vector4 {
                GameSettings::render_scale.x * -0.5f * GameSettings::enemy_bullet_collider_size.x,    // left
                GameSettings::render_scale.y * -1.0f * GameSettings::enemy_bullet_collider_size.y,    // top
                GameSettings::render_scale.x *  0.5f * GameSettings::enemy_bullet_collider_size.x,    // right
                GameSettings::render_scale.y *  0.0f * GameSettings::enemy_bullet_collider_size.y     // bottom
            };

            for (s64 bullet_i = state.enemy_bullets.positions.size - 1; bullet_i >= 0; bullet_i--)
            {
                const Vector2 bullet_position = state.enemy_bullets.positions[bullet_i];
                const Vector4 bullet_aabb = bullet_aabb_coord + Vector4 { bullet_position.x, bullet_position.y, bullet_position.x, bullet_position.y };

                if (test_aabb_vs_aabb(bullet_aabb, player_aabb))
                {
                    entity_remove(state.enemy_bullets, bullet_i);
                    damage_player(state, app.time);
                }
            }
        }

        {   // Test Kamikaze Enemies vs Player
            const Vector4 enemy_aabb_coord = Vector4 {
                GameSettings::render_scale.x * -0.5f * GameSettings::enemy_collider_size.x,   // left
                GameSettings::render_scale.y * -0.5f * GameSettings::enemy_collider_size.y,   // top
                GameSettings::render_scale.x *  0.5f * GameSettings::enemy_collider_size.x,   // right
                GameSettings::render_scale.y *  0.5f * GameSettings::enemy_collider_size.y    // bottom
            };

            for (s64 enemy_i = state.kamikaze_enemies.positions.size - 1; enemy_i >= 0; enemy_i--)
            {
                const Vector2 enemy_position = state.kamikaze_enemies.positions[enemy_i];
                const Vector4 enemy_aabb = enemy_aabb_coord + Vector4 { enemy_position.x, enemy_position.y, enemy_position.x, enemy_position.y };

                if (test_aabb_vs_aabb(enemy_aabb, player_aabb))
                {
                    entity_remove(state.kamikaze_enemies, enemy_i);
                    damage_player(state, app.time);
                }
            }
        }
    }

    {   // Player Animation Logic
        if (!(state.current_screen & GameScreen::GAME_OVER))
        {
            if (state.player_animation.animation_index == (u64) PlayerState::HURT && state.player_animation.instance.loop_count >= 5)
            {
                state.player_animation.animation_index = state.player_previous_animation_index;
                animation_start_instance(state.player_animation.instance, app.time);
            }

            if (state.player_animation.animation_index == player_explosion_animation_index && state.player_animation.instance.loop_count > 0)
            {
                state.player_animation.animation_index = (u64) PlayerState::NORMAL;
                animation_start_instance(state.player_animation.instance, app.time);

                screen_switch_to(state, GameScreen::GAME_OVER);

                if (state.player_score > state.player_settings.high_score)
                {
                    state.player_settings.high_score = state.player_score;
                    save_settings(state.player_settings, app.window.style);
                    state.new_high_score = true;

                    // Switch to High Score Screen instead
                    // screen_switch_off(state, GameScreen::GAME_OVER);
                    screen_switch_to(state, GameScreen::HIGH_SCORE);
                }
            }
        }
    }

    {   // Update All Animation Instances
        animation_step_instance(state.anims[state.player_animation.animation_index], state.player_animation.instance, app.time);
        animation_step_instance(state.anims[(u64) BulletType::LAZER], state.lazer_chunk.instance, app.time);

        entity_animation_step(state.anims, state.player_bullets, app.time);
        entity_animation_step(state.anims, state.enemy_bullets, app.time);
        entity_animation_step(state.anims, state.enemies[0], app.time);
        entity_animation_step(state.anims, state.enemies[1], app.time);
        entity_animation_step(state.anims, state.enemies[2], app.time);
        entity_animation_step(state.anims, state.explosions, app.time);
        entity_animation_step(state.anims, state.power_shot_explosions, app.time);
        entity_animation_step(state.anims, state.pickups, app.time);
        entity_animation_step(state.anims, state.kamikaze_enemies, app.time);
    }
}

static inline void pause_all_audio()
{
    if (Audio::source_is_playing(source_lazer))
        Audio::source_pause(source_lazer);
        
    if (Audio::source_is_playing(source_lazer_charged))
        Audio::source_pause(source_lazer_charged);
}

static inline void resume_all_audio()
{
    if (Audio::source_is_playing(source_lazer))
        Audio::source_resume(source_lazer);
        
    if (Audio::source_is_playing(source_lazer_charged))
        Audio::source_resume(source_lazer_charged);
}

void game_state_update(Application& app, GameState& state)
{
    if (Input::get_key_down(Key::GRAVE))
        state.is_debug = !state.is_debug;

    if (!(state.current_screen & (GameScreen::MAIN_MENU | GameScreen::GAME_OVER | GameScreen::SETTINGS_MENU)))
    {
        if (Input::get_key_down(Key::ESCAPE))
        {
            if (state.current_screen & GameScreen::PAUSE_MENU)
            {
                // Resume
                screen_clear_and_switch_to(state, GameScreen::GAME);
                resume_all_audio();
            }
            else
            {
                // Pause
                screen_switch_to(state, GameScreen::PAUSE_MENU);
                pause_all_audio();
            }
        }
    }
    
    if (state.current_screen & (GameScreen::SETTINGS_MENU) && Input::get_key(Key::ESCAPE))
        screen_switch_off(state, GameScreen::SETTINGS_MENU);

    if (state.current_screen & (GameScreen::PAUSE_MENU | GameScreen::SETTINGS_MENU))
        return;

    u64 remaining_enemies;

    coroutine_start(state.state_co);

    if (state.current_screen & GameScreen::MAIN_MENU)
        Audio::play_buffer(source_main_menu, sound_main_menu.buffer, true, false);

    while (true)
    {
        remaining_enemies = state.kamikaze_enemies.positions.size;
        for (u64 enemy_type = 0; enemy_type < (u64) EnemyType::NUM_TYPES; enemy_type++)
            remaining_enemies += state.enemies[enemy_type].positions.size;

        if (remaining_enemies == 0)
        {
            stage_init(state.stage_co, state.current_stage);
            init_enemies(app, state);
            coroutine_yield(state.state_co);
        }

        internal_state_update_gameplay(app, state);
        coroutine_yield(state.state_co);
    }

    coroutine_end(state.state_co);
}

// Uses Imgui
void game_state_render(Application& app, GameState& state, const Imgui::Font& font)
{
    const Vector2 relative_scale = Vector2 { (state.game_rect.right - state.game_rect.left) / state.game_playground.x, (state.game_rect.bottom - state.game_rect.top) / state.game_playground.y };
    Imgui::set_scale(relative_scale.x, relative_scale.y);
    Imgui::set_offset(state.game_rect.left, state.game_rect.top);

    constexpr f32 z_offset = -0.00001f;
    f32 z = 0.8f;

    {   // Render Background
        if (state.player_settings.dynamic_background)
        {
            for (u64 i = 0; i < state.star_positions.size; i++)
            {
                const Vector3 star_position = state.star_positions[i];

                Vector3 position = Vector3 { state.game_playground.x, state.game_playground.y, 0.15f } * star_position;

                const f32 z_multiplier = 1.0f - star_position.z * star_position.z;

                const f32 x_center = 0.5f * state.game_playground.x;
                const f32 x_offset = (state.player_position.x - x_center) * z_multiplier;
                position.x += -GameSettings::background_star_offset_multiplier * x_offset;

                const f32 y_center = state.game_playground.y - 0.5f * GameSettings::player_region_height;
                const f32 y_offset = (state.player_position.y - y_center) * z_multiplier;
                position.y += -GameSettings::background_star_offset_multiplier * y_offset;

                const Sprite& sprite = state.anims[stars_animation_index].sprites[state.star_sprite_indices[i]];
                Imgui::render_sprite(sprite, Vector2 { position.x, position.y }, z);
            }
        }
        else
        {
            for (u64 i = 0; i < state.star_positions.size; i++)
            {
                const Vector3 star_position = state.star_positions[i];
                Vector3 position = Vector3 { state.game_playground.x, state.game_playground.y, 0.15f } * star_position;

                const Sprite& sprite = state.anims[stars_animation_index].sprites[state.star_sprite_indices[i]];
                Imgui::render_sprite(sprite, Vector2 { position.x, position.y }, z);
            }
        }

        z += z_offset;
    }

    if (!(state.current_screen & ~(GameScreen::GAME | GameScreen::GAME_OVER)))
    {
        if (state.is_lazer_active)
        {
            const f32 x_offset = state.game_rect.left + relative_scale.x * GameSettings::screen_shake_amplitude_lazer * (Math::random() * 2 - 1);
            const f32 y_offset = state.game_rect.top  + relative_scale.y * GameSettings::screen_shake_amplitude_lazer * (Math::random() * 2 - 1);
            Imgui::set_offset(x_offset, y_offset);
        }
        else if (state.time_since_screen_shake_start <= GameSettings::screen_shake_enemy_kill_duration)
        {
            const f32 x_offset = state.game_rect.left + relative_scale.x * GameSettings::screen_shake_amplitude_enemy * (Math::random() * 2 - 1);
            const f32 y_offset = state.game_rect.top  + relative_scale.y * GameSettings::screen_shake_amplitude_enemy * (Math::random() * 2 - 1);
            Imgui::set_offset(x_offset, y_offset);
        }
    }

    entity_render(state, state.enemies[0], z);
    entity_render(state, state.enemies[1], z);
    entity_render(state, state.enemies[2], z);
    entity_render(state, state.kamikaze_enemies, z);

    if (state.is_lazer_active)
    {
        const Animation2D& anim = state.anims[(u64) BulletType::LAZER];
        const Vector2 sprite_size = anim.sprites[0].size;

        Vector2 position = state.lazer_position;
        position.y -= state.lazer_start * GameSettings::render_scale.y * sprite_size.y;
        for (u32 i = state.lazer_start; i < state.lazer_end; i++)
        {
            const Sprite& sprite = anim.sprites[state.lazer_chunk.instance.current_frame_index];
            Imgui::render_sprite(sprite, position, z, GameSettings::render_scale);

            position.y -= GameSettings::render_scale.y * sprite.size.y;
        }

        z += z_offset;
    }

    entity_render(state, state.explosions, z);
    entity_render(state, state.power_shot_explosions, z);
    entity_render(state, state.pickups, z);
    entity_render(state, state.player_bullets, z);
    entity_render(state, state.enemy_bullets, z);
    
    // Render Player
    if (!(state.current_screen & GameScreen::GAME_OVER))
    {
        const Animation2D& anim = state.anims[state.player_animation.animation_index];
        const Sprite& sprite = anim.sprites[state.player_animation.instance.current_frame_index];
        Imgui::render_sprite(sprite, state.player_position, z, GameSettings::render_scale);

        z += z_offset;
    }

    if (!(state.current_screen & (GameScreen::MAIN_MENU | GameScreen::GAME_OVER)))
    {
        // Render UI

        {   // Render lives
            constexpr f32 x_offset = 20.0f;
            const f32 scale = GameSettings::render_scale.x / 2.0f;

            const Sprite& sprite = state.anims[(u64) PlayerState::NORMAL].sprites[0];
            Vector2 position = Vector2 { scale * (sprite.size.x / 2.0f), state.game_playground.y - scale * (sprite.size.y / 2.0f) };

            for (u32 i = 0; i < state.player_lives; i++)
            {
                Imgui::render_sprite(sprite, position, z, Vector2 { scale, scale });
                position.x += sprite.size.x + x_offset;
            }
        }

        constexpr f32 x_padding = 10.0f;
        char text_buffer[128];

        {   // Render Ammo
            Vector2 position = state.game_playground;

            // Render Power Shot Ammo
            if (state.player_equipped_bullet_type == BulletType::POWER_SHOT)
            {
                {   // Count
                    constexpr f32 scale = 1.0f;
                    const f32 font_size = scale * font.size;

                    String text = ref(text_buffer, 0);
                    to_string(text, state.player_power_shot_ammo);

                    const Vector2 size = Imgui::get_rendered_text_size(text, font, font_size);
                    Imgui::render_text(text, font, position - size + Vector2 { 0.0f, 1.0f }, z, font_size);

                    position.x -= size.x + x_padding;
                }

                {   // Sprite
                    const Sprite& sprite = state.anims[(u64) PickupType::POWER_SHOT].sprites[0];
                    const Vector2 scale = Vector2 { 2.5f, 2.5f };
                    const Vector2 size = scale * sprite.size;

                    Imgui::render_sprite(sprite, position - 0.5f * size, z, scale);
                    position.x -= size.x + x_padding;
                }

                z += z_offset;
            }

            position.x -= x_padding;

            // Render Extra Shot Ammo
            if (state.player_bullets_per_shot > 1)
            {
                {   // Count
                    constexpr f32 scale = 1.0f;
                    const f32 font_size = scale * font.size;

                    String text = ref(text_buffer, 0);
                    to_string(text, state.player_extra_shot_ammo);

                    const Vector2 size = Imgui::get_rendered_text_size(text, font, font_size);
                    Imgui::render_text(text, font, position - size + Vector2 { 0.0f, 1.0f }, z, font_size);

                    position.x -= size.x + x_padding;
                }

                {   // Sprite
                    const Sprite& sprite = state.anims[(u64) PickupType::EXTRA_SHOT].sprites[0];
                    const Vector2 scale = Vector2 { 2.5f, 2.5f };
                    const Vector2 size = scale * sprite.size;

                    Imgui::render_sprite(sprite, position - 0.5f * size, z, scale);
                    position.x -= size.x + x_padding;
                }

                z += z_offset;
            }
        }
        
        {   // Render Score
            {   // Text
                constexpr f32 scale = 1.0f;
                const f32 font_size = scale * font.size;

                Vector4 color = Vector4(1.0f);
                if (state.player_score <= state.player_settings.high_score)
                    sprintf(text_buffer, "Score: %u", state.player_score);
                else
                {
                    sprintf(text_buffer, "High Score: %u", state.player_score);
                    color = high_score_color;
                }

                String text = ref(text_buffer);

                const Vector2 size = Imgui::get_rendered_text_size(text, font, font_size);
                const Vector2 top_left = Vector2 { 0.5f * (state.game_playground.x - size.x), 0.0f };
                Imgui::render_text(text, font, top_left, z, font_size, color);
            }
        }

        {   // Render Lazer Charge
            Vector2 position = Vector2 { 0.0f, 0.0f };

            {   // Sprite
                const Sprite& sprite = state.anims[(u64) PickupType::LAZER_CHARGE].sprites[0];
                const Vector2 scale = Vector2 { 2.5f, 2.5f };
                const Vector2 size = scale * sprite.size;

                Imgui::render_sprite(sprite, position + 0.5f * size, z, scale);
                position.x += size.x + x_padding;
            }

            {   // Count
                constexpr f32 scale = 1.0f;
                const f32 font_size = scale * font.size;

                sprintf(text_buffer, "%u/%u", state.lazer_charge, GameSettings::lazer_power_requirement);

                String text = ref(text_buffer);
                Imgui::render_text(text, font, position - Vector2 { 0.0f, 3.0f }, z, font_size);
            }
        }
        
        {   // Render Active Bonuses
            constexpr f32 y_padding = 10.0f;
            f32 y = 0.0f;

            {   // Total Bonus
                constexpr f32 scale = 1.0f;
                const f32 font_size = scale * font.size;

                // Kill streak
                const s32 kill_streaks = clamp(state.player_kill_streak / GameSettings::lazer_streak_requirement, 0, GameSettings::max_kill_streak_multipliers);
                f32 multiplier = Math::pow(GameSettings::kill_streak_multiplier, kill_streaks);
                multiplier *= (state.player_lives == 1) ? GameSettings::low_health_multiplier : 1.0f;

                sprintf(text_buffer, "Bonus: x%.1f", multiplier);

                String text = ref(text_buffer);

                const Vector2 size = Imgui::get_rendered_text_size(text, font, font_size);
                const Vector2 top_left = Vector2 { (state.game_playground.x - size.x), y };
                Imgui::render_text(text, font, top_left, z, font_size);

                y += size.y + y_padding;
            }
            
            {   // Kill Streaks
                static f32 fade_out_start_time = -100.0f;

                const s32 kill_streaks = clamp(state.player_kill_streak / GameSettings::lazer_streak_requirement, 0, GameSettings::max_kill_streak_multipliers);
                if (kill_streaks > 0)
                    fade_out_start_time = app.time;

                const f32 time_since_fade_out = app.time - fade_out_start_time;
                if (time_since_fade_out < GameSettings::ui_fade_out_time)
                {
                    constexpr f32 scale = 1.0f / Math::golden_ratio;
                    const f32 font_size = scale * font.size;

                    sprintf(text_buffer, "+ %dx Streaks", kill_streaks);

                    String text = ref(text_buffer);

                    const Vector2 size = Imgui::get_rendered_text_size(text, font, font_size);
                    const Vector2 top_left = Vector2 { (state.game_playground.x - size.x), y };

                    const f32 alpha = 1.0f - (time_since_fade_out / GameSettings::ui_fade_out_time);
                    Imgui::render_text(text, font, top_left, z, font_size, Vector4 { 1.0f, 1.0f, 1.0f, alpha });

                    y += size.y + y_padding;
                }
            }
            
            {   // One the Wire ()
                static f32 fade_out_start_time = -100.0f;

                if (state.player_lives == 1)
                    fade_out_start_time = app.time;

                const f32 time_since_fade_out = app.time - fade_out_start_time;
                if (time_since_fade_out < GameSettings::ui_fade_out_time)
                {
                    constexpr f32 scale = 1.0f / Math::golden_ratio;
                    const f32 font_size = scale * font.size;

                    String text = ref("+ On The Wire");

                    const Vector2 size = Imgui::get_rendered_text_size(text, font, font_size);
                    const Vector2 top_left = Vector2 { (state.game_playground.x - size.x), y };

                    const f32 alpha = 1.0f - (time_since_fade_out / GameSettings::ui_fade_out_time);
                    Imgui::render_text(text, font, top_left, z, font_size, Vector4 { 1.0f, 1.0f, 1.0f, alpha });

                    y += size.y + y_padding;
                }
            }
        }

        z += z_offset;
    }

    Imgui::set_offset(state.game_rect.left, state.game_rect.top);

    if (state.current_screen & GameScreen::SETTINGS_MENU)
    {
        constexpr f32 y_offset = 5.0f;
        f32 y = state.game_playground.y * (1.0f - (1.0f / Math::golden_ratio));
        
        {   // Darken Background
            Rect rect = Rect { 0.0f, 0.0f, (f32) state.game_playground.x, (f32) state.game_playground.y };
            Imgui::render_rect(rect, z, Vector4 { 0.0f, 0.0f, 0.0f, GameSettings::ui_background_alpha });

            z += z_offset;
        }
        
        {   // Main Text
            constexpr f32 scale = Math::golden_ratio;
            const f32 font_size = scale * font.size;
            
            char buffer[] = "Settings";
            const String text = ref(buffer, (u64)(sizeof(buffer) - 1));

            const Vector2 size = Imgui::get_rendered_text_size(text, font, font_size);
            const Vector2 top_left = Vector2 { 0.5f * (state.game_playground.x - size.x), y };
            Imgui::render_text(text, font, top_left, z, font_size, heading_color);

            y += size.y + 2 * y_offset;
        }

        bool dirty = false;
        Settings& settings = state.player_settings;
        
        {   // Control Scheme
            constexpr f32 x_padding = 10.0f;
            constexpr f32 y_padding = 5.0f;

            constexpr f32 scale = 1.0f;
            const f32 font_size = scale * font.size;

            char buffer[256];
            sprintf(buffer, "Control Scheme: %s", control_scheme_name(settings.control_scheme).data);
            String text = ref(buffer);

            const Vector2 size = Imgui::get_rendered_text_size(text, font, font_size);
            Rect rect = Rect { 0.5f * (state.game_playground.x - size.x - 2 * x_padding), y - y_padding, 0.5f * (state.game_playground.x + size.x + 2 * x_padding), y + size.y + y_padding };

            if (Imgui::render_text_button(imgui_gen_id(), rect, text, font, Vector2 { x_padding, y_padding }, z, font_size))
            {
                settings.control_scheme = (ControlScheme) (((u32) settings.control_scheme + 1) % (u32) ControlScheme::NUM_SCHEMES);
                dirty = true;
            }

            y += size.y + 2 * y_padding + y_offset;
        }
        
        {   // Dynamic Background
            constexpr f32 x_padding = 10.0f;
            constexpr f32 y_padding = 5.0f;

            constexpr f32 scale = 1.0f;
            const f32 font_size = scale * font.size;

            char buffer[] = "Dynamic Background: [X]";
            String text = ref(buffer, (u64)(sizeof(buffer) - 1));

            text[text.size - 2] = settings.dynamic_background ? 'X' : ' ';

            const Vector2 size = Imgui::get_rendered_text_size(text, font, font_size);
            Rect rect = Rect { 0.5f * (state.game_playground.x - size.x - 2 * x_padding), y - y_padding, 0.5f * (state.game_playground.x + size.x + 2 * x_padding), y + size.y + y_padding };

            if (Imgui::render_text_button(imgui_gen_id(), rect, text, font, Vector2 { x_padding, y_padding }, z, font_size))
            {
                settings.dynamic_background = !settings.dynamic_background;
                dirty = true;
            }

            y += size.y + 2 * y_padding + y_offset;
        }
        
        {   // Window Style
            constexpr f32 x_padding = 10.0f;
            constexpr f32 y_padding = 5.0f;

            constexpr f32 scale = 1.0f;
            const f32 font_size = scale * font.size;

            char buffer[256];
            sprintf(buffer, "Window Style: %s", window_style_name(app.window.style).data);
            String text = ref(buffer);

            const Vector2 size = Imgui::get_rendered_text_size(text, font, font_size);
            Rect rect = Rect { 0.5f * (state.game_playground.x - size.x - 2 * x_padding), y - y_padding, 0.5f * (state.game_playground.x + size.x + 2 * x_padding), y + size.y + y_padding };

            if (Imgui::render_text_button(imgui_gen_id(), rect, text, font, Vector2 { x_padding, y_padding }, z, font_size))
            {
                WindowStyle style = (WindowStyle) (((u32) app.window.style + 1) % (u32) WindowStyle::NUM_STYLES);
                application_set_window_style(app, style);
                dirty = true;
            }

            y += size.y + 2 * y_padding + y_offset;
        }
        
        {   // Volume Slider
            constexpr f32 x_padding = 10.0f;

            constexpr f32 slider_width  = 100.0f;
            constexpr f32 slider_height = 20.0f;

            f32 x = 0.5f * state.game_playground.x;
            f32 height = 0.0f;

            {   // Text
                constexpr f32 scale = 1.0f;
                const f32 font_size = scale * font.size;

                char buffer[] = "Volume:";
                String text = ref(buffer, sizeof(buffer) - 1);
                const Vector2 size = Imgui::get_rendered_text_size(text, font, font_size);

                x -= 0.5f * (size.x + x_padding + slider_width);

                const Vector2 top_left = Vector2 { x, y };
                Imgui::render_text(text, font, top_left, z, font_size);

                x += size.x + x_padding;
                height = size.y;
            }

            {   // Slider
                const Rect rect = Rect { x, y, x + slider_width, y + height };
                f32 new_volume = Imgui::render_slider(imgui_gen_id(), settings.volume, min_volume, max_volume, rect, Vector2 { 10.0f, height }, z, !state.player_settings.mute_audio);
                if (new_volume != settings.volume)
                {
                    Audio::set_master_volume(inv_lerp(new_volume, min_volume, max_volume));
                    settings.volume = new_volume;
                    dirty = true;
                }

                x += slider_width + x_padding;
            }

            y += height + y_offset + y_offset;
        }
        
        {   // Mute Audio
            constexpr f32 x_padding = 10.0f;
            constexpr f32 y_padding = 5.0f;

            constexpr f32 scale = 1.0f;
            const f32 font_size = scale * font.size;

            char buffer[] = "Mute Audio: [X]";
            String text = ref(buffer, (u64)(sizeof(buffer) - 1));

            text[text.size - 2] = settings.mute_audio ? 'X' : ' ';

            const Vector2 size = Imgui::get_rendered_text_size(text, font, font_size);
            Rect rect = Rect { 0.5f * (state.game_playground.x - size.x - 2 * x_padding), y - y_padding, 0.5f * (state.game_playground.x + size.x + 2 * x_padding), y + size.y + y_padding };

            if (Imgui::render_text_button(imgui_gen_id(), rect, text, font, Vector2 { x_padding, y_padding }, z, font_size))
            {
                settings.mute_audio = !settings.mute_audio;
                Audio::set_master_volume((f32) !settings.mute_audio);
                dirty = true;
            }

            y += size.y + 2 * y_padding + y_offset;
        }
        
        {   // Save Settings
            if (dirty)
                save_settings(settings, app.window.style);
        }
        
        {   // Back button
            constexpr f32 x_padding = 10.0f;
            constexpr f32 y_padding = 5.0f;

            constexpr f32 scale = 1.0f / Math::golden_ratio;
            const f32 font_size = scale * font.size;

            char buffer[] = "Back";
            const String text = ref(buffer, (u64)(sizeof(buffer) - 1));

            const Vector2 size = Imgui::get_rendered_text_size(text, font, font_size);

            Rect rect = Rect { state.game_playground.x - size.x - 2 * x_padding, state.game_playground.y - size.y - 2 * y_padding, state.game_playground.x, state.game_playground.y };

            if (Imgui::render_text_button(imgui_gen_id(), rect, text, font, Vector2 { x_padding, y_padding }, z, font_size))
                screen_switch_off(state, GameScreen::SETTINGS_MENU);

        }

        z += z_offset;
    }
    else if (state.current_screen & GameScreen::MAIN_MENU)
    {
        constexpr f32 y_offset = 5.0f;
        f32 y = state.game_playground.y * (1.0f - (1.0f / Math::golden_ratio));

        char text_buffer[256];

        {   // Text
            constexpr f32 scale = 1.0f;
            const f32 font_size = scale * font.size;

            sprintf(text_buffer, "High Score: %u", state.player_settings.high_score);

            String text = ref(text_buffer);

            const Vector2 size = Imgui::get_rendered_text_size(text, font, font_size);
            const Vector2 top_left = Vector2 { 0.5f * (state.game_playground.x - size.x), 0.0f };
            Imgui::render_text(text, font, top_left, z, font_size);
        }
        
        {   // Main Text
            constexpr f32 scale = Math::golden_ratio;
            const f32 font_size = scale * font.size;
            
            char buffer[] = "Invaders from Outer Space!";
            const String text = ref(buffer, (u64)(sizeof(buffer) - 1));

            const Vector2 size = Imgui::get_rendered_text_size(text, font, font_size);
            const Vector2 top_left = Vector2 { 0.5f * (state.game_playground.x - size.x), y };
            Imgui::render_text(text, font, top_left, z, font_size, heading_color);

            y += size.y + y_offset;
        }
        
        {   // Help Text
            const s32 blink_iterations = (s32) Math::floor(app.time / GameSettings::ui_blink_delay);
            const f32 alpha = (blink_iterations % 2 == 0);

            constexpr f32 scale = 1.0f;
            const f32 font_size = scale * font.size;

            char buffer[] = "Press ENTER to start";
            const String text = ref(buffer, (u64)(sizeof(buffer) - 1));

            const Vector2 size = Imgui::get_rendered_text_size(text, font, font_size);
            const Vector2 top_left = Vector2 { 0.5f * (state.game_playground.x - size.x), y };
            Imgui::render_text(text, font, top_left, z, font_size, Vector4 { 0.5f, 0.5f, 0.5f, alpha });

            y += size.y + y_offset;
        }

        {   // "Tutorial"
            const f32 font_size = GameSettings::ui_tutorial_font_scale * font.size;

            char buffer[128];
            sprintf(buffer, "%s to move\nSPACE to shoot\nZ to use lazer (when charged)", control_scheme_name(state.player_settings.control_scheme).data);
            const String text = ref(buffer);

            const Vector2 size = Imgui::get_rendered_text_size(text, font, font_size);
            const Vector2 top_left = Vector2 { 0.0f, state.game_playground.y - size.y };
            Imgui::render_text(text, font, top_left, z, font_size, Vector4 { 1.0f, 1.0f, 1.0f, 0.5f });

            z += z_offset;
        }

        {   // Bottom Left Buttons
            f32 y = state.game_playground.y;

            {   // Quit button
                constexpr f32 x_padding = 10.0f;
                constexpr f32 y_padding = 5.0f;

                constexpr f32 scale = 1.0f / Math::golden_ratio;
                const f32 font_size = scale * font.size;

                char buffer[] = "Quit";
                const String text = ref(buffer, (u64)(sizeof(buffer) - 1));

                const Vector2 size = Imgui::get_rendered_text_size(text, font, font_size);

                Rect rect = Rect { state.game_playground.x - size.x - 2 * x_padding, y - size.y - 2 * y_padding, state.game_playground.x, y };

                if (Imgui::render_text_button(imgui_gen_id(), rect, text, font, Vector2 { x_padding, y_padding }, z, font_size))
                    app.is_running = false;

                y -= size.y + 3 * y_padding;
            }

            {   // Settings button
                constexpr f32 x_padding = 10.0f;
                constexpr f32 y_padding = 5.0f;

                constexpr f32 scale = 1.0f / Math::golden_ratio;
                const f32 font_size = scale * font.size;

                char buffer[] = "Settings";
                const String text = ref(buffer, (u64)(sizeof(buffer) - 1));

                const Vector2 size = Imgui::get_rendered_text_size(text, font, font_size);

                Rect rect = Rect { state.game_playground.x - size.x - 2 * x_padding, y - size.y - 2 * y_padding, state.game_playground.x, y };

                if (Imgui::render_text_button(imgui_gen_id(), rect, text, font, Vector2 { x_padding, y_padding }, z, font_size))
                    screen_switch_to(state, GameScreen::SETTINGS_MENU);

                y -= size.y + 3 * y_padding;
            }

            z += z_offset;
        }

        if (Input::get_key_down(Key::ENTER))
        {
            screen_clear_and_switch_to(state, GameScreen::GAME);

            // Delete all player bullets
            clear(state.player_bullets.animations);
            clear(state.player_bullets.positions);

            Audio::source_stop(source_main_menu);
        }
    }
    else if (state.current_screen & GameScreen::PAUSE_MENU)
    {
        constexpr f32 y_offset = 5.0f;
        f32 y = state.game_playground.y * (1.0f - (1.0f / Math::golden_ratio));

        {   // Darken Background
            Rect rect = Rect { 0.0f, 0.0f, (f32) state.game_playground.x, (f32) state.game_playground.y };
            Imgui::render_rect(rect, z, Vector4 { 0.0f, 0.0f, 0.0f, GameSettings::ui_background_alpha });

            z += z_offset;
        }

        {   // Main Text
            constexpr f32 scale = Math::golden_ratio;
            const f32 font_size = scale * font.size;

            char buffer[] = "Paused";
            const String text = ref(buffer, (u64)(sizeof(buffer) - 1));

            const Vector2 size = Imgui::get_rendered_text_size(text, font, font_size);
            const Vector2 top_left = Vector2 { 0.5f * (state.game_playground.x - size.x), y };
            Imgui::render_text(text, font, top_left, z, font_size, heading_color);

            y += size.y + y_offset;
        }
        
        {   // Help Text
            const s32 blink_iterations = (s32) Math::floor(app.time / GameSettings::ui_blink_delay);
            const f32 alpha = (blink_iterations % 2 == 0);
            
            constexpr f32 scale = 1.0f;
            const f32 font_size = scale * font.size;

            char buffer[] = "Press ESC to resume";
            const String text = ref(buffer, (u64)(sizeof(buffer) - 1));

            const Vector2 size = Imgui::get_rendered_text_size(text, font, font_size);
            const Vector2 top_left = Vector2 { 0.5f * (state.game_playground.x - size.x), y };
            Imgui::render_text(text, font, top_left, z, font_size, Vector4 { 0.5f, 0.5f, 0.5f, alpha });

            y += size.y + y_offset;
        }

        {   // Bottom Left Buttons
            f32 y = state.game_playground.y;

            {   // Quit button
                constexpr f32 x_padding = 10.0f;
                constexpr f32 y_padding = 5.0f;

                constexpr f32 scale = 1.0f / Math::golden_ratio;
                const f32 font_size = scale * font.size;

                char buffer[] = "Quit";
                const String text = ref(buffer, (u64)(sizeof(buffer) - 1));

                const Vector2 size = Imgui::get_rendered_text_size(text, font, font_size);

                Rect rect = Rect { state.game_playground.x - size.x - 2 * x_padding, y - size.y - 2 * y_padding, state.game_playground.x, y };

                if (Imgui::render_text_button(imgui_gen_id(), rect, text, font, Vector2 { x_padding, y_padding }, z, font_size))
                    app.is_running = false;

                y -= size.y + 3 * y_padding;
            }

            {   // Settings button
                constexpr f32 x_padding = 10.0f;
                constexpr f32 y_padding = 5.0f;

                constexpr f32 scale = 1.0f / Math::golden_ratio;
                const f32 font_size = scale * font.size;

                char buffer[] = "Settings";
                const String text = ref(buffer, (u64)(sizeof(buffer) - 1));

                const Vector2 size = Imgui::get_rendered_text_size(text, font, font_size);

                Rect rect = Rect { state.game_playground.x - size.x - 2 * x_padding, y - size.y - 2 * y_padding, state.game_playground.x, y };

                if (Imgui::render_text_button(imgui_gen_id(), rect, text, font, Vector2 { x_padding, y_padding }, z, font_size))
                    screen_switch_to(state, GameScreen::SETTINGS_MENU);

                y -= size.y + 3 * y_padding;
            }
            
            {   // Restart button
                constexpr f32 x_padding = 10.0f;
                constexpr f32 y_padding = 5.0f;

                constexpr f32 scale = 1.0f / Math::golden_ratio;
                const f32 font_size = scale * font.size;

                char buffer[] = "Restart";
                const String text = ref(buffer, (u64)(sizeof(buffer) - 1));

                const Vector2 size = Imgui::get_rendered_text_size(text, font, font_size);

                Rect rect = Rect { state.game_playground.x - size.x - 2 * x_padding, y - size.y - 2 * y_padding, state.game_playground.x, y };

                if (Imgui::render_text_button(imgui_gen_id(), rect, text, font, Vector2 { x_padding, y_padding }, z, font_size))
                {
                    game_state_reset(app, state);
                    screen_clear_and_switch_to(state, GameScreen::GAME);
                }

                y -= size.y + 3 * y_padding;
            }

            z += z_offset;
        }
    }
    else if (state.current_screen & GameScreen::HIGH_SCORE)
    {
        constexpr f32 y_offset = 5.0f;
        f32 y = state.game_playground.y * (1.0f - (1.0f / Math::golden_ratio));

        {   // Darken Background
            Rect rect = Rect { 0.0f, 0.0f, (f32) state.game_playground.x, (f32) state.game_playground.y };
            Imgui::render_rect(rect, z, Vector4 { 0.0f, 0.0f, 0.0f, GameSettings::ui_background_alpha });

            z += z_offset;
        }

        {   // Main Text
            constexpr f32 scale = Math::golden_ratio;
            const f32 font_size = scale * font.size;

            char buffer[] = "New High Score!";
            const String text = ref(buffer, (u64)(sizeof(buffer) - 1));

            const Vector2 size = Imgui::get_rendered_text_size(text, font, font_size);
            const Vector2 top_left = Vector2 { 0.5f * (state.game_playground.x - size.x), y };
            Imgui::render_text(text, font, top_left, z, font_size, high_score_color);

            y += size.y + y_offset;
        }
        
        {   // Score Text
            constexpr f32 scale = 1.0f;
            const f32 font_size = scale * font.size;

            char buffer[128];
            sprintf(buffer, "Score: %u", state.player_score);
            
            const String text = ref(buffer);
            const Vector2 size = Imgui::get_rendered_text_size(text, font, font_size);
            const Vector2 top_left = Vector2 { 0.5f * (state.game_playground.x - size.x), y };
            Imgui::render_text(text, font, top_left, z, font_size, Vector4 { 1.0f, 1.0f, 1.0f, 1.0f });

            y += size.y + y_offset;
        }
        
        {   // Help Text
            const s32 blink_iterations = (s32) Math::floor(app.time / GameSettings::ui_blink_delay);
            const f32 alpha = (blink_iterations % 2 == 0);

            constexpr f32 scale = 1.0f;
            const f32 font_size = scale * font.size;

            char buffer[] = "Press ENTER to continue";
            const String text = ref(buffer, (u64)(sizeof(buffer) - 1));

            const Vector2 size = Imgui::get_rendered_text_size(text, font, font_size);
            const Vector2 top_left = Vector2 { 0.5f * (state.game_playground.x - size.x), y };
            Imgui::render_text(text, font, top_left, z, font_size, Vector4 { 0.5f, 0.5f, 0.5f, alpha });

            y += size.y + y_offset;
        }
        
        {   // Bottom Left Buttons
            f32 y = state.game_playground.y;

            {   // Quit button
                constexpr f32 x_padding = 10.0f;
                constexpr f32 y_padding = 5.0f;

                constexpr f32 scale = 1.0f / Math::golden_ratio;
                const f32 font_size = scale * font.size;

                char buffer[] = "Quit";
                const String text = ref(buffer, (u64)(sizeof(buffer) - 1));

                const Vector2 size = Imgui::get_rendered_text_size(text, font, font_size);

                Rect rect = Rect { state.game_playground.x - size.x - 2 * x_padding, y - size.y - 2 * y_padding, state.game_playground.x, y };

                if (Imgui::render_text_button(imgui_gen_id(), rect, text, font, Vector2 { x_padding, y_padding }, z, font_size))
                    app.is_running = false;

                y -= size.y + 3 * y_padding;
            }

            {   // Settings button
                constexpr f32 x_padding = 10.0f;
                constexpr f32 y_padding = 5.0f;

                constexpr f32 scale = 1.0f / Math::golden_ratio;
                const f32 font_size = scale * font.size;

                char buffer[] = "Settings";
                const String text = ref(buffer, (u64)(sizeof(buffer) - 1));

                const Vector2 size = Imgui::get_rendered_text_size(text, font, font_size);

                Rect rect = Rect { state.game_playground.x - size.x - 2 * x_padding, y - size.y - 2 * y_padding, state.game_playground.x, y };

                if (Imgui::render_text_button(imgui_gen_id(), rect, text, font, Vector2 { x_padding, y_padding }, z, font_size))
                    screen_switch_to(state, GameScreen::SETTINGS_MENU);

                y -= size.y + 3 * y_padding;
            }
            

            if (Input::get_key_down(Key::ENTER))
                screen_switch_off(state, GameScreen::HIGH_SCORE);

            z += z_offset;
        }
    }
    else if (state.current_screen & GameScreen::GAME_OVER)
    {
        constexpr f32 y_offset = 5.0f;
        f32 y = state.game_playground.y * (1.0f - (1.0f / Math::golden_ratio));

        {   // Darken Background
            Rect rect = Rect { 0.0f, 0.0f, (f32) state.game_playground.x, (f32) state.game_playground.y };
            Imgui::render_rect(rect, z, Vector4 { 0.0f, 0.0f, 0.0f, GameSettings::ui_background_alpha });

            z += z_offset;
        }

        {   // Main Text
            constexpr f32 scale = Math::golden_ratio;
            const f32 font_size = scale * font.size;

            char buffer[] = "You Died";
            const String text = ref(buffer, (u64)(sizeof(buffer) - 1));

            const Vector2 size = Imgui::get_rendered_text_size(text, font, font_size);
            const Vector2 top_left = Vector2 { 0.5f * (state.game_playground.x - size.x), y };
            Imgui::render_text(text, font, top_left, z, font_size, heading_color);

            y += size.y + y_offset;
        }
        
        {   // Score Text
            constexpr f32 scale = 1.0f;
            const f32 font_size = scale * font.size;

            char buffer[128];
            Vector4 color = Vector4(1.0f);

            if (!state.new_high_score)
                sprintf(buffer, "Score: %u", state.player_score);
            else
            {
                sprintf(buffer, "New High Score: %u", state.player_score);
                color = high_score_color;
            }
            
            const String text = ref(buffer);
            const Vector2 size = Imgui::get_rendered_text_size(text, font, font_size);
            const Vector2 top_left = Vector2 { 0.5f * (state.game_playground.x - size.x), y };
            Imgui::render_text(text, font, top_left, z, font_size, color);

            y += size.y + y_offset;
        }
        
        {   // Help Text
            const s32 blink_iterations = (s32) Math::floor(app.time / GameSettings::ui_blink_delay);
            const f32 alpha = (blink_iterations % 2 == 0);

            constexpr f32 scale = 1.0f;
            const f32 font_size = scale * font.size;

            char buffer[] = "Press ENTER to restart";
            const String text = ref(buffer, (u64)(sizeof(buffer) - 1));

            const Vector2 size = Imgui::get_rendered_text_size(text, font, font_size);
            const Vector2 top_left = Vector2 { 0.5f * (state.game_playground.x - size.x), y };
            Imgui::render_text(text, font, top_left, z, font_size, Vector4 { 0.5f, 0.5f, 0.5f, alpha });

            y += size.y + y_offset;
        }
        
        {   // Bottom Left Buttons
            f32 y = state.game_playground.y;

            {   // Quit button
                constexpr f32 x_padding = 10.0f;
                constexpr f32 y_padding = 5.0f;

                constexpr f32 scale = 1.0f / Math::golden_ratio;
                const f32 font_size = scale * font.size;

                char buffer[] = "Quit";
                const String text = ref(buffer, (u64)(sizeof(buffer) - 1));

                const Vector2 size = Imgui::get_rendered_text_size(text, font, font_size);

                Rect rect = Rect { state.game_playground.x - size.x - 2 * x_padding, y - size.y - 2 * y_padding, state.game_playground.x, y };

                if (Imgui::render_text_button(imgui_gen_id(), rect, text, font, Vector2 { x_padding, y_padding }, z, font_size))
                    app.is_running = false;

                y -= size.y + 3 * y_padding;
            }

            {   // Settings button
                constexpr f32 x_padding = 10.0f;
                constexpr f32 y_padding = 5.0f;

                constexpr f32 scale = 1.0f / Math::golden_ratio;
                const f32 font_size = scale * font.size;

                char buffer[] = "Settings";
                const String text = ref(buffer, (u64)(sizeof(buffer) - 1));

                const Vector2 size = Imgui::get_rendered_text_size(text, font, font_size);

                Rect rect = Rect { state.game_playground.x - size.x - 2 * x_padding, y - size.y - 2 * y_padding, state.game_playground.x, y };

                if (Imgui::render_text_button(imgui_gen_id(), rect, text, font, Vector2 { x_padding, y_padding }, z, font_size))
                    screen_switch_to(state, GameScreen::SETTINGS_MENU);

                y -= size.y + 3 * y_padding;
            }

            z += z_offset;
        }

        if (Input::get_key_down(Key::ENTER))
        {
            game_state_reset(app, state);
            screen_clear_and_switch_to(state, GameScreen::GAME);
        }

        z += z_offset;
    }
    
    {   // Render Black Bars to hide anything off the playground
        const Vector4 color = Vector4 { 0.0f, 0.0, 0.0f, 1.0f };

        {   // Left
            const Rect rect = Rect { -100.0f, 0.0f, 0.0f, state.game_playground.y };
            Imgui::render_rect(rect, z, color);
        }

        {   // Top
            const Rect rect = Rect { -100.0f, -100.0f, state.game_playground.x + 100.0f, 0.0f };
            Imgui::render_rect(rect, z, color);
        }

        {   // Right
            const Rect rect = Rect { state.game_playground.x, 0.0f, state.game_playground.x + 100.0f, state.game_playground.y };
            Imgui::render_rect(rect, z, color);
        }

        {   // Bottom
            const Rect rect = Rect { -100.0f, state.game_playground.y, state.game_playground.x + 100.0f, state.game_playground.y + 100.0f };
            Imgui::render_rect(rect, z, color);
        }

        z += z_offset;
    }

    #ifndef GN_RELEASE
    if (state.is_debug)
    {
        {   // Render Player Bullet AABBs
            const Vector4 bullet_aabb_coord = Vector4 {
                GameSettings::render_scale.x * -0.5f * GameSettings::player_bullet_collider_size.x,
                GameSettings::render_scale.y *  0.0f * GameSettings::player_bullet_collider_size.y,
                GameSettings::render_scale.x *  0.5f * GameSettings::player_bullet_collider_size.x,
                GameSettings::render_scale.y *  1.0f * GameSettings::player_bullet_collider_size.y
            };

            Rect rect;
            for (u64 i = 0; i < state.player_bullets.positions.size; i++)
            {
                const Vector2 bullet_position = state.player_bullets.positions[i];
                const Vector4 bullet_aabb = bullet_aabb_coord + Vector4 { bullet_position.x, bullet_position.y, bullet_position.x, bullet_position.y };

                rect.v4 = bullet_aabb;
                Imgui::render_rect(rect, z, Vector4 { 1.0f, 0.0f, 0.0f, 0.5f });

                z += z_offset;
            }
        }

        {   // Render Enemy Bullet AABBs
            const Vector4 bullet_aabb_coord = Vector4 {
                GameSettings::render_scale.x * -0.5f * GameSettings::enemy_bullet_collider_size.x,
                GameSettings::render_scale.y * -1.0f * GameSettings::enemy_bullet_collider_size.y,
                GameSettings::render_scale.x *  0.5f * GameSettings::enemy_bullet_collider_size.x,
                GameSettings::render_scale.y *  0.0f * GameSettings::enemy_bullet_collider_size.y
            };

            Rect rect;
            for (u64 i = 0; i < state.enemy_bullets.positions.size; i++)
            {
                const Vector2 bullet_position = state.enemy_bullets.positions[i];
                const Vector4 bullet_aabb = bullet_aabb_coord + Vector4 { bullet_position.x, bullet_position.y, bullet_position.x, bullet_position.y };

                rect.v4 = bullet_aabb;
                Imgui::render_rect(rect, z, Vector4 { 1.0f, 1.0f, 0.0f, 0.5f });

                z += z_offset;
            }
        }

        
        {   // Render Player Powered Shot Explosion AABBs
            const Vector4 explosion_aabb_coord = Vector4 {
                GameSettings::render_scale.x * -0.5f * GameSettings::player_powered_shot_collider_size.x,
                GameSettings::render_scale.y * -0.5f * GameSettings::player_powered_shot_collider_size.y,
                GameSettings::render_scale.x *  0.5f * GameSettings::player_powered_shot_collider_size.x,
                GameSettings::render_scale.y *  0.5f * GameSettings::player_powered_shot_collider_size.y
            };

            Rect rect;
            for (u64 i = 0; i < state.power_shot_explosions.positions.size; i++)
            {
                const Vector2 explosion_position = state.power_shot_explosions.positions[i];
                const Vector4 explosion_aabb = explosion_aabb_coord + Vector4 { explosion_position.x, explosion_position.y, explosion_position.x, explosion_position.y };

                rect.v4 = explosion_aabb;
                Imgui::render_rect(rect, z, Vector4 { 1.0f, 0.0f, 1.0f, 0.5f });

                z += z_offset;
            }
        }

        {   // Render Enemy AABBs
            const Vector4 enemy_aabb_coord = Vector4 {
                GameSettings::render_scale.x * -0.5f * GameSettings::enemy_collider_size.x,
                GameSettings::render_scale.y * -0.5f * GameSettings::enemy_collider_size.y,
                GameSettings::render_scale.x *  0.5f * GameSettings::enemy_collider_size.x,
                GameSettings::render_scale.y *  0.5f * GameSettings::enemy_collider_size.y
            };

            Rect rect;

            for (u64 enemy_type = 0; enemy_type < (u64) EnemyType::NUM_TYPES; enemy_type++)
            {
                EntityData& enemies = state.enemies[enemy_type];

                for (u64 i = 0; i < enemies.positions.size; i++)
                {
                    const Vector2 enemy_position = enemies.positions[i];
                    const Vector4 enemy_aabb = enemy_aabb_coord + Vector4 { enemy_position.x, enemy_position.y, enemy_position.x, enemy_position.y };

                    rect.v4 = enemy_aabb;
                    Imgui::render_rect(rect, z, Vector4 { 0.0f, 1.0f, 0.0f, 0.35f });

                    z += z_offset;
                }
            }

            for (u64 i = 0; i < state.kamikaze_enemies.positions.size; i++)
            {
                const Vector2 enemy_position = state.kamikaze_enemies.positions[i];
                const Vector4 enemy_aabb = enemy_aabb_coord + Vector4 { enemy_position.x, enemy_position.y, enemy_position.x, enemy_position.y };

                rect.v4 = enemy_aabb;
                Imgui::render_rect(rect, z, Vector4 { 0.0f, 1.0f, 0.0f, 0.35f });

                z += z_offset;
            }
        }

        {   // Render Player AABB
            const Vector4 player_aabb_coord = Vector4 {
                GameSettings::render_scale.x * -0.5f * GameSettings::player_collider_size.x,
                GameSettings::render_scale.y * -0.5f * GameSettings::player_collider_size.y,
                GameSettings::render_scale.x *  0.5f * GameSettings::player_collider_size.x,
                GameSettings::render_scale.y *  0.5f * GameSettings::player_collider_size.y
            };

            const Vector4 player_aabb = player_aabb_coord + Vector4 { state.player_position.x, state.player_position.y, state.player_position.x, state.player_position.y };

            Rect rect;
            rect.v4 = player_aabb;
            Imgui::render_rect(rect, z, Vector4 { 0.0f, 0.3f, 1.0f, 0.35f });

            z += z_offset;
        }

        {   // Render Lazer AABB
            if (state.is_lazer_active)
            {
                const Vector2 sprite_size = GameSettings::render_scale * state.anims[(u64) BulletType::LAZER].sprites[0].size;
                const Vector4 lazer_aabb  = Vector4 {
                    state.lazer_position.x - sprite_size.x / 2.0f,              // left
                    state.lazer_position.y - state.lazer_end   * sprite_size.y, // top
                    state.lazer_position.x + sprite_size.x / 2.0f,              // right
                    state.lazer_position.y - state.lazer_start * sprite_size.y  // bottom
                };

                Rect rect;
                rect.v4 = lazer_aabb;
                Imgui::render_rect(rect, z, Vector4 { 1.0f, 0.3f, 0.0f, 0.5f });
                
                z += z_offset;
            }
        }

        {   // Render Pickup Deck
            constexpr f32 padding = 5.0f;
            const Vector2 sprite_size = state.anims[(u64) pickup_deck[0]].sprites[0].size; 

            Vector2 position = Vector2 { 0.5f * (state.game_playground.x - pickup_deck.size * sprite_size.x - (pickup_deck.size - 1) * padding), state.game_playground.y - sprite_size.y };
            for (u64 i = 0; i < pickup_deck.size; i++)
            {
                const Sprite& sprite = state.anims[(u64) pickup_deck[i]].sprites[0];
                const f32 alpha = (i == pickup_deck_index) ? 1.0f : 0.5f;

                Imgui::render_sprite(sprite, position, z, Vector2 { 1.0f, 1.0f }, Vector4 { 1.0f, 1.0f, 1.0f, alpha });
                position.x += sprite.size.x + padding;
            }

            z += z_offset;
        }
    }
    #endif GN_RELEASE

    Imgui::set_offset(0, 0);
    Imgui::set_scale(1, 1);
}

void game_state_window_resize(const Application& app, GameState& state)
{
    constexpr f32 aspect_ratio = 224.0f / 256.0f;
    const f32 current_aspect_ratio = (f32) app.window.ref_width / (f32) app.window.ref_height;

    Vector2 size;
    if (current_aspect_ratio >= aspect_ratio)
        size = Vector2 { (aspect_ratio * app.window.ref_height) - 2 * GameSettings::padding.x, app.window.ref_height - 2 * GameSettings::padding.y };
    else
        size = Vector2 { app.window.ref_width - 2 * GameSettings::padding.x, (app.window.ref_width / aspect_ratio) - 2 * GameSettings::padding.y };
    
    const f32 render_offset_x = 0.5f * (app.window.ref_width  - size.x);
    const f32 render_offset_y = 0.5f * (app.window.ref_height - size.y);
    state.game_rect = Rect { render_offset_x, render_offset_y, render_offset_x + size.x, render_offset_y + size.y };
}