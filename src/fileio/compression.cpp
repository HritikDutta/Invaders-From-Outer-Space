#include "compression.h"

#include "containers/bytes.h"
#include "core/types.h"
#include "core/common.h"
#include "core/logger.h"
#include "platform/platform.h"
#include "miniz.h"

Bytes compress_bytes(const Bytes& uncompressed_bytes)
{
    // The compressed bytes store the decompression ratio as a float at the beginning

    u8* compressed_bytes = (u8*) platform_allocate(sizeof(f32) + uncompressed_bytes.size);
    mz_ulong compressed_size = compressBound(uncompressed_bytes.size);

    int status = mz_compress(compressed_bytes + sizeof(f32), &compressed_size, uncompressed_bytes.data, (mz_ulong) uncompressed_bytes.size);
    gn_assert_with_message(status == Z_OK, "Couldn't compress the given bytes!");

    if (compressed_size != uncompressed_bytes.size)
    {
        compressed_bytes = (u8*) platform_reallocate(compressed_bytes, (u64) compressed_size);
        gn_assert_with_message(compressed_bytes, "Couldn't reallocate compressed bytes!");
    }

    // Store decompression ratio
    *(f32*) compressed_bytes = (f32) uncompressed_bytes.size / (f32) compressed_size;

    return Bytes { compressed_bytes, sizeof(f32) + (u64) compressed_size };
}

Bytes decompress_bytes(const Bytes& compressed_bytes)
{
    f32 decompression_ratio = *(f32*) compressed_bytes.data;

    u8* actual_data = compressed_bytes.data + sizeof(f32);
    u64 data_size   = compressed_bytes.size - sizeof(f32);

    u8* uncompressed_bytes = (u8*) platform_allocate(decompression_ratio * compressed_bytes.size);
    mz_ulong uncompressed_size = decompression_ratio * compressed_bytes.size;

    int status = mz_uncompress(uncompressed_bytes, &uncompressed_size, compressed_bytes.data + sizeof(f32), (mz_ulong) (compressed_bytes.size - sizeof(f32)));
    gn_assert_with_message(status == Z_OK, "Couldn't uncompress the given bytes!");

    if (uncompressed_size != compressed_bytes.size)
    {
        uncompressed_bytes = (u8*) platform_reallocate(uncompressed_bytes, (u64) uncompressed_size);
        gn_assert_with_message(uncompressed_bytes, "Couldn't reallocate uncompressed bytes!");
    }

    return Bytes { uncompressed_bytes, (u64) uncompressed_size };
}