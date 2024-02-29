#include "audio.h"

#ifdef GN_PLATFORM_WINDOWS

#include "containers/bytes.h"
#include "containers/darray.h"
#include "containers/hash.h"
#include "containers/hash_table.h"
#include "core/logger.h"
#include "core/types.h"
#include "internal/audio_wav_codes.h"
#include "platform/platform.h"

#include <windows.h>
#include <combaseapi.h>
#include <xaudio2.h>

namespace Audio
{

template<>
struct Hasher<WavFmtData>
{
    inline Hash operator()(WavFmtData const& key) const
    {
        const Bytes as_bytes = Bytes { (u8*)&key, sizeof(WavFmtData) };
        return Hasher<Bytes>()(as_bytes);
    }
};

inline bool operator==(const WavFmtData& fmt1, const WavFmtData& fmt2)
{
    return platform_compare_memory(&fmt1, &fmt2, sizeof(WavFmtData));
}

using SourcePool = DynamicArray<Source>;

static struct
{
    IXAudio2* xa_engine;
    IXAudio2MasteringVoice* xa_mastering_voice;

    DynamicArray<SourcePool> source_pools;
    HashTable<WavFmtData, u64> idle_source_pool_table;
    HashTable<Source, u64>     active_source_pool_table;

    DynamicArray<Source> sources_to_be_pooled;

    s32 active_sources;
    s32 total_sources;
} audio_data;

class VoiceCallback : public IXAudio2VoiceCallback
{
public:
    void OnVoiceProcessingPassStart(UINT32 BytesRequired) noexcept override {}
    void OnVoiceProcessingPassEnd() noexcept override {}

    void OnStreamEnd() noexcept override {}

    void OnBufferStart(void* pBufferContext) noexcept override {}
    void OnBufferEnd(void* pBufferContext) noexcept override
    {
        if (pBufferContext)
            append(audio_data.sources_to_be_pooled, (Source) pBufferContext);
    }

    void OnLoopEnd(void* pBufferContext) noexcept override {}

    void OnVoiceError(void* pBufferContext, HRESULT Error) noexcept override {}
};

bool init()
{
    // Initialize COM (for threading)
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr))
    {
        print_error("Failed to intialize multithreading for audio!\n");
        return false;
    }

    hr = XAudio2Create(&audio_data.xa_engine, 0, XAUDIO2_DEFAULT_PROCESSOR);
    if (FAILED(hr))
    {
        print_error("Failed to intialize XAudio Engine!\n");
        return false;
    }

    audio_data.xa_engine->StartEngine();

    hr = audio_data.xa_engine->CreateMasteringVoice(&audio_data.xa_mastering_voice);
    if (FAILED(hr))
    {
        print_error("Failed to create mastering voice for XAudio!\n");
        return false;
    }

    {   // Initialize source pools
        audio_data.source_pools = make<DynamicArray<SourcePool>>();

        audio_data.idle_source_pool_table = make<HashTable<WavFmtData, u64>>();
        audio_data.active_source_pool_table = make<HashTable<Source, u64>>();

        audio_data.sources_to_be_pooled = make<DynamicArray<Source>>();

        audio_data.active_sources = 0;
    }

    return true;
}

void shutdown()
{
    audio_data.xa_engine->StopEngine();
    audio_data.xa_mastering_voice->DestroyVoice();

    for (u64 i = 0; i < audio_data.source_pools.size; i++)
    {
        for (u64 j = 0; j < audio_data.source_pools[i].size; j++)
        {
            Source& source = audio_data.source_pools[i][j];
            source_destroy(source);
        }

        free(audio_data.source_pools[i]);
    }

    free(audio_data.source_pools);
}

void source_destroy(Audio::Source& source)
{
    if (!source)
        return;

    IXAudio2SourceVoice* voice = (IXAudio2SourceVoice*) source;
    voice->Stop();
    voice->FlushSourceBuffers();
    voice->DestroyVoice();

    source = nullptr;
    Audio::audio_data.total_sources--;
}

bool load_from_bytes(const Bytes bytes, Sound& sound)
{
    u32 const* as_u32 = (u32*) bytes.data;

    {   // Make sure the file is WAV
        gn_assert_with_message(as_u32[0] == char_code_RIFF && as_u32[2] == char_code_WAVE, "Given bytes are not from a wave file!");
        as_u32 = &as_u32[3];
    }

    {   // Read fmt data
        gn_assert_with_message(as_u32[0] == char_code_FMT, "2nd subchunk is not fmt!");
        const u32 sub_chunk_size = as_u32[1];
        platform_copy_memory(&sound.fmt, &as_u32[2], sizeof(WavFmtData));

        as_u32 = (u32*) ((u8*) &as_u32[2] + sub_chunk_size);
    }

    {   // Read Buffer Data
        gn_assert_with_message(as_u32[0] == char_code_DATA, "3rd subchunk is not data!");
        const u32 sub_chunk_size = as_u32[1];

        sound.buffer.data = (u8*) platform_reallocate(sound.buffer.data, sub_chunk_size);
        gn_assert_with_message(sound.buffer.data, "Couldn't reallocate data for sound buffer!");

        platform_copy_memory(sound.buffer.data, &as_u32[2], sub_chunk_size);
        sound.buffer.size = sub_chunk_size;
    }

    return true;
}

static inline u64 find_pool_index(const WavFmtData& fmt)
{
    const auto& elem = find(audio_data.idle_source_pool_table, fmt);
    if (elem)
        return elem.value();

    // No pool exists for given fmt, create one
    const u64 pool_index = audio_data.source_pools.size;
    put(audio_data.idle_source_pool_table, fmt, pool_index);
    append(audio_data.source_pools, make<SourcePool>());

    return pool_index;
}

void pool_sources()
{
    for (s64 i = audio_data.sources_to_be_pooled.size - 1; i >= 0; i--)
    {
        Source source = pop(audio_data.sources_to_be_pooled);
        auto& elem = find(audio_data.active_source_pool_table, source);

        gn_assert_with_message(elem, "Audio source can't be pooled since it wasn't considered active!");
        append(audio_data.source_pools[elem.value()], source);

        remove(elem);   // Remove element from active sources

        audio_data.active_sources--;
    }
}

Source source_create(const WavFmtData& fmt)
{
    IXAudio2SourceVoice* voice;

    VoiceCallback* callback = new VoiceCallback;    // Need to call constructor
    HRESULT hr = audio_data.xa_engine->CreateSourceVoice(&voice, (WAVEFORMATEX*) &fmt, 0, XAUDIO2_DEFAULT_FREQ_RATIO, callback);
    if (FAILED(hr))
    {
        print_error("Failed to create source voice for XAudio!\n");
        return nullptr;
    }

    // Find pool index for fmt
    const u64 pool_index = find_pool_index(fmt);
    put(audio_data.active_source_pool_table, (Source) voice, pool_index);

    audio_data.active_sources++;
    audio_data.total_sources++;

    return (Source) voice;
}

void source_resume(Audio::Source& source)
{
    gn_assert_with_message(source, "Audio source was null!");

    IXAudio2SourceVoice* voice = (IXAudio2SourceVoice*) source;
    voice->Start(0);
}

void source_pause(Audio::Source& source)
{
    gn_assert_with_message(source, "Audio source was null!");

    IXAudio2SourceVoice* voice = (IXAudio2SourceVoice*) source;
    voice->Stop();
}

void source_stop(Audio::Source& source)
{
    gn_assert_with_message(source, "Audio source was null!");

    IXAudio2SourceVoice* voice = (IXAudio2SourceVoice*) source;
    voice->Stop();
    voice->FlushSourceBuffers();
}

void source_set_volume(Audio::Source& source, f32 volume)
{
    gn_assert_with_message(source, "Audio source was null!");

    IXAudio2SourceVoice* voice = (IXAudio2SourceVoice*) source;
    voice->SetVolume(volume);
}

bool source_is_playing(const Audio::Source& source)
{
    gn_assert_with_message(source, "Audio source was null!");

    IXAudio2SourceVoice* voice = (IXAudio2SourceVoice*) source;

    XAUDIO2_VOICE_STATE state;
    voice->GetState(&state, XAUDIO2_VOICE_NOSAMPLESPLAYED);

    return state.BuffersQueued > 0;
}

bool play_buffer(Source& source, const Bytes buffer, bool loop, bool pool_source)
{
    // Ignore null sources
    if (!source)
        return false;

    IXAudio2SourceVoice* voice = (IXAudio2SourceVoice*) source;

    XAUDIO2_BUFFER xa_buffer = {};
    xa_buffer.pAudioData = buffer.data;
    xa_buffer.AudioBytes = buffer.size;
    xa_buffer.Flags = XAUDIO2_END_OF_STREAM;
    xa_buffer.LoopCount = (loop) ? XAUDIO2_LOOP_INFINITE : 0;

    if (pool_source)
        xa_buffer.pContext = (void*) source;

    HRESULT hr = voice->SubmitSourceBuffer(&xa_buffer);
    if (FAILED(hr))
    {
        print_error("Failed to sumit buffer to source voice for XAudio!\n");
        return false;
    }

    voice->Start(0);
    return true;
}

static Source find_or_create_source(const WavFmtData& fmt)
{
    auto& elem = find(audio_data.idle_source_pool_table, fmt);
    if (elem)
    {
        SourcePool& pool = audio_data.source_pools[elem.value()];
        if (pool.size > 0)
        {
            Source source = pop(pool);

            // Add source to active sources
            put(audio_data.active_source_pool_table, source, elem.value());

            audio_data.active_sources++;
            return source;
        }
    }

    return source_create(fmt);
}

bool play_sound(const Sound& sound, bool loop)
{
    Source source = find_or_create_source(sound.fmt);
    return play_buffer(source, sound.buffer, loop, true);
}

void set_master_volume(f32 volume)
{
    audio_data.xa_mastering_voice->SetVolume(volume);
}

s32 get_active_source_count()
{
    return audio_data.active_sources;
}

s32 get_total_source_count()
{
    return audio_data.total_sources;
}

} // namespace Audio

#endif // GN_PLATFORM_WINDOWS