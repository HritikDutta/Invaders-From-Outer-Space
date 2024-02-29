#pragma once

#include "containers/bytes.h"

Bytes compress_bytes(const Bytes& uncompressed_bytes);
Bytes decompress_bytes(const Bytes& compressed_bytes);