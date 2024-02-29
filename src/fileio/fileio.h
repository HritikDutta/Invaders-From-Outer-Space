#pragma once

#include "core/types.h"
#include "containers/string.h"
#include "containers/bytes.h"

String file_load_string(const String& filepath);
Bytes  file_load_bytes(const String& filepath);

void file_write_string(const String& filepath, const String& string);
void file_write_bytes(const String& filepath, const Bytes& bytes);