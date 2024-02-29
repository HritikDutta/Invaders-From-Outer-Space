#define STB_IMAGE_IMPLEMENTATION

#include "platform/platform.h"
#define STBI_MALLOC platform_allocate
#define STBI_REALLOC platform_reallocate
#define STBI_FREE platform_free

#include "../include/stb_image.h"