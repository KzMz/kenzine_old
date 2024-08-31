#include "platform.h"

#if KZ_PLATFORM_WINDOWS == 1

#include "core/log.h"
#include "core/input/input.h"
#include "lib/containers/dyn_array.h"
#include "renderer/vulkan/vulkan_defines.h"
#include "core/event.h"
#include "core/memory.h"

#include <windows.h>
#include <windowsx.h>
#include <vulkan/vulkan_win32.h>
#include <hidsdi.h>
#include <stdio.h>
#include <Xinput.h>

typedef struct PlatformState
{
    HINSTANCE h_instance;
    HWND h_window;
    VkSurfaceKHR surface;
} PlatformState;

static PlatformState* platform_state = 0;

// Clock
static f64 clock_frequency = 0.0;
static LARGE_INTEGER start_time = {0};

LRESULT CALLBACK win32_process_message(HWND window, u32 msg, WPARAM w_param, LPARAM l_param);

u64 platform_get_state_size(void)
{
    return sizeof(PlatformState);
}

void clock_setup(void)
{
    LARGE_INTEGER frequency = {0};
    QueryPerformanceFrequency(&frequency);
    clock_frequency = 1.0 / (f64) frequency.QuadPart;
    QueryPerformanceCounter(&start_time);
}

bool platform_init(void* state, const char* app_name, i32 width, i32 height, i32 x, i32 y)
{
    platform_state = (PlatformState*) state;
    platform_state->h_instance = GetModuleHandleA(NULL);

    HICON h_icon = LoadIcon(platform_state->h_instance, IDI_APPLICATION);
    WNDCLASSA wc = {0};
    wc.style = CS_DBLCLKS;
    wc.lpfnWndProc = win32_process_message;
    wc.hInstance = platform_state->h_instance;
    wc.hIcon = h_icon;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;
    wc.lpszClassName = "KenzineWindowClass";

    if (!RegisterClassA(&wc))
    {
        MessageBoxA(NULL, "Failed to register window class.", "Error", MB_OK | MB_ICONERROR);
        log_fatal("Failed to register window class.");
        return false;
    }

    u32 window_x = x;
    u32 window_y = y;
    u32 window_width = width;
    u32 window_height = height;
    u32 window_style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU;
    u32 window_ex_style = WS_EX_APPWINDOW;

    window_style |= WS_MAXIMIZEBOX;
    window_style |= WS_MINIMIZEBOX;
    window_style |= WS_THICKFRAME;

    RECT window_rect = {0};
    AdjustWindowRectEx(&window_rect, window_style, FALSE, window_ex_style);

    window_x += window_rect.left;
    window_y += window_rect.top;
    window_width += window_rect.right - window_rect.left;
    window_height += window_rect.bottom - window_rect.top;

    HWND handle = CreateWindowExA(
        window_ex_style, wc.lpszClassName, app_name, 
        window_style, window_x, window_y, window_width, window_height, 
        NULL, NULL, platform_state->h_instance, NULL);

    if (handle == 0)
    {
        MessageBoxA(NULL, "Failed to create window.", "Error", MB_OK | MB_ICONERROR);
        log_fatal("Failed to create window.");
        return false;
    }

    platform_state->h_window = handle;

    bool should_activate = true;
    i32 show_command = should_activate ? SW_SHOW : SW_SHOWNOACTIVATE;
    ShowWindow(platform_state->h_window, show_command);

    clock_setup();   

    // TODO: for testing
    platform_register_hid_device(); 
    return true;
}

void platform_shutdown()
{
    if (platform_state == NULL)
    {
        return;
    }

    if (platform_state->h_window != NULL)
    {
        DestroyWindow(platform_state->h_window);
        platform_state->h_window = NULL;
    }

    platform_state = NULL;
}

bool platform_handle_messages()
{
    if (platform_state == NULL)
    {
        return false;
    }

    MSG message = {0};
    while (PeekMessageA(&message, NULL, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&message);
        DispatchMessageA(&message);
    }

    return true;
}

void* platform_alloc(u64 size, bool aligned)
{
    (void) aligned;
    return malloc(size);
}

void platform_free(void* block, bool aligned)
{
    (void) aligned;
    free(block);
}

void* platform_zero_memory(void* block, u64 size)
{
    return memset(block, 0, size);
}

void* platform_copy_memory(void* dest, const void* source, u64 size)
{
    return memcpy(dest, source, size);
}

void* platform_set_memory(void* dest, i32 value, u64 size)
{
    return memset(dest, value, size);
}

void platform_console_write(const char* message, LogLevel level)
{
    HANDLE console_handle = GetStdHandle(STD_OUTPUT_HANDLE);

    static u8 level_colors[6] = { 64, 4, 6, 2, 1, 8 };
    SetConsoleTextAttribute(console_handle, level_colors[level]);

    OutputDebugStringA(message);
    u64 message_length = strlen(message);
    LPDWORD written = 0;
    WriteConsoleA(console_handle, message, message_length, written, NULL);
}

void platform_console_write_error(const char* message, LogLevel level)
{
    HANDLE console_handle = GetStdHandle(STD_ERROR_HANDLE);

    static u8 level_colors[6] = { 64, 4, 6, 2, 1, 8 };
    SetConsoleTextAttribute(console_handle, level_colors[level]);

    OutputDebugStringA(message);
    u64 message_length = strlen(message);
    LPDWORD written = 0;
    WriteConsoleA(console_handle, message, message_length, written, NULL);
}

f64  platform_get_absolute_time(void)
{
    if (!clock_frequency)
    {
        clock_setup();
    }

    LARGE_INTEGER now = {0};
    QueryPerformanceCounter(&now);
    return (f64) (now.QuadPart) * clock_frequency;
}

void platform_sleep(u64 ms)
{
    Sleep(ms);
}

void platform_get_required_extension_names(const char*** extension_names)
{
    dynarray_push(*extension_names, &"VK_KHR_win32_surface");
}

bool platform_create_vulkan_surface(VulkanContext* context)
{
    if (platform_state == NULL)
    {
        return false;
    }

    VkWin32SurfaceCreateInfoKHR surface_create_info = {VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR};
    surface_create_info.hinstance = platform_state->h_instance;
    surface_create_info.hwnd = platform_state->h_window;

    VK_ASSERT(vkCreateWin32SurfaceKHR(context->instance, &surface_create_info, context->allocator, &platform_state->surface));

    context->surface = platform_state->surface;
    return true;
}

void platform_create_hid_device(void* handle, PlatformHIDDevice* out_device)
{
    if (handle == NULL || out_device == NULL)
    {
        return;
    }

    out_device->device_handle = handle;
    HANDLE dhandle = (HANDLE) handle;

    WCHAR device_name[1024] = {0};
    u32 buffer_size = sizeof(device_name) / sizeof(*device_name);
    GetRawInputDeviceInfoW(dhandle, RIDI_DEVICENAME, device_name, &buffer_size);
    sprintf_s(out_device->name, sizeof(out_device->name), "%ws", device_name);

    out_device->output_handle = CreateFileW(device_name, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
    HidD_GetProductString(out_device->output_handle, out_device->product, sizeof(out_device->product) / sizeof(out_device->product[0]));
    HidD_GetManufacturerString(out_device->output_handle, out_device->manufacturer, sizeof(out_device->manufacturer) / sizeof(out_device->manufacturer[0]));
    HidD_GetSerialNumberString(out_device->output_handle, out_device->serial_number, sizeof(out_device->serial_number) / sizeof(out_device->serial_number[0]));

    RID_DEVICE_INFO device_info = {0};
    u32 device_info_size = sizeof(device_info);
    if (GetRawInputDeviceInfo(dhandle, RIDI_DEVICEINFO, &device_info, &device_info_size) > 0)
    {
        out_device->product_id = device_info.hid.dwProductId;
        out_device->vendor_id = device_info.hid.dwVendorId;
    }
}

bool platform_register_hid_device(void)
{
    if (platform_state == NULL)
    {
        return false;
    }

    RAWINPUTDEVICE device_list[2] = {0};
    device_list[0].usUsagePage = HID_USAGE_PAGE_GENERIC;
    device_list[0].usUsage = HID_USAGE_GENERIC_GAMEPAD;
    device_list[0].dwFlags = RIDEV_DEVNOTIFY | RIDEV_INPUTSINK;
    device_list[0].hwndTarget = platform_state->h_window;

    device_list[1].usUsagePage = HID_USAGE_PAGE_GENERIC;
    device_list[1].usUsage = HID_USAGE_GENERIC_JOYSTICK;
    device_list[1].dwFlags = RIDEV_DEVNOTIFY | RIDEV_INPUTSINK;
    device_list[1].hwndTarget = platform_state->h_window;
    RegisterRawInputDevices(device_list, 2, sizeof(RAWINPUTDEVICE));
    return true;
}

void platform_destroy_hid_device(PlatformHIDDevice* device)
{
    if (device == NULL)
    {
        return;
    }

    if (device->output_handle == INVALID_HANDLE_VALUE)
    {
        return;
    }

    CloseHandle(device->output_handle);
    memory_zero(device, sizeof(PlatformHIDDevice));
    device->output_handle = INVALID_HANDLE_VALUE;
}

// XBOX Gamepad
#define XINPUT_MAX_TRIGGER 255
#define XINPUT_MAX_THUMB 32767
#define XINPUT_MAX_VIBRATION 65535

bool platform_gamepad_xbox_set_vibration(u32 sub_id, GamepadVibration vibration)
{
    XINPUT_VIBRATION xinput_vibration = {0};
    xinput_vibration.wLeftMotorSpeed = (u16) (vibration.left_motor * XINPUT_MAX_VIBRATION);
    xinput_vibration.wRightMotorSpeed = (u16) (vibration.right_motor * XINPUT_MAX_VIBRATION);

    DWORD result = XInputSetState(sub_id, &xinput_vibration);
    if (result != ERROR_SUCCESS)
    {
        log_warning("Failed to set gamepad vibration. ID: %d Error code: %d", sub_id, result);
        return false;
    }

    return true;
}

void* platform_gamepad_xbox_get_current_state(u32 sub_id)
{
    GamepadState* state = (GamepadState*) gamepad_get_current_state(sub_id);

    XINPUT_STATE xinput_state = {0};
    DWORD result = XInputGetState(sub_id, &xinput_state);
    if (result == ERROR_SUCCESS)
    {
        state->connected = true;
        
        state->buttons[GAMEPAD_BUTTON_FACE_BOTTOM] = (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_A) != 0;
        state->buttons[GAMEPAD_BUTTON_FACE_RIGHT] = (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_B) != 0;
        state->buttons[GAMEPAD_BUTTON_FACE_LEFT] = (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_X) != 0;
        state->buttons[GAMEPAD_BUTTON_FACE_TOP] = (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_Y) != 0;
        state->buttons[GAMEPAD_BUTTON_SHOULDER_LEFT] = (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) != 0;
        state->buttons[GAMEPAD_BUTTON_SHOULDER_RIGHT] = (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) != 0;
        state->buttons[GAMEPAD_BUTTON_THUMB_LEFT] = (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB) != 0;
        state->buttons[GAMEPAD_BUTTON_THUMB_RIGHT] = (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB) != 0;
        state->buttons[GAMEPAD_BUTTON_DPAD_UP] = (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP) != 0;
        state->buttons[GAMEPAD_BUTTON_DPAD_RIGHT] = (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) != 0;
        state->buttons[GAMEPAD_BUTTON_DPAD_DOWN] = (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) != 0;
        state->buttons[GAMEPAD_BUTTON_DPAD_LEFT] = (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) != 0; 
        state->buttons[GAMEPAD_BUTTON_START] = (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_START) != 0;
        state->buttons[GAMEPAD_BUTTON_BACK] = (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_BACK) != 0;

        state->axes[GAMEPAD_AXIS_LEFT_THUMB_X] = (f32) xinput_state.Gamepad.sThumbLX / XINPUT_MAX_THUMB;
        state->axes[GAMEPAD_AXIS_LEFT_THUMB_Y] = (f32) xinput_state.Gamepad.sThumbLY / XINPUT_MAX_THUMB;
        state->axes[GAMEPAD_AXIS_RIGHT_THUMB_X] = (f32) xinput_state.Gamepad.sThumbRX / XINPUT_MAX_THUMB;
        state->axes[GAMEPAD_AXIS_RIGHT_THUMB_Y] = (f32) xinput_state.Gamepad.sThumbRY / XINPUT_MAX_THUMB;
        state->axes[GAMEPAD_AXIS_TRIGGER_LEFT] = (f32) xinput_state.Gamepad.bLeftTrigger / XINPUT_MAX_TRIGGER;
        state->axes[GAMEPAD_AXIS_TRIGGER_RIGHT] = (f32) xinput_state.Gamepad.bRightTrigger / XINPUT_MAX_TRIGGER;
    }
    else
    {
        state->connected = false;
        log_warning("Failed to get gamepad state. ID: %d Error code: %d", sub_id, result);
    }

    return state;
}

LRESULT CALLBACK win32_process_message(HWND window, u32 msg, WPARAM w_param, LPARAM l_param)
{
    switch (msg)
    {
        case WM_ERASEBKGND:
            return 1; // Prevent flickering
        case WM_CLOSE:
            EventContext event = {0};
            event_trigger(EVENT_CODE_APPLICATION_QUIT, 0, event);
            return TRUE;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        case WM_SIZE:
        {
            RECT client_rect = {0};
            GetClientRect(window, &client_rect);
            i32 width = client_rect.right - client_rect.left;
            i32 height = client_rect.bottom - client_rect.top;

            EventContext event = {0};
            event.data.u32[0] = width;
            event.data.u32[1] = height;
            event_trigger(EVENT_CODE_RESIZED, 0, event);
        } break;
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYUP: 
        {
            bool pressed = (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN);
            KeyboardKeys key = (u16) w_param;

            bool extended = (HIWORD(l_param) & KF_EXTENDED) == KF_EXTENDED;

            if (w_param == VK_MENU)
            {
                key = extended ? KEY_RALT : KEY_LALT;
            } 
            else if (w_param == VK_SHIFT)
            {
                u32 left_shift = MapVirtualKey(VK_LSHIFT, MAPVK_VK_TO_VSC);
                u32 scancode = (u32) ((l_param & (0xFF << 16)) >> 16);
                key = scancode == left_shift ? KEY_LSHIFT : KEY_RSHIFT;
            } 
            else if (w_param == VK_CONTROL)
            {
                key = extended ? KEY_RCONTROL : KEY_LCONTROL;
            } 

            input_process_key(KEYBOARD_DEVICE_ID, 0, key, pressed);
            return 0;
        }
        case WM_MOUSEMOVE: 
        {
            i32 x = GET_X_LPARAM(l_param);
            i32 y = GET_Y_LPARAM(l_param);

            mouse_process_mouse_move(0, x, y);
        } break;
        case WM_MOUSEWHEEL: 
        {
            i32 delta = GET_WHEEL_DELTA_WPARAM(w_param);
            if (delta != 0)
            {
                // flatten input
                delta = (delta > 0) ? 1 : -1;
                mouse_process_mouse_wheel(0, delta);
            }
        } break;
        case WM_LBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_MBUTTONUP:
        case WM_RBUTTONUP:
        {
            bool pressed = (msg == WM_LBUTTONDOWN || msg == WM_MBUTTONDOWN || msg == WM_RBUTTONDOWN);
            MouseButton button = MOUSE_BUTTON_COUNT;
            switch (msg)
            {
                case WM_LBUTTONDOWN:
                case WM_LBUTTONUP:
                    button = MOUSE_BUTTON_LEFT;
                    break;
                case WM_MBUTTONDOWN:
                case WM_MBUTTONUP:
                    button = MOUSE_BUTTON_MIDDLE;
                    break;
                case WM_RBUTTONDOWN:
                case WM_RBUTTONUP:
                    button = MOUSE_BUTTON_RIGHT;
                    break;
            }

            if (button != MOUSE_BUTTON_COUNT)
            {
                input_process_key(MOUSE_DEVICE_ID, 0, button, pressed);
            }
        } break;
        case WM_INPUT_DEVICE_CHANGE:
        {
            PlatformHIDDevice device = {0};
            if (w_param == GIDC_ARRIVAL)
            {
                input_on_connected(GAMEPAD_DEVICE_ID, (void*) l_param);

                EventContext event = {0};
                event.data.u64[0] = l_param;
                event_trigger(EVENT_CODE_HID_DEVICE_CONNECTED, 0, event);
            }
            else if (w_param == GIDC_REMOVAL)
            {
                input_on_disconnected((void*) l_param);

                EventContext event = {0};
                event.data.u64[0] = l_param;
                event_trigger(EVENT_CODE_HID_DEVICE_DISCONNECTED, 0, event);
            }
        } break;
    }

    return DefWindowProcA(window, msg, w_param, l_param);
}

#endif // KZ_PLATFORM_WINDOWS