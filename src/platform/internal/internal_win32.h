#pragma once

#ifdef GN_PLATFORM_WINDOWS

#include <windows.h>

struct InternalState
{
    HINSTANCE h_instance;
    HWND hwnd;
    HDC hdc;

    WINDOWPLACEMENT window_placement_prev;
};

#endif // GN_PLATFORM_WINDOWS