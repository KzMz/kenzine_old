#include "vulkan_backend.h"
#include "vulkan_defines.h"
#include "core/log.h"
#include "core/memory.h"
#include "lib/string.h"
#include "lib/containers/dyn_array.h"
#include "platform/platform.h"
#include "core/app.h"

#include "vulkan_platform.h"
#include "vulkan_device.h"
#include "vulkan_swapchain.h"
#include "vulkan_renderpass.h"
#include "vulkan_command_buffer.h"
#include "vulkan_framebuffer.h"
#include "vulkan_fence.h"

#define MIN_FRAMEBUFFER_WIDTH 800
#define MIN_FRAMEBUFFER_HEIGHT 600

static VulkanContext context = {0};
static u32 raw_framebuffer_width = 0;
static u32 raw_framebuffer_height = 0;

VKAPI_ATTR VkBool32 VKAPI_CALL vulkan_debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_type,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* user_data);

i32 find_memory_index(u32 type_filter, u32 property_flags);

void create_command_buffers(RendererBackend* backend);
void destroy_command_buffers(RendererBackend* backend);

void regenerate_framebuffers(RendererBackend* backend, VulkanSwapchain* swapchain, VulkanRenderPass* render_pass);
void destroy_framebuffers(RendererBackend* backend, VulkanSwapchain* swapchain);

void create_sync_objects(RendererBackend* backend);
void destroy_sync_objects(RendererBackend* backend);

bool vulkan_renderer_backend_init(RendererBackend* backend, const char* app_name, struct Platform* platform)
{
    context.find_memory_index = find_memory_index;

    context.allocator = NULL;

    app_get_framebuffer_size(&raw_framebuffer_width, &raw_framebuffer_height);
    context.framebuffer_width = raw_framebuffer_width != 0 ? raw_framebuffer_width : MIN_FRAMEBUFFER_WIDTH;
    context.framebuffer_height = raw_framebuffer_height != 0 ? raw_framebuffer_height : MIN_FRAMEBUFFER_HEIGHT;
    raw_framebuffer_width = 0;
    raw_framebuffer_height = 0;

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

    vulkan_swapchain_create(&context, context.framebuffer_width, context.framebuffer_height, &context.swapchain);

    vulkan_renderpass_create(
        &context, &context.main_render_pass,
        0, 0, context.framebuffer_width, context.framebuffer_height,
        0.0f, 0.0f, 0.2f, 1.0f,
        1.0f,
        0);

    context.swapchain.framebuffers = dynarray_reserve(VulkanFramebuffer, context.swapchain.image_count);
    regenerate_framebuffers(backend, &context.swapchain, &context.main_render_pass);

    create_command_buffers(backend);

    create_sync_objects(backend);

    log_info("Vulkan renderer initialized successfully.");
    return true;
}

void vulkan_renderer_backend_shutdown(RendererBackend* backend)
{
    vkDeviceWaitIdle(context.device.logical_device);

    destroy_sync_objects(backend);

    destroy_command_buffers(backend);

    destroy_framebuffers(backend, &context.swapchain);

    vulkan_renderpass_destroy(&context, &context.main_render_pass);

    vulkan_swapchain_destroy(&context, &context.swapchain);

    vulkan_device_destroy(&context);

    if (context.surface) 
    {
        log_debug("Destroying Vulkan surface...");
        vkDestroySurfaceKHR(context.instance, context.surface, context.allocator);
        context.surface = VK_NULL_HANDLE;
    }

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

i32 find_memory_index(u32 type_filter, u32 property_flags)
{
    VkPhysicalDeviceMemoryProperties memory_properties;
    vkGetPhysicalDeviceMemoryProperties(context.device.physical_device, &memory_properties);

    for (u32 i = 0; i < memory_properties.memoryTypeCount; ++i)
    {
        if ((type_filter & (1 << i)) && (memory_properties.memoryTypes[i].propertyFlags & property_flags) == property_flags)
        {
            return i;
        }
    }

    return -1;
}

void create_command_buffers(RendererBackend* backend)
{
    // one for each swapchain image
    if (!context.graphics_command_buffers)
    {
        context.graphics_command_buffers = dynarray_reserve(VulkanCommandBuffer, context.swapchain.image_count);
        for (u32 i = 0; i < context.swapchain.image_count; ++i)
        {
            memory_zero(&context.graphics_command_buffers[i], sizeof(VulkanCommandBuffer));
        }
    }

    for (u32 i = 0; i < context.swapchain.image_count; ++i)
    {
        if (context.graphics_command_buffers[i].command_buffer)
        {
            vulkan_command_buffer_free(&context, context.device.graphics_command_pool, &context.graphics_command_buffers[i]);
        }

        memory_zero(&context.graphics_command_buffers[i], sizeof(VulkanCommandBuffer));
        vulkan_command_buffer_alloc(&context, context.device.graphics_command_pool, true, &context.graphics_command_buffers[i]);
    }
}

void destroy_command_buffers(RendererBackend* backend)
{
    for (u32 i = 0; i < context.swapchain.image_count; ++i)
    {
        vulkan_command_buffer_free(&context, context.device.graphics_command_pool, &context.graphics_command_buffers[i]);
    }

    dynarray_destroy(context.graphics_command_buffers);
    context.graphics_command_buffers = NULL;
}

void regenerate_framebuffers(RendererBackend* backend, VulkanSwapchain* swapchain, VulkanRenderPass* render_pass)
{
    for (u32 i = 0; i < swapchain->image_count; ++i)
    {
        if (swapchain->framebuffers[i].framebuffer)
        {
            vulkan_framebuffer_destroy(&context, &swapchain->framebuffers[i]);
        }

        u32 attachment_count = 2;
        VkImageView attachments[2] = {swapchain->image_views[i], swapchain->depth_attachment.view};

        vulkan_framebuffer_create(
            &context, render_pass,
            context.framebuffer_width, context.framebuffer_height,
            attachment_count, attachments,
            &swapchain->framebuffers[i]);
    }
}

void destroy_framebuffers(RendererBackend* backend, VulkanSwapchain* swapchain)
{
    for (u32 i = 0; i < swapchain->image_count; ++i)
    {
        vulkan_framebuffer_destroy(&context, &swapchain->framebuffers[i]);
    }
}

void create_sync_objects(RendererBackend* backend)
{
    context.image_available_semaphores = dynarray_reserve(VkSemaphore, context.swapchain.max_frames_in_flight);
    context.queue_complete_semaphores = dynarray_reserve(VkSemaphore, context.swapchain.max_frames_in_flight);
    context.in_flight_fences = dynarray_reserve(VulkanFence, context.swapchain.max_frames_in_flight);

    for (u8 i = 0; i < context.swapchain.max_frames_in_flight; ++i)
    {
        VkSemaphoreCreateInfo semaphore_create_info = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        VK_CHECK(vkCreateSemaphore(context.device.logical_device, &semaphore_create_info, context.allocator, &context.image_available_semaphores[i]));
        VK_CHECK(vkCreateSemaphore(context.device.logical_device, &semaphore_create_info, context.allocator, &context.queue_complete_semaphores[i]));

        vulkan_fence_create(&context, true, &context.in_flight_fences[i]);
    }

    // check if we need VulkanFence* here
    context.images_in_flight = dynarray_reserve(VulkanFence, context.swapchain.image_count);
    for (u32 i = 0; i < context.swapchain.image_count; ++i)
    {
        context.images_in_flight[i] = NULL;
    }
}

void destroy_sync_objects(RendererBackend* backend)
{
    for (u8 i = 0; i < context.swapchain.max_frames_in_flight; ++i)
    {
        if (context.image_available_semaphores[i] != VK_NULL_HANDLE)
        {
            vkDestroySemaphore(context.device.logical_device, context.image_available_semaphores[i], context.allocator);
        }
        
        if (context.queue_complete_semaphores[i] != VK_NULL_HANDLE)
        {
            vkDestroySemaphore(context.device.logical_device, context.queue_complete_semaphores[i], context.allocator);
        }

        vulkan_fence_destroy(&context, &context.in_flight_fences[i]);
    }

    dynarray_destroy(context.image_available_semaphores);
    context.image_available_semaphores = NULL;

    dynarray_destroy(context.queue_complete_semaphores);
    context.queue_complete_semaphores = NULL;

    dynarray_destroy(context.in_flight_fences);
    context.in_flight_fences = NULL;

    dynarray_destroy(context.images_in_flight);
    context.images_in_flight = NULL;
}