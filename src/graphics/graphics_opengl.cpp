#include "graphics.h"

#ifdef GN_USE_OPENGL

#ifdef GN_PLATFORM_WINDOWS

#include "platform/platform.h"
#include "platform/internal/internal_win32.h"
#include "core/types.h"
#include "core/logger.h"
#include <windows.h>

// To load opengl functions
#include <glad/glad.h>
#include <wglext.h>

typedef BOOL (*SwapIntervalFunction)(int interval);
static SwapIntervalFunction internal_set_swap_interval;

static bool gl_initialized = false;

static void* gl_get_proc_address(const char* name)
{
    void* ptr = (void*) wglGetProcAddress(name);

    if (ptr == nullptr || ptr == (void*) 0x1 ||
        ptr == (void*) 0x2 || ptr == (void*) 0x3 ||
        ptr == (void*) -1)
    {
        HMODULE module = LoadLibraryA("opengl32.dll");
        ptr = (void*) GetProcAddress(module, name);
    }

    return ptr;
}

#ifdef GN_DEBUG
static void APIENTRY gl_debug_output(GLenum source, GLenum type, unsigned int id, GLenum severity,
                                   GLsizei length, const char* message, const void* user_param)
{

    // ignore non-significant error/warning codes
    if(id == 131169 || id == 131185 || id == 131218 || id == 131204) return; 

    print_error("GL Debug Message (%): %\n", id, message);

    switch (source)
    {
        case GL_DEBUG_SOURCE_API:             print_error("Source: API\n"); break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   print_error("Source: Window System\n"); break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER: print_error("Source: Shader Compiler\n"); break;
        case GL_DEBUG_SOURCE_THIRD_PARTY:     print_error("Source: Third Party\n"); break;
        case GL_DEBUG_SOURCE_APPLICATION:     print_error("Source: Application\n"); break;
        case GL_DEBUG_SOURCE_OTHER:           print_error("Source: Other\n"); break;
    }

    switch (type)
    {
        case GL_DEBUG_TYPE_ERROR:               print_error("Type: Error\n"); break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: print_error("Type: Deprecated Behaviour\n"); break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  print_error("Type: Undefined Behaviour\n"); break;
        case GL_DEBUG_TYPE_PORTABILITY:         print_error("Type: Portability\n"); break;
        case GL_DEBUG_TYPE_PERFORMANCE:         print_error("Type: Performance\n"); break;
        case GL_DEBUG_TYPE_MARKER:              print_error("Type: Marker\n"); break;
        case GL_DEBUG_TYPE_PUSH_GROUP:          print_error("Type: Push Group\n"); break;
        case GL_DEBUG_TYPE_POP_GROUP:           print_error("Type: Pop Group\n"); break;
        case GL_DEBUG_TYPE_OTHER:               print_error("Type: Other\n"); break;
    }
    
    switch (severity)
    {
        case GL_DEBUG_SEVERITY_HIGH:         print_error("Severity: high\n"); break;
        case GL_DEBUG_SEVERITY_MEDIUM:       print_error("Severity: medium\n"); break;
        case GL_DEBUG_SEVERITY_LOW:          print_error("Severity: low\n"); break;
        case GL_DEBUG_SEVERITY_NOTIFICATION: print_error("Severity: notification\n"); break;
    }

    gn_break_point();
}
#endif // GN_DEBUG

bool graphics_init(InternalState& state)
{
    PIXELFORMATDESCRIPTOR pfd = {};
    pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;     // TODO: Maybe some way of setting this?
    pfd.cColorBits = 32;                // TODO: Maybe some way of setting this?
    pfd.cDepthBits = 16;                // TODO: Maybe some way of setting this?
    pfd.iLayerType = PFD_MAIN_PLANE;

    state.hdc = GetDC(state.hwnd);
    
    if (!state.hdc)
    {
        MessageBoxA(state.hwnd, "Couldn't create device context for OpenGL!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        return false;
    }
    
    // Choose pixel format with the desired settings
    GLuint pixel_format = ChoosePixelFormat(state.hdc, &pfd);
    if (!pixel_format)
    {
        MessageBoxA(state.hwnd, "Couldn't find a suitable pixel format!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        return false;
    }

    // Set the pixel format for the program
    if (!SetPixelFormat(state.hdc, pixel_format, &pfd))
    {
        MessageBoxA(state.hwnd, "Couldn't set the pixel format!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        return false;
    }

    // Create Rendering Context
    HGLRC hrc = wglCreateContext(state.hdc);
    if (!hrc)
    {
        MessageBoxA(state.hwnd, "Couldn't create rendering context for OpenGL!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        wglDeleteContext(hrc);
        return false;
    }

    // Activate the rendering context for OpenGL
    if (!wglMakeCurrent(state.hdc, hrc))
    {
        MessageBoxA(state.hwnd, "Couldn't activate the rendering context for OpenGL!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        wglDeleteContext(hrc);
        return false;
    }

    // Set load function for gl functions
    if (!gladLoadGLLoader((GLADloadproc) gl_get_proc_address))
    {
        MessageBoxA(state.hwnd, "Couldn't load OpenGL functions!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        wglDeleteContext(hrc);
        return false;
    }

    // Load opengl functions
    if (!gladLoadGL())
    {
        MessageBoxA(state.hwnd, "Couldn't load OpenGL functions!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        wglDeleteContext(hrc);
        return false;
    }

    internal_set_swap_interval= (SwapIntervalFunction) gl_get_proc_address("wglSwapIntervalEXT");

    
    // Initialize opengl with things like 4x MSAA
    PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC) gl_get_proc_address("wglChoosePixelFormatARB");
    PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC) gl_get_proc_address("wglCreateContextAttribsARB");

    if (!wglChoosePixelFormatARB || !wglCreateContextAttribsARB)
    {
        MessageBoxA(state.hwnd, "Couldn't find wglCreateContextAttribsARB function!", "Warning!", MB_ICONEXCLAMATION | MB_OK);
        return true;
    }

    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(hrc);

    int ipixel_attribs[] =
    {
        WGL_SUPPORT_OPENGL_ARB, GL_TRUE,    // Must support OGL rendering
        WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,    // pf that can run a window
        WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,    // must be HW accelerated
        WGL_DOUBLE_BUFFER_ARB, GL_TRUE,     // Double buffered context

        WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
        WGL_COLOR_BITS_ARB, 32,             // 8 bits of each R, G and B
        WGL_DEPTH_BITS_ARB, 16,             // 24 bits of depth precision for window
        WGL_STENCIL_BITS_ARB, 8, // Template buffer, template buffer number = 8

        WGL_SAMPLE_BUFFERS_ARB, GL_TRUE, // MSAA ON, open multiple samples
        WGL_SAMPLES_ARB, 4,              // ​​4X MSAA, multiple sample samples are 4

        0
    };

    pixel_format = -1;
    u32 pixel_count = 0;
    f32 fpixel_attribs[] = { 0, 0 };

    wglChoosePixelFormatARB(state.hdc, ipixel_attribs, fpixel_attribs, 1, (int*) &pixel_format, &pixel_count);
    if (pixel_format == -1)
    {
        MessageBoxA(state.hwnd, "Couldn't find a suitable pixel format!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        return false;
    }

#if GN_DEBUG
    s32 debugBit = WGL_CONTEXT_DEBUG_BIT_ARB;
#else
    s32 debugBit = 0;
#endif // GN_DEBUG
    
    s32 attribs[] =
    {
        WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
        WGL_CONTEXT_MINOR_VERSION_ARB, 5,
        WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB | debugBit,
        WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
        0
    };

    hrc = wglCreateContextAttribsARB(state.hdc, nullptr, attribs);
    if (!hrc)
    {
        MessageBoxA(state.hwnd, "Couldn't create rendering context for OpenGL!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        wglDeleteContext(hrc);
        return false;
    }

    if (!wglMakeCurrent(state.hdc, hrc))
    {
        MessageBoxA(state.hwnd, "Couldn't activate the rendering context for OpenGL!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        wglDeleteContext(hrc);
        return false;
    }

    print("GL Version: %\n", (const char*) glGetString(GL_VERSION));

    // Opengl Settings?
    glEnable(GL_MULTISAMPLE);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    glEnable(GL_DEPTH_TEST);

#ifdef GN_DEBUG
    // Setup debugging for opengl
    int flags; glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
    if (flags & GL_CONTEXT_FLAG_DEBUG_BIT)
    {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(gl_debug_output, nullptr);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE,
                                0, nullptr, GL_TRUE);

        print("[OpenGL] Ready to debug...\n");
    }
#endif // GN_DEBUG

    gl_initialized = true;
    return true;
}

void graphics_shutdown(InternalState& state)
{
    wglMakeCurrent(state.hdc, NULL);
    wglDeleteContext(wglGetCurrentContext());
}

void graphics_swap_buffers(const PlatformState& pstate)
{
    SwapBuffers(pstate.internal_state->hdc);
}

void graphics_resize_canvas_callback(s32 width, s32 height)
{
    if (!gl_initialized)
        return;

    glViewport(0, 0, width, height);
}

void graphics_set_vsync(bool value)
{
    internal_set_swap_interval((int) value);
}

void graphics_set_clear_color(f32 red, f32 green, f32 blue, f32 alpha)
{
    glClearColor(red, green, blue, alpha);
}

void graphics_clear_canvas()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

#endif // GN_PLATFORM_WINDOWS

#endif // GN_USE_OPENGL