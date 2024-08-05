#include "vulkan_backend.h"
#include "vulkan_defines.h"
#include "core/log.h"
#include "lib/string.h"
#include "lib/containers/dyn_array.h"
#include "platform/platform.h"
#include "vulkan_platform.h"
#include "vulkan_device.h"

static VulkanContext context = {0};

VKAPI_ATTR VkBool32 VKAPI_CALL vulkan_debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_type,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* user_data);

bool vulkan_renderer_backend_init(RendererBackend* backend, const char* app_name, struct Platform* platform)
{
    context.allocator = NULL;

    VkApplicationInfo app_info = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
    app_info.apiVersion = VK_API_VERSION_1_2;
    app_info.pApplicationName = app_name;
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "Kenzine";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);

    VkInstanceCreateInfo create_info = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    create_info.pApplicationInfo = &app_info;

    const char** required_extentions = dynarray_create(const char*);
    dynarray_push(required_extentions, &VK_KHR_SURFACE_EXTENSION_NAME);
    platform_get_required_extension_names(&required_extentions);

#if defined(_DEBUG)
    dynarray_push(required_extentions, &VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    log_debug("Required Vulkan extensions:");
    for (u64 i = 0; i < dynarray_length(required_extentions); ++i)
    {
        log_debug("  %s", required_extentions[i]);
    }
#endif

    create_info.enabledExtensionCount = dynarray_length(required_extentions);
    create_info.ppEnabledExtensionNames = required_extentions;

    const char** required_validation_layer_names = 0;
    u32 required_validation_layer_count = 0;

#if defined(_DEBUG)
    log_debug("Validation layers enabled. Enumerating...");

    required_validation_layer_names = dynarray_create(const char*);
    dynarray_push(required_validation_layer_names, &"VK_LAYER_KHRONOS_validation");
    required_validation_layer_count = dynarray_length(required_validation_layer_names);

    u32 available_layer_count = 0;
    VK_CHECK(vkEnumerateInstanceLayerProperties(&available_layer_count, NULL));
    VkLayerProperties* available_layers = dynarray_reserve(VkLayerProperties, available_layer_count);
    VK_CHECK(vkEnumerateInstanceLayerProperties(&available_layer_count, available_layers));

    for (u32 i = 0; i < required_validation_layer_count; ++i)
    {
        log_debug("Searching for layer %s...", (required_validation_layer_names[i]));
        bool found = false;
        for (u32 j = 0; j < available_layer_count; ++j)
        {
            if (!string_equals(required_validation_layer_names[i], available_layers[j].layerName))
            {
                continue;
            }

            found = true;
            log_debug("Found.");
            break;
        }

        if (!found)
        {
            log_fatal("Validation layer %s not found.", required_validation_layer_names[i]);
            return false;
        }
    }
    log_debug("Validation layers found.");
#endif

    create_info.enabledLayerCount = required_validation_layer_count;
    create_info.ppEnabledLayerNames = required_validation_layer_names;

    VK_CHECK(vkCreateInstance(&create_info, context.allocator, &context.instance));
    log_info("Vulkan instance created.");

#if defined(_DEBUG)
    log_debug("Creating Vulkan debugger...");
    u32 log_severity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;

    VkDebugUtilsMessengerCreateInfoEXT debug_create_info = {VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
    debug_create_info.messageSeverity = log_severity;
    debug_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debug_create_info.pfnUserCallback = vulkan_debug_callback;

    PFN_vkCreateDebugUtilsMessengerEXT func = 
        (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(context.instance, "vkCreateDebugUtilsMessengerEXT");
    kz_assert_msg(func, "Failed to get vkCreateDebugUtilsMessengerEXT function pointer.");
    VK_CHECK(func(context.instance, &debug_create_info, context.allocator, &context.debug_messenger));
    log_debug("Vulkan debugger created.");
#endif

    log_debug("Creating Vulkan surface...");
    if (!platform_create_vulkan_surface(platform, &context))
    {
        log_fatal("Failed to create Vulkan surface.");
        return false;
    }
    log_debug("Vulkan surface created.");

    if (!vulkan_device_create(&context))
    {
        log_fatal("Failed to create Vulkan device.");
        return false;
    }

    log_info("Vulkan renderer initialized successfully.");
    return true;
}

void vulkan_renderer_backend_shutdown(RendererBackend* backend)
{
    vulkan_device_destroy(&context);

#if defined(_DEBUG)
    log_debug("Destroying Vulkan debugger...");
    if (context.debug_messenger)
    {
        PFN_vkDestroyDebugUtilsMessengerEXT func = 
            (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(context.instance, "vkDestroyDebugUtilsMessengerEXT");
        kz_assert_msg(func, "Failed to get vkDestroyDebugUtilsMessengerEXT function pointer.");
        func(context.instance, context.debug_messenger, context.allocator);
    }
#endif

    log_debug("Destroying Vulkan instance...");
    vkDestroyInstance(context.instance, context.allocator);
    log_info("Vulkan renderer shutdown.");
}

bool vulkan_renderer_backend_begin_frame(RendererBackend* backend, f64 delta_time)
{
    return true;
}

bool vulkan_renderer_backend_end_frame(RendererBackend* backend, f64 delta_time)
{
    return true;
}

void vulkan_renderer_backend_resize(RendererBackend* backend, i32 width, i32 height)
{

}

VKAPI_ATTR VkBool32 VKAPI_CALL vulkan_debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_type,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* user_data)
{
    switch (message_severity)
    {
        default:
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            log_error(callback_data->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            log_warning(callback_data->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            log_info(callback_data->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            log_trace(callback_data->pMessage);
            break;    
    }
    return VK_FALSE;   
}