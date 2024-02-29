#pragma once

#include "core/types.h"

struct InternalState;   // Defined based on the OS
enum struct WindowStyle;

struct PlatformState
{
    InternalState* internal_state;
};

// Window Stuff

bool platform_window_startup(PlatformState& pstate, const char* window_name, int x, int y, int width, int height, const char* icon_path, WindowStyle style);
void platform_window_shutdown(PlatformState& pstate);

bool platform_pump_messages();

void platform_set_window_style(WindowStyle style);

// Memory Stuff

void* platform_allocate(u64 size);                   // TODO: Option for aligned memory
void* platform_reallocate(void* block, u64 size);    // TODO: Option for aligned memory
void  platform_free(void* block);                    // TODO: Option for aligned memory

void* platform_zero_memory(void* block, u64 size);
void* platform_copy_memory(void* dest, const void* source, u64 size);
void* platform_set_memory(void* dest, s32 value, u64 size);

bool platform_compare_memory(const void* ptr1, const void* ptr2, u64 size);

// Time Stuff

void platform_init_clock();
f64  platform_get_time_absolute();
f64  platform_get_time();

// Input Stuff

void platform_get_mouse_position(s32& x, s32& y);
void platform_set_mouse_position(s32 x, s32 y);
void platform_show_mouse_cursor(bool value);

// File Stuff

bool platform_dialogue_open_file(const char filter[], char* out_filepath, u32 max_path_size);