#pragma once

#include "platform/platform.h"

bool graphics_init(InternalState& state);
void graphics_shutdown(InternalState& state);

void graphics_swap_buffers(const PlatformState& pstate);
void graphics_resize_canvas_callback(s32 width, s32 height);

void graphics_set_vsync(bool value);

void graphics_set_clear_color(f32 red, f32 green, f32 blue, f32 alpha);
void graphics_clear_canvas();