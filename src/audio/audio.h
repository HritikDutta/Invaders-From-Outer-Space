#pragma once

#include "core/types.h"
#include "containers/bytes.h"

namespace Audio
{

using Source = void*;

struct WavFmtData
{
    u16 format_tag;
    u16 channels;
    u32 sample_rate;
    u32 byte_rate;
    u16 block_align;
    u16 bits_per_sample;
    u16 extra_params_size;
};

struct Sound
{
    WavFmtData fmt;
    Bytes      buffer;
};

bool init();
void shutdown();

void pool_sources();

bool load_from_bytes(const Bytes bytes, Sound& sound);

Source source_create(const WavFmtData& fmt);
void source_destroy(Audio::Source& source);

void source_resume(Audio::Source& source);
void source_pause(Audio::Source& source);
void source_stop(Audio::Source& source);

void source_set_volume(Audio::Source& source, f32 volume);

bool source_is_playing(const Audio::Source& source);

bool play_buffer(Source& source, const Bytes buffer, bool loop, bool pool_source);
bool play_sound(const Sound& sound, bool loop);

void set_master_volume(f32 volume);

s32 get_active_source_count();
s32 get_total_source_count();

} // namespace Audio