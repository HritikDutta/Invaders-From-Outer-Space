#pragma once

#include "containers/bytes.h"
#include "containers/string.h"
#include "core/types.h"
#include "imgui.h"

namespace Imgui
{
    
// Serialization
Bytes font_encode_to_bytes(const Font& font);

} // namespace Imgui