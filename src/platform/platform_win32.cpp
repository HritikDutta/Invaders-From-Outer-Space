#include "platform.h"

#ifdef GN_PLATFORM_WINDOWS

#include "core/types.h"
#include "core/input.h"
#include "core/input_processing.h"
#include "internal/internal_win32.h"
#include "graphics/graphics.h"
#include "application/application.h"
#include "application/application_internal.h"
#include <cstdlib>
#include <windows.h>

// Clock Stuff
static f64 clock_frequency;
static LARGE_INTEGER start_time;
static PlatformState* g_pstate = nullptr;

// Window Stuff

LRESULT CALLBACK win32_process_message(HWND hwnd, u32 msg, WPARAM wParam, LPARAM lParam);

static inline u32 get_window_style_mask(WindowStyle style)
{
    switch (style)
    {
        case WindowStyle::WINDOWED:
            return WS_OVERLAPPEDWINDOW;
        
        case WindowStyle::BORDERLESS:
            return 0;
        
        case WindowStyle::FULLSCREEN:
            return 0;
    }

    // Not gonna reach here
    return WS_OVERLAPPEDWINDOW;
}

bool platform_window_startup(PlatformState& pstate, const char* window_name, int x, int y, int width, int height, const char* icon_path, WindowStyle style)
{
    g_pstate = &pstate;

    pstate.internal_state = (InternalState*) platform_allocate(sizeof(InternalState));
    InternalState& state = *pstate.internal_state;

    state.h_instance = GetModuleHandleA(0);

    HICON icon;
    if (icon_path)
        icon = (HICON) LoadImageA(state.h_instance, icon_path, IMAGE_ICON, 0, 0, LR_LOADFROMFILE);
    else
        icon = LoadIcon(state.h_instance, IDI_APPLICATION);
        
    WNDCLASSA wc = {};
    wc.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc = win32_process_message;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = state.h_instance;
    wc.hIcon = icon;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);   // NULL; Manage the cursor manually
    wc.hbrBackground = NULL;                    // Transparent
    wc.lpszClassName = "Gonad Window Class";

    if (!RegisterClassA(&wc))
    {
        MessageBoxA(0, "Window Registration Failed", "Error", MB_ICONEXCLAMATION | MB_OK);
        return false;
    }

    u32 window_style    = WS_OVERLAPPEDWINDOW;  // Start as overlapped window
    u32 window_ex_style = WS_EX_APPWINDOW;

    // Get size of window
    RECT border_rect = { 0 };
    AdjustWindowRectEx(&border_rect, window_style, 0, window_ex_style);
    
    // Adjust border size and position
    u32 window_x = x + border_rect.left;
    u32 window_y = y + border_rect.top;
    u32 window_width = width + (border_rect.right - border_rect.left);
    u32 window_height = height + (border_rect.bottom - border_rect.top);

    state.hwnd = CreateWindowExA(window_ex_style, "Gonad Window Class", window_name,
                                 window_style, window_x, window_y, window_width, window_height,
                                 0, 0, state.h_instance, 0);

    if (!state.hwnd)
    {
        MessageBoxA(NULL, "Window creation failed", "Error!", MB_ICONEXCLAMATION | MB_OK);
        return false;
    }

    SetWindowLongPtrA(state.hwnd, GWLP_USERDATA, (LONG_PTR) &pstate);

    if (!graphics_init(state))
    {
        MessageBoxA(NULL, "Graphics intialization failed", "Error!", MB_ICONEXCLAMATION | MB_OK);
        return false;
    }

    // Show the window
    s32 show_window_command_flags = SW_SHOW;
    // This should be SW_MAXIMIZE for maximizing at start
    ShowWindow(state.hwnd, show_window_command_flags);
    
    platform_set_window_style(style);
    
    platform_init_clock();

    return true;
}

void platform_window_shutdown(PlatformState& pstate)
{
    InternalState& state = *pstate.internal_state;

    if (state.hwnd)
    {
        DestroyWindow(state.hwnd);
        state.hwnd = 0;
    }
}

bool platform_pump_messages()
{
    MSG message;
    while (PeekMessageA(&message, NULL, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&message);
        DispatchMessageA(&message);
    }

    return true;
}

void platform_set_window_style(WindowStyle style)
{
    InternalState& state = *g_pstate->internal_state;
    SetWindowLong(state.hwnd, GWL_STYLE, get_window_style_mask(style));

    if (style == WindowStyle::FULLSCREEN)
    {
        MONITORINFO mi = { sizeof(mi) };

        if (GetWindowPlacement(state.hwnd, &state.window_placement_prev) &&
            GetMonitorInfo(MonitorFromWindow(state.hwnd, MONITOR_DEFAULTTOPRIMARY), &mi))
        {
            SetWindowPos(state.hwnd, NULL,
                         mi.rcMonitor.left, mi.rcMonitor.top,
                         mi.rcMonitor.right - mi.rcMonitor.left,
                         mi.rcMonitor.bottom - mi.rcMonitor.top,
                         SWP_NOOWNERZORDER | SWP_FRAMECHANGED | SWP_SHOWWINDOW | SWP_DRAWFRAME);
        }
    }
    else
    {
        SetWindowPlacement(state.hwnd, &state.window_placement_prev);
        SetWindowPos(state.hwnd, NULL, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                     SWP_NOOWNERZORDER | SWP_FRAMECHANGED | SWP_SHOWWINDOW | SWP_DRAWFRAME);
    }
}

// Memory Stuff
void* platform_allocate(u64 size)
{
    return malloc(size);
}

void* platform_reallocate(void* block, u64 size)
{
    return realloc(block, size);
}

void platform_free(void* block)
{
    free(block);
}

void* platform_zero_memory(void* dest, u64 size)
{
    return memset(dest, 0, size);
}

void* platform_copy_memory(void* dest, const void* source, u64 size)
{
    return memcpy(dest, source, size);
}

void* platform_set_memory(void* dest, s32 value, u64 size)
{
    return memset(dest, value, size);
}

bool platform_compare_memory(const void* ptr1, const void* ptr2, u64 size)
{
    return memcmp(ptr1, ptr2, size) == 0;
}

// Time Stuff

void platform_init_clock()
{
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    clock_frequency = 1.0 / (f64) frequency.QuadPart;
    QueryPerformanceCounter(&start_time);
}

f64 platform_get_time_absolute()
{
    LARGE_INTEGER now_time;
    QueryPerformanceCounter(&now_time);
    return (f64) now_time.QuadPart * clock_frequency;
}

f64 platform_get_time()
{
    LARGE_INTEGER now_time;
    QueryPerformanceCounter(&now_time);
    return (f64) (now_time.QuadPart - start_time.QuadPart) * clock_frequency;
}

LRESULT CALLBACK win32_process_message(HWND hwnd, u32 msg, WPARAM wParam, LPARAM lParam)
{
    PlatformState* pstate = g_pstate;

    switch (msg)
    {
        case WM_ERASEBKGND:
            // Notify the OS that the erasing will be handled by the app to prevent flickering
            return 1;
        
        case WM_CLOSE:
        {
            if (pstate)
            {
                Application& app = application_get_active();
                app.is_running = false;
            }
        }
        return 0;
        
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        
        case WM_SIZE:
        {
            if (pstate)
            {
                s32 width = LOWORD(lParam);
                s32 height = HIWORD(lParam);

                application_window_resize_callback(width, height);
                graphics_resize_canvas_callback(width, height);
            }
        } return 0;
        
        case WM_MOVE:
        {
            if (pstate)
            {
                s32 x = LOWORD(lParam);
                s32 y = HIWORD(lParam);

                application_window_move_callback(x, y);
            }
        } return 0;

        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYUP:
        {
            bool pressed = msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN;
            Key key = (Key) wParam;
            input_process_key(key, pressed);

            // TODO: Handle Left and Right versions of shift, control, alt, etc...
        } break;

        case WM_MOUSEWHEEL:
        {
            s32 z_delta = GET_WHEEL_DELTA_WPARAM(wParam);

            if (z_delta != 0)
            {
                // Flatten the input to an OS-independent (-1, 1)
                z_delta = (z_delta < 0) ? -1 : 1;
                input_process_mouse_wheel(z_delta);
            }
        } break;

        case WM_LBUTTONDOWN:
        case WM_LBUTTONDBLCLK:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONDBLCLK:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONDBLCLK:
        case WM_LBUTTONUP:
        case WM_MBUTTONUP:
        case WM_RBUTTONUP:
        {
            bool pressed = msg == WM_LBUTTONDOWN || msg == WM_LBUTTONDBLCLK ||
                           msg == WM_MBUTTONDOWN || msg == WM_MBUTTONDBLCLK ||
                           msg == WM_RBUTTONDOWN || msg == WM_RBUTTONDBLCLK;
            MouseButton btn = MouseButton::NUM_BUTTONS;

            switch (msg)
            {
                case WM_LBUTTONDOWN:
                case WM_LBUTTONUP:
                case WM_LBUTTONDBLCLK:
                {
                    btn = MouseButton::LEFT;
                } break;

                case WM_MBUTTONDOWN:
                case WM_MBUTTONUP:
                case WM_MBUTTONDBLCLK:
                {
                    btn = MouseButton::MIDDLE;
                } break;

                case WM_RBUTTONDOWN:
                case WM_RBUTTONUP:
                case WM_RBUTTONDBLCLK:
                {
                    btn = MouseButton::RIGHT;
                } break;
            }

            input_process_mouse_button(btn, pressed);
        } break;

        case WM_KILLFOCUS:
        case WM_SETFOCUS:
        {
            Application& app = application_get_active();
            app.window.has_focus = msg == WM_SETFOCUS;
        } break;
    }

    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

void platform_get_mouse_position(s32& x, s32& y)
{
    POINT point;
    GetCursorPos(&point);

    x = point.x;
    y = point.y;
}

void platform_set_mouse_position(s32 x, s32 y)
{
    SetCursorPos(x, y);
}

void platform_show_mouse_cursor(bool value)
{
    ShowCursor(value);
}

bool platform_dialogue_open_file(const char filter[], char* out_filepath, u32 max_path_size)
{
    OPENFILENAME ofn {};
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.lpstrFilter = filter;
    ofn.lpstrFile   = out_filepath;
    ofn.nMaxFile    = max_path_size;
    ofn.hwndOwner   = g_pstate->internal_state->hwnd;
    ofn.Flags       = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    ofn.lpstrTitle  = "Select Wallpaper";

    if (GetOpenFileName(&ofn))
        return true;

    return false;
}

#endif // GN_PLATFORM_WINDOWS