#pragma once

#include "containers/darray.h"
#include "serialization/json.h"
#include "sprite.h"

bool animation_load_from_json(const Json::Document& document, DynamicArray<Animation2D>& anims);