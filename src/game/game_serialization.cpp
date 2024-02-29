#include "game_serialization.h"

#include "application/application.h"
#include "containers/string.h"
#include "containers/string_builder.h"
#include "fileio/fileio.h"
#include "serialization/json.h"
#include "player_settings.h"
#include "game_state.h"

void save_settings(const Settings& settings, const WindowStyle window_style)
{
    StringBuilder builder = make<StringBuilder>();
    char buffer0[128], buffer1[128], buffer2[128], buffer3[128];

    append(builder, ref("{ "));

    {   // Control Scheme
        append(builder, ref("\"control_scheme\": "));

        sprintf(buffer0, "\"%s\"", control_scheme_name(settings.control_scheme).data);
        append(builder, ref(buffer0));
    }

    {   // Dynamic Background
        append(builder, ref(", \"dynamic_background\": "));
        append(builder, ref(settings.dynamic_background ? "true" : "false"));
    }

    {   // Window Style
        append(builder, ref(", \"window_style\": "));

        sprintf(buffer1, "\"%s\"", window_style_name(window_style).data);
        append(builder, ref(buffer1));
    }

    {   // Volume
        append(builder, ref(", \"volume\": "));

        sprintf(buffer2, "%0.1f", settings.volume);
        append(builder, ref(buffer2));
    }

    {   // Mute Audio
        append(builder, ref(", \"mute_audio\": "));
        append(builder, ref(settings.mute_audio ? "true" : "false"));
    }

    {   // High Score
        append(builder, ref(", \"high_score\": "));

        sprintf(buffer3, "%u", settings.high_score);
        append(builder, ref(buffer3));
    }

    append(builder, ref(" }"));

    String data = build_string(builder);
    file_write_string(ref(settings_file_name, settings_file_name_size), data);

    free(builder);
    free(data);
}

void load_settings_from_json(const Json::Document& document, Settings& settings, WindowStyle& window_style)
{
    const Json::Value& j_data = document.start();

    settings.dynamic_background = j_data[ref("dynamic_background")].boolean();
    settings.mute_audio = j_data[ref("mute_audio")].boolean();
    settings.volume = j_data[ref("volume")].float64();

    settings.high_score = j_data[ref("high_score")].int64();

    const String style_string = j_data[ref("window_style")].string();

    if (style_string == ref("Windowed"))
        window_style = WindowStyle::WINDOWED;
    else if (style_string == ref("Fullscreen"))
        window_style = WindowStyle::FULLSCREEN;
    else if (style_string == ref("Borderless"))
        window_style = WindowStyle::BORDERLESS;
    else
        gn_assert_with_message(false, "Unsupported Window Style! (style name: %)", style_string);

    const String scheme_string = j_data[ref("control_scheme")].string();

    if (scheme_string == control_scheme_name(ControlScheme::WASD))
        settings.control_scheme = ControlScheme::WASD;
    else if (scheme_string == control_scheme_name(ControlScheme::ARROW_KEYS))
        settings.control_scheme = ControlScheme::ARROW_KEYS;
    else
        gn_assert_with_message(false, "Unsupported Control Scheme! (scheme: %)", scheme_string);
}