#pragma once

#include "application/application.h"
#include "serialization/json.h"
#include "player_settings.h"
#include "game_state.h"

static char settings_file_name[] = "assets/settings/player_settings.json";
static u64  settings_file_name_size = sizeof(settings_file_name) - 1;

void save_settings(const Settings& settings, const WindowStyle window_style);
void load_settings_from_json(const Json::Document& document, Settings& settings, WindowStyle& window_style);