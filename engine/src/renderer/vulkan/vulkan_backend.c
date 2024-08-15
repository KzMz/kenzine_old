#include "vulkan_backend.h"
#include "vulkan_defines.h"
#include "core/log.h"
#include "core/memory.h"
#include "lib/string.h"
#include "lib/containers/dyn_array.h"
#include "platform/platform.h"
#include "core/app.h"
#include "lib/math/math_defines.h"

#include "vulkan_platform.h"
#include "vulkan_device.h"
#include "vulkan_swapchain.h"
#include "vulkan_renderpass.h"
#include "vulkan_command_buffer.h"
#include "vulkan_framebuffer.h"
#include "vulkan_fence.h"
#include "vulkan_utils.h"
#include "vulkan_buffer.h"

#include "shaders/vulkan_obj_shader.h"

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

bool create_buffers(VulkanContext* context);
void destroy_buffers(VulkanContext* context);

void create_command_buffers(RendererBackend* backend);
void destroy_command_buffers(RendererBackend* backend);

void regenerate_framebuffers(RendererBackend* backend, VulkanSwapchain* swapchain, VulkanRenderPass* render_pass);
void destroy_framebuffers(RendererBackend* backend, VulkanSwapchain* swapchain);

void create_sync_objects(RendererBackend* backend);
void destroy_sync_objects(RendererBackend* backend);

bool recreate_swapchain(RendererBackend* backend);

void upload_data(VulkanContext* context, VkCommandPool pool, VkFence fence, VkQueue queue, VulkanBuffer* buffer, u64 offset, u64 size, void* data);

bool vulkan_renderer_backend_init(RendererBackend* backend, const char* app_name)
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
    VK_ASSERT(vkEnumerateInstanceLayerProperties(&available_layer_count, NULL));
    VkLayerProperties* available_layers = dynarray_reserve(VkLayerProperties, available_layer_count);
    VK_ASSERT(vkEnumerateInstanceLayerProperties(&available_layer_count, available_layers));

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

    VK_ASSERT(vkCreateInstance(&create_info, context.allocator, &context.instance));
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
    VK_ASSERT(func(context.instance, &debug_create_info, context.allocator, &context.debug_messenger));
    log_debug("Vulkan debugger created.");
#endif

    log_debug("Creating Vulkan surface...");
    if (!platform_create_vulkan_surface(&context))
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

    if (!vulkan_obj_shader_create(&context, &context.obj_shader))
    {
        log_fatal("Failed to create object shader.");
        return false;
    }

    create_buffers(&context);

    // TODO: test code
    const u32 vertex_count = 4;
    Vertex3d verts[vertex_count] = {
        {-.5f * 10, -0.5f * 10, 0.0f},
        {0.5f * 10, 0.5f * 10, 0.0f},
        {-0.5f * 10, 0.5f * 10, 0.0f},
        {0.5f * 10, -0.5f * 10, 0.0f}
    };

    const u32 index_count = 6;
    u32 indices[index_count] = {0, 1, 2, 0, 3, 1};

    upload_data(
        &context, 
        context.device.graphics_command_pool, 
        VK_NULL_HANDLE, context.device.graphics_queue, &context.obj_vertex_buffer, 0, sizeof(Vertex3d) * vertex_count, verts);
    upload_data(
        &context, 
        context.device.graphics_command_pool, 
        VK_NULL_HANDLE, context.device.graphics_queue, &context.obj_index_buffer, 0, sizeof(u32) * index_count, indices);
    // TODO: test code end

    log_info("Vulkan renderer initialized successfully.");
    return true;
}

void vulkan_renderer_backend_shutdown(RendererBackend* backend)
{
    vkDeviceWaitIdle(context.device.logical_device);

    destroy_buffers(&context);

    vulkan_obj_shader_destroy(&context, &context.obj_shader);

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
    VulkanDevice* device = &context.device;

    if (context.recreating_swapchain)
    {
        VkResult result = vkDeviceWaitIdle(device->logical_device);
        if (!vulkan_result_is_successful(result))
        {
            log_error("Failed to wait for device to become idle. %s", vulkan_result_string(result, true));
            return false;
        }
        log_info("Recreating swapchain...");
        return false;
    }

    if (context.framebuffer_size_generated != context.framebuffer_last_size_generated)
    {
        VkResult result = vkDeviceWaitIdle(device->logical_device);
        if (!vulkan_result_is_successful(result))
        {
            log_error("Failed to wait for device to become idle. %s", vulkan_result_string(result, true));
            return false;
        }

        if (!recreate_swapchain(backend)) 
        {
            return false;
        }

        log_info("Recreated swapchain.");
        return false;
    }

    if (!vulkan_fence_wait(&context, &context.in_flight_fences[context.current_frame], 0xffffffffffffffff))
    {
        log_warning("Failed to wait for in flight fence.");
        return false;
    }

    if (!vulkan_swapchain_acquire_next_image(
        &context, &context.swapchain, 0xffffffffffffffff,
        context.image_available_semaphores[context.current_frame],
        NULL,
        &context.image_index))
    {
        return false;
    }

    VulkanCommandBuffer* command_buffer = &context.graphics_command_buffers[context.image_index];
    vulkan_command_buffer_reset(command_buffer);
    vulkan_command_buffer_begin(command_buffer, false, false, false);

    VkViewport viewport;
    viewport.x = 0.0f;
    viewport.y = context.framebuffer_height;
    viewport.width = context.framebuffer_width;
    viewport.height = -(f32)context.framebuffer_height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor;
    scissor.offset = (VkOffset2D) {0, 0};
    scissor.extent = (VkExtent2D) {context.framebuffer_width, context.framebuffer_height};

    vkCmdSetViewport(command_buffer->command_buffer, 0, 1, &viewport);
    vkCmdSetScissor(command_buffer->command_buffer, 0, 1, &scissor);

    context.main_render_pass.w = context.framebuffer_width;
    context.main_render_pass.h = context.framebuffer_height;

    vulkan_renderpass_begin(command_buffer, &context.main_render_pass, context.swapchain.framebuffers[context.image_index].framebuffer);

    return true;
}

bool vulkan_renderer_backend_end_frame(RendererBackend* backend, f64 delta_time)
{
    VulkanCommandBuffer* command_buffer = &context.graphics_command_buffers[context.image_index];
    vulkan_renderpass_end(command_buffer, &context.main_render_pass);
    vulkan_command_buffer_end(command_buffer);

    if (context.images_in_flight[context.image_index] != VK_NULL_HANDLE)
    {
        vulkan_fence_wait(&context, context.images_in_flight[context.image_index], 0xffffffffffffffff);
    }

    context.images_in_flight[context.image_index] = &context.in_flight_fences[context.current_frame];
    vulkan_fence_reset(&context, &context.in_flight_fences[context.current_frame]);

    VkSubmitInfo submit_info = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer->command_buffer;
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &context.queue_complete_semaphores[context.current_frame];
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &context.image_available_semaphores[context.current_frame];

    VkPipelineStageFlags flags[1] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submit_info.pWaitDstStageMask = flags;

    VkResult result = vkQueueSubmit(context.device.graphics_queue, 1, &submit_info, context.in_flight_fences[context.current_frame].fence);
    if (!vulkan_result_is_successful(result))
    {
        log_error("Failed to submit command buffer to queue. %s", vulkan_result_string(result, true));
        return false;
    }

    vulkan_command_buffer_update_submitted(command_buffer);

    vulkan_swapchain_present(
        &context, &context.swapchain,
        context.device.graphics_queue, context.device.present_queue,
        context.queue_complete_semaphores[context.current_frame],
        context.image_index);

    return true;
}

void vulkan_renderer_update_global_uniform(Mat4 projection, Mat4 view, Vec3 view_positioni, Vec4 ambient_color, i32 mode)
{
    VulkanCommandBuffer* command_buffer = &context.graphics_command_buffers[context.image_index];
    vulkan_obj_shader_use(&context, &context.obj_shader);

    context.obj_shader.global_uniform.projection = projection;
    context.obj_shader.global_uniform.view = view;

    vulkan_obj_shader_update_global_uniform(&context, &context.obj_shader);
}

void vulkan_renderer_update_model(Mat4 model)
{
    vulkan_obj_shader_update_model(&context, &context.obj_shader, model);

    VulkanCommandBuffer* command_buffer = &context.graphics_command_buffers[context.image_index];
    // TODO: test code
    vulkan_obj_shader_use(&context, &context.obj_shader);

    VkDeviceSize offsets[1] = {0};
    vkCmdBindVertexBuffers(command_buffer->command_buffer, 0, 1, &context.obj_vertex_buffer.buffer, offsets);
    vkCmdBindIndexBuffer(command_buffer->command_buffer, context.obj_index_buffer.buffer, 0, VK_INDEX_TYPE_UINT32); 
    vkCmdDrawIndexed(command_buffer->command_buffer, 6, 1, 0, 0, 0);
}

void vulkan_renderer_backend_resize(RendererBackend* backend, i32 width, i32 height)
{
    raw_framebuffer_height = height;
    raw_framebuffer_width = width;
    context.framebuffer_size_generated++;

    log_info("Resizing framebuffer to %dx%d %d", width, height, context.framebuffer_size_generated);
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
        VK_ASSERT(vkCreateSemaphore(context.device.logical_device, &semaphore_create_info, context.allocator, &context.image_available_semaphores[i]));
        VK_ASSERT(vkCreateSemaphore(context.device.logical_device, &semaphore_create_info, context.allocator, &context.queue_complete_semaphores[i]));

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

bool recreate_swapchain(RendererBackend* backend)
{
    if (context.recreating_swapchain)
    {
        log_debug("Already recreating swapchain.");
        return false;
    }

    if (context.framebuffer_width == 0 || context.framebuffer_height == 0)
    {
        log_warning("Framebuffer size is 0. Cannot recreate swapchain.");
        return false;
    }

    context.recreating_swapchain = true;

    vkDeviceWaitIdle(context.device.logical_device);

    for (u32 i = 0; i < context.swapchain.image_count; ++i)
    {
        context.images_in_flight[i] = NULL;
    }

    vulkan_device_query_swapchain_support(context.device.physical_device, context.surface, &context.device.swapchain_support);
    vulkan_device_detect_depth_format(&context.device);

    vulkan_swapchain_recreate(&context, raw_framebuffer_width, raw_framebuffer_height, &context.swapchain);

    context.framebuffer_width = raw_framebuffer_width;
    context.framebuffer_height = raw_framebuffer_height;
    context.main_render_pass.w = context.framebuffer_width;
    context.main_render_pass.h = context.framebuffer_height;
    raw_framebuffer_width = 0;
    raw_framebuffer_height = 0;

    context.framebuffer_last_size_generated = context.framebuffer_size_generated;

    for (u32 i = 0; i < context.swapchain.image_count; ++i)
    {
        vulkan_command_buffer_free(&context, context.device.graphics_command_pool, &context.graphics_command_buffers[i]);
    }

    context.main_render_pass.x = 0;
    context.main_render_pass.y = 0;
    context.main_render_pass.w = context.framebuffer_width;
    context.main_render_pass.h = context.framebuffer_height;

    regenerate_framebuffers(backend, &context.swapchain, &context.main_render_pass);
    create_command_buffers(backend);

    context.recreating_swapchain = false;

    return true;
}

bool create_buffers(VulkanContext* context)
{
    VkMemoryPropertyFlagBits flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    const u64 vertex_buffer_size = sizeof(Vertex3d) * 1024 * 1024;
    if (!vulkan_buffer_create(
        context,
        vertex_buffer_size,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        flags,
        true,
        &context->obj_vertex_buffer
    ))
    {
        log_error("Failed to create vertex buffer.");
        return false;
    }

    context->geometry_vertex_offset = 0;

    const u64 index_buffer_size = sizeof(u32) * 1024 * 1024;
    if (!vulkan_buffer_create(
        context,
        index_buffer_size,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        flags,
        true,
        &context->obj_index_buffer
    ))
    {
        log_error("Failed to create index buffer.");
        return false;
    }

    context->geometry_index_offset = 0;

    return true;
}

void destroy_buffers(VulkanContext* context)
{
    vulkan_buffer_destroy(context, &context->obj_vertex_buffer);
    vulkan_buffer_destroy(context, &context->obj_index_buffer);
}

void upload_data(VulkanContext* context, VkCommandPool pool, VkFence fence, VkQueue queue, VulkanBuffer* buffer, u64 offset, u64 size, void* data)
{
    VkBufferUsageFlags flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    VulkanBuffer staging_buffer = {0};
    vulkan_buffer_create(context, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, flags, true, &staging_buffer);

    vulkan_buffer_load_data(context, &staging_buffer, 0, size, 0, data);

    vulkan_buffer_copy(
        context,
        pool,
        fence,
        queue,
        staging_buffer.buffer,
        0,
        buffer->buffer,
        offset,
        size
    );

    vulkan_buffer_destroy(context, &staging_buffer);
}