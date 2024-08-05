#include "vulkan_backend.h"
#include "vulkan_defines.h"
#include "core/log.h"

static VulkanContext context = {0};

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
    create_info.enabledLayerCount = 0;
    create_info.ppEnabledLayerNames = NULL;
    create_info.enabledExtensionCount = 0;
    create_info.ppEnabledExtensionNames = NULL;

    VkResult result = vkCreateInstance(&create_info, context.allocator, &context.instance);
    if (result != VK_SUCCESS)
    {
        log_error("vkCreateInstance: Failed to create Vulkan instance. Result: %u", result);
        return false;
    }

    log_info("Vulkan renderer initialized successfully.");
    return true;
}

void vulkan_renderer_backend_shutdown(RendererBackend* backend)
{

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