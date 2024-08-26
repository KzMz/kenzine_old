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
#include "vulkan_utils.h"
#include "vulkan_buffer.h"
#include "vulkan_image.h"
#include "vulkan_pipeline.h"

#include "systems/shader_system.h"
#include "systems/material_system.h"
#include "systems/texture_system.h"
#include "systems/resource_system.h"

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

void regenerate_framebuffers(void);
void destroy_framebuffers(void);

void create_sync_objects(RendererBackend* backend);
void destroy_sync_objects(RendererBackend* backend);

bool recreate_swapchain(RendererBackend* backend);
bool create_module(VulkanShader* shader, VulkanShaderStageConfig config, VulkanShaderStage* stage);

bool upload_data(VulkanContext* context, VkCommandPool pool, VkFence fence, VkQueue queue, VulkanBuffer* buffer, u64* old_offset, u64 size, void* data);
void free_data(VulkanBuffer* buffer, u64 offset, u64 size);

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

    // World render pass
    vulkan_renderpass_create(
        &context, &context.main_render_pass,
        (Vec4) { 0, 0, context.framebuffer_width, context.framebuffer_height },
        (Vec4) { 0.0f, 0.0f, 0.2f, 1.0f },
        1.0f,
        0,
        RENDERPASS_CLEAR_STENCIL_BUFFER_FLAG | RENDERPASS_CLEAR_COLOR_BUFFER_FLAG | RENDERPASS_CLEAR_DEPTH_BUFFER_FLAG,
        false, true
    );

    // UI render pass
    vulkan_renderpass_create(
        &context, &context.ui_render_pass,
        (Vec4) { 0, 0, context.framebuffer_width, context.framebuffer_height },
        (Vec4) { 0.0f, 0.0f, 0.0f, 0.0f },
        1.0f,
        0,
        RENDERPASS_CLEAR_NONE_FLAG,
        true, false
    );

    regenerate_framebuffers();

    create_command_buffers(backend);

    create_sync_objects(backend);

    create_buffers(&context);

    for (u32 i = 0; i < MAX_GEOMETRY_COUNT; ++i)
    {
        context.geometries[i].id = INVALID_ID;
    }

    log_info("Vulkan renderer initialized successfully.");
    return true;
}

void vulkan_renderer_backend_shutdown(RendererBackend* backend)
{
    vkDeviceWaitIdle(context.device.logical_device);

    destroy_buffers(&context);

    destroy_sync_objects(backend);

    destroy_command_buffers(backend);

    destroy_framebuffers();

    vulkan_renderpass_destroy(&context, &context.ui_render_pass);
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
    context.frame_delta_time = delta_time;
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

    VkResult result = vkWaitForFences(device->logical_device, 1, &context.in_flight_fences[context.current_frame], VK_TRUE, 0xffffffffffffffff);
    if (!vulkan_result_is_successful(result))
    {
        log_warning("Failed to wait for in flight fence. %s", vulkan_result_string(result, true));
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

    context.main_render_pass.render_area.z = context.framebuffer_width;
    context.main_render_pass.render_area.w = context.framebuffer_height;

    context.ui_render_pass.render_area.z = context.framebuffer_width;
    context.ui_render_pass.render_area.w = context.framebuffer_height;

    return true;
}

bool vulkan_renderer_backend_end_frame(RendererBackend* backend, f64 delta_time)
{
    VulkanCommandBuffer* command_buffer = &context.graphics_command_buffers[context.image_index];
    vulkan_command_buffer_end(command_buffer);

    if (context.images_in_flight[context.image_index] != VK_NULL_HANDLE)
    {
        VkResult result = vkWaitForFences(context.device.logical_device, 1, &context.images_in_flight[context.image_index], VK_TRUE, 0xffffffffffffffff);
        if (!vulkan_result_is_successful(result))
        {
            log_error("Failed to wait for in flight fence. %s", vulkan_result_string(result, true));
            return false;
        }
    }

    context.images_in_flight[context.image_index] = context.in_flight_fences[context.current_frame];

    VK_ASSERT(vkResetFences(context.device.logical_device, 1, &context.in_flight_fences[context.current_frame]));

    VkSubmitInfo submit_info = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer->command_buffer;
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &context.queue_complete_semaphores[context.current_frame];
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &context.image_available_semaphores[context.current_frame];

    VkPipelineStageFlags flags[1] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submit_info.pWaitDstStageMask = flags;

    VkResult result = vkQueueSubmit(context.device.graphics_queue, 1, &submit_info, context.in_flight_fences[context.current_frame]);
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

bool vulkan_renderer_begin_renderpass(RendererBackend* backend, u8 renderpass_id)
{
    VulkanRenderPass* render_pass = NULL;
    VkFramebuffer framebuffer = VK_NULL_HANDLE;
    VulkanCommandBuffer* command_buffer = &context.graphics_command_buffers[context.image_index];

    switch (renderpass_id)
    {
        case BUILTIN_RENDERPASS_WORLD:
            render_pass = &context.main_render_pass;
            framebuffer = context.world_framebuffers[context.image_index];
            break;
        case BUILTIN_RENDERPASS_UI:
            render_pass = &context.ui_render_pass;
            framebuffer = context.swapchain.framebuffers[context.image_index];
            break;
        default:
            log_error("Invalid renderpass id. [Id: %d]", renderpass_id);
            return false;
    }

    vulkan_renderpass_begin(command_buffer, render_pass, framebuffer);
    return true;
}

bool vulkan_renderer_end_renderpass(RendererBackend* backend, u8 renderpass_id)
{
    VulkanRenderPass* render_pass = NULL;
    VulkanCommandBuffer* command_buffer = &context.graphics_command_buffers[context.image_index];

    switch (renderpass_id)
    {
        case BUILTIN_RENDERPASS_WORLD:
            render_pass = &context.main_render_pass;
            break;
        case BUILTIN_RENDERPASS_UI:
            render_pass = &context.ui_render_pass;
            break;
        default:
            log_error("Invalid renderpass id. [Id: %d]", renderpass_id);
            return false;
    }

    vulkan_renderpass_end(command_buffer, render_pass);
    return true;
}

bool vulkan_renderer_create_geometry
(    
    Geometry* geometry, 
    u32 vertex_count, u32 vertex_size, const void* vertices, 
    u32 index_count, u32 index_size, const void* indices
)
{
    if (vertex_count == 0 || vertices == NULL)
    {
        log_error("Invalid vertex data.");
        return false;
    }

    bool reupload = geometry->internal_id != INVALID_ID;
    VulkanGeometryData old_data = {0};

    VulkanGeometryData* internal_data = NULL;
    if (reupload)
    {
        internal_data = &context.geometries[geometry->internal_id];

        old_data.index_buffer_offset = internal_data->index_buffer_offset;
        old_data.index_count = internal_data->index_count;
        old_data.index_element_size = internal_data->index_element_size;
        old_data.vertex_buffer_offset = internal_data->vertex_buffer_offset;
        old_data.vertex_count = internal_data->vertex_count;
        old_data.vertex_element_size = internal_data->vertex_element_size;
    }
    else 
    {
        for (u32 i = 0; i < MAX_GEOMETRY_COUNT; ++i)
        {
            if (context.geometries[i].id != INVALID_ID) continue;

            geometry->internal_id = i;
            context.geometries[i].id = i;
            internal_data = &context.geometries[i];
            break;
        }        
    }

    if (internal_data == NULL)
    {
        log_error("Failed to find free geometry slot.");
        return false;
    }

    VkCommandPool pool = context.device.graphics_command_pool;
    VkQueue queue = context.device.graphics_queue;

    internal_data->vertex_count = vertex_count;
    internal_data->vertex_element_size = sizeof(Vertex3d);
    u32 vertex_buffer_size = vertex_count * internal_data->vertex_element_size;
    if (!upload_data(
        &context, pool, VK_NULL_HANDLE, queue, 
        &context.obj_vertex_buffer, 
        &internal_data->vertex_buffer_offset, 
        vertex_buffer_size, 
        (void* ) vertices
    ))
    {
        log_error("Failed to upload vertex data.");
        return false;
    }


    if (index_count > 0 && indices != NULL)
    {
        internal_data->index_count = index_count;
        internal_data->index_element_size = sizeof(u32);
        u32 index_buffer_size = index_count * internal_data->index_element_size;
        if (!upload_data(
            &context, pool, VK_NULL_HANDLE, queue, 
            &context.obj_index_buffer, 
            &internal_data->index_buffer_offset, 
            index_buffer_size, 
            (void*) indices
        ))
        {
            log_error("Failed to upload index data.");
            return false;
        }
    }

    if (internal_data->generation == INVALID_ID)
    {
        internal_data->generation = 0;
    }
    else
    {
        internal_data->generation++;
    }

    if (reupload)
    {
        free_data(&context.obj_vertex_buffer, old_data.vertex_buffer_offset, old_data.vertex_element_size * old_data.vertex_count);
        if (old_data.index_count > 0)
        {
            free_data(&context.obj_index_buffer, old_data.index_buffer_offset, old_data.index_element_size * old_data.index_count);
        }
    }

    return true;
}

void vulkan_renderer_draw_geometry(GeometryRenderData data)
{
    if (data.geometry == NULL) return;
    if (data.geometry->internal_id == INVALID_ID) return;

    VulkanGeometryData* internal_data = &context.geometries[data.geometry->internal_id];
    VulkanCommandBuffer* command_buffer = &context.graphics_command_buffers[context.image_index];

    VkDeviceSize offsets[1] = {internal_data->vertex_buffer_offset};
    vkCmdBindVertexBuffers(command_buffer->command_buffer, 0, 1, &context.obj_vertex_buffer.buffer, (VkDeviceSize*) offsets);

    if (internal_data->index_count > 0)
    {
        vkCmdBindIndexBuffer(command_buffer->command_buffer, context.obj_index_buffer.buffer, internal_data->index_buffer_offset, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(command_buffer->command_buffer, internal_data->index_count, 1, 0, 0, 0);
    }
    else
    {
        vkCmdDraw(command_buffer->command_buffer, internal_data->vertex_count, 1, 0, 0);
    }
}

void vulkan_renderer_destroy_geometry(Geometry* geometry)
{
    if (geometry == NULL) return;
    if (geometry->internal_id == INVALID_ID) return;

    vkDeviceWaitIdle(context.device.logical_device);
    VulkanGeometryData* internal_data = &context.geometries[geometry->internal_id];

    free_data(&context.obj_vertex_buffer, internal_data->vertex_buffer_offset, internal_data->vertex_element_size * internal_data->vertex_count);

    if (internal_data->index_count > 0)
    {
        free_data(&context.obj_index_buffer, internal_data->index_buffer_offset, internal_data->index_element_size * internal_data->index_count);
    }

    memory_zero(internal_data, sizeof(VulkanGeometryData));
    internal_data->id = INVALID_ID;
    internal_data->generation = INVALID_ID;
}

void vulkan_renderer_backend_resize(RendererBackend* backend, i32 width, i32 height)
{
    raw_framebuffer_height = height;
    raw_framebuffer_width = width;
    context.framebuffer_size_generated++;

    log_info("Resizing framebuffer to %dx%d %d", width, height, context.framebuffer_size_generated);
}

void vulkan_renderer_create_texture(const u8* pixels, Texture* texture)
{
    texture->data = memory_alloc(sizeof(VulkanTexture), MEMORY_TAG_TEXTURE);
    VulkanTexture* vk_texture = (VulkanTexture*) texture->data;

    VkDeviceSize image_size = texture->width * texture->height * texture->channel_count;
    VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;

    VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    VulkanBuffer staging_buffer = {0};
    vulkan_buffer_create(&context, image_size, usage, properties, true, false, &staging_buffer);
    vulkan_buffer_load_data(&context, &staging_buffer, 0, image_size, 0, pixels);

    vulkan_image_create(
        &context,
        VK_IMAGE_TYPE_2D,
        texture->width, texture->height,
        format, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        true,
        VK_IMAGE_ASPECT_COLOR_BIT,
        &vk_texture->image
    );

    VulkanCommandBuffer command_buffer = {0};
    VkCommandPool command_pool = context.device.graphics_command_pool;
    VkQueue queue = context.device.graphics_queue;
    vulkan_command_buffer_alloc_and_begin_single_use(&context, command_pool, &command_buffer);

    vulkan_image_transition_layout(
        &context, &command_buffer, &vk_texture->image, format,
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
    );

    vulkan_image_copy_from_buffer(
        &context, &vk_texture->image, staging_buffer.buffer, &command_buffer
    );

    vulkan_image_transition_layout(
        &context, &command_buffer, &vk_texture->image, format,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    );

    vulkan_command_buffer_end_and_submit_single_use(&context, command_pool, &command_buffer, queue);
    vulkan_buffer_destroy(&context, &staging_buffer);

    VkSamplerCreateInfo sampler_info = {VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    sampler_info.magFilter = VK_FILTER_LINEAR;
    sampler_info.minFilter = VK_FILTER_LINEAR;
    sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.anisotropyEnable = VK_TRUE;
    sampler_info.maxAnisotropy = 16;
    sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    sampler_info.unnormalizedCoordinates = VK_FALSE;
    sampler_info.compareEnable = VK_FALSE;
    sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
    sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_info.mipLodBias = 0.0f;
    sampler_info.minLod = 0.0f;
    sampler_info.maxLod = 0.0f;

    VkResult result = vkCreateSampler(context.device.logical_device, &sampler_info, context.allocator, &vk_texture->sampler);
    if (!vulkan_result_is_successful(result))
    {
        log_error("Failed to create vk_texture sampler. %s", vulkan_result_string(result, true));
        return;
    }

    texture->generation++;
}

void vulkan_renderer_destroy_texture(Texture* texture)
{
    vkDeviceWaitIdle(context.device.logical_device);

    VulkanTexture* vtexture = (VulkanTexture*) texture->data;
    if (vtexture != NULL)
    {
        vulkan_image_destroy(&context, &vtexture->image);
        memory_zero(&vtexture->image, sizeof(VulkanImage));
        vkDestroySampler(context.device.logical_device, vtexture->sampler, context.allocator);
        vtexture->sampler = VK_NULL_HANDLE;

        memory_free(texture->data, sizeof(VulkanTexture), MEMORY_TAG_TEXTURE);
    }
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

void regenerate_framebuffers(void)
{
    u32 image_count = context.swapchain.image_count;
    for (u32 i = 0; i < image_count; ++i)
    {
        VkImageView world_attachments[2] = {context.swapchain.image_views[i], context.swapchain.depth_attachment.view};
        VkFramebufferCreateInfo framebuffer_create_info = {VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        framebuffer_create_info.renderPass = context.main_render_pass.render_pass;
        framebuffer_create_info.attachmentCount = 2;
        framebuffer_create_info.pAttachments = world_attachments;
        framebuffer_create_info.width = context.framebuffer_width;
        framebuffer_create_info.height = context.framebuffer_height;
        framebuffer_create_info.layers = 1;

        VK_ASSERT(vkCreateFramebuffer(context.device.logical_device, &framebuffer_create_info, context.allocator, &context.world_framebuffers[i]));

        VkImageView ui_attachments[1] = {context.swapchain.image_views[i]};
        VkFramebufferCreateInfo ui_framebuffer_create_info = {VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        ui_framebuffer_create_info.renderPass = context.ui_render_pass.render_pass;
        ui_framebuffer_create_info.attachmentCount = 1;
        ui_framebuffer_create_info.pAttachments = ui_attachments;
        ui_framebuffer_create_info.width = context.framebuffer_width;
        ui_framebuffer_create_info.height = context.framebuffer_height;
        ui_framebuffer_create_info.layers = 1;

        VK_ASSERT(vkCreateFramebuffer(context.device.logical_device, &ui_framebuffer_create_info, context.allocator, &context.swapchain.framebuffers[i]));
    }
}

void destroy_framebuffers(void)
{
    u32 image_count = context.swapchain.image_count;
    for (u32 i = 0; i < image_count; ++i)
    {
        vkDestroyFramebuffer(context.device.logical_device, context.world_framebuffers[i], context.allocator);
        vkDestroyFramebuffer(context.device.logical_device, context.swapchain.framebuffers[i], context.allocator);
    }
}

void create_sync_objects(RendererBackend* backend)
{
    context.image_available_semaphores = dynarray_reserve(VkSemaphore, context.swapchain.max_frames_in_flight);
    context.queue_complete_semaphores = dynarray_reserve(VkSemaphore, context.swapchain.max_frames_in_flight);

    memory_zero(context.in_flight_fences, sizeof(VkFence) * 2);

    for (u8 i = 0; i < context.swapchain.max_frames_in_flight; ++i)
    {
        VkSemaphoreCreateInfo semaphore_create_info = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        VK_ASSERT(vkCreateSemaphore(context.device.logical_device, &semaphore_create_info, context.allocator, &context.image_available_semaphores[i]));
        VK_ASSERT(vkCreateSemaphore(context.device.logical_device, &semaphore_create_info, context.allocator, &context.queue_complete_semaphores[i]));

        VkFenceCreateInfo fence_create_info = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        VK_ASSERT(vkCreateFence(context.device.logical_device, &fence_create_info, context.allocator, &context.in_flight_fences[i]));
    }

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

        vkDestroyFence(context.device.logical_device, context.in_flight_fences[i], context.allocator);
    }

    dynarray_destroy(context.image_available_semaphores);
    context.image_available_semaphores = NULL;

    dynarray_destroy(context.queue_complete_semaphores);
    context.queue_complete_semaphores = NULL;
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
    raw_framebuffer_width = 0;
    raw_framebuffer_height = 0;

    context.framebuffer_last_size_generated = context.framebuffer_size_generated;

    for (u32 i = 0; i < context.swapchain.image_count; ++i)
    {
        vulkan_command_buffer_free(&context, context.device.graphics_command_pool, &context.graphics_command_buffers[i]);
    }

    destroy_framebuffers();
    
    context.main_render_pass.render_area = (Vec4) { 0, 0, context.framebuffer_width, context.framebuffer_height };
    context.ui_render_pass.render_area = (Vec4) { 0, 0, context.framebuffer_width, context.framebuffer_height };

    regenerate_framebuffers();
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
        true,
        &context->obj_vertex_buffer
    ))
    {
        log_error("Failed to create vertex buffer.");
        return false;
    }

    const u64 index_buffer_size = sizeof(u32) * 1024 * 1024;
    if (!vulkan_buffer_create(
        context,
        index_buffer_size,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        flags,
        true,
        true,
        &context->obj_index_buffer
    ))
    {
        log_error("Failed to create index buffer.");
        return false;
    }

    return true;
}

void destroy_buffers(VulkanContext* context)
{
    vulkan_buffer_destroy(context, &context->obj_vertex_buffer);
    vulkan_buffer_destroy(context, &context->obj_index_buffer);
}

bool upload_data(VulkanContext* context, VkCommandPool pool, VkFence fence, VkQueue queue, VulkanBuffer* buffer, u64* out_offset, u64 size, void* data)
{
    if (!vulkan_buffer_alloc(buffer, size, out_offset))
    {
        log_error("Failed to allocate buffer memory.");
        return false;
    }

    VkBufferUsageFlags flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    VulkanBuffer staging_buffer = {0};
    vulkan_buffer_create(context, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, flags, true, false, &staging_buffer);

    vulkan_buffer_load_data(context, &staging_buffer, 0, size, 0, data);

    vulkan_buffer_copy(
        context,
        pool,
        fence,
        queue,
        staging_buffer.buffer,
        0,
        buffer->buffer,
        *out_offset,
        size
    );

    vulkan_buffer_destroy(context, &staging_buffer);
    return true;
}

void free_data(VulkanBuffer* buffer, u64 offset, u64 size)
{
    if (buffer == NULL) return;

    vulkan_buffer_free(buffer, size, offset);
}

const u32 DESC_SET_INDEX_GLOBAL = 0;
const u32 DESC_SET_INDEX_INSTANCE = 1;

const u32 BINDING_INDEX_UBO = 0;
const u32 BINDING_INDEX_SAMPLER = 1;

bool vulkan_renderer_create_shader(struct Shader* shader, u8 renderpass_id, u8 stage_count, const char** stage_files, ShaderStage* stages)
{
    shader->internal_data = memory_alloc(sizeof(VulkanShader), MEMORY_TAG_RENDERER);

    // TODO: dynamic renderpass
    VulkanRenderPass* renderpass = renderpass_id == 1 ? &context.ui_render_pass : &context.main_render_pass;

    VkShaderStageFlags vk_stages[VULKAN_SHADER_MAX_STAGES] = {0};
    for (u8 i = 0; i < stage_count; ++i)
    {
        switch (stages[i])
        {
            case SHADER_STAGE_FRAGMENT:
                vk_stages[i] = VK_SHADER_STAGE_FRAGMENT_BIT;
                break;
            case SHADER_STAGE_VERTEX:
                vk_stages[i] = VK_SHADER_STAGE_VERTEX_BIT;
                break;
            case SHADER_STAGE_GEOMETRY:
                log_warning("vulkan_renderer_create_shader: VK_SHADER_STAGE_GEOMETRY_BIT set but not supported");
                vk_stages[i] = VK_SHADER_STAGE_GEOMETRY_BIT;
                break;
            case SHADER_STAGE_COMPUTE:
                log_warning("vulkan_renderer_create_shader: VK_SHADER_STAGE_COMPUTE_BIT set but not supported");
                vk_stages[i] = VK_SHADER_STAGE_COMPUTE_BIT;
                break;
            default:
                log_error("vulkan_renderer_create_shader: Invalid shader stage.");
                return false;
        }
    }

    u32 max_descriptor_allocate_count = 1024;
    VulkanShader* out_shader = (VulkanShader*) shader->internal_data;
    out_shader->render_pass = renderpass;
    out_shader->config.max_descriptor_set_count = max_descriptor_allocate_count;

    memory_zero(out_shader->config.stages, sizeof(VulkanShaderStageConfig) * VULKAN_SHADER_MAX_STAGES);
    out_shader->config.stage_count = 0;

    for (u32 i = 0; i < stage_count; ++i)
    {
        if (out_shader->config.stage_count + 1 > VULKAN_SHADER_MAX_STAGES)
        {
            log_error("vulkan_renderer_create_shader: Too many shader stages.");
            return false;
        }

        VkShaderStageFlagBits stage_flag;
        switch (stages[i])
        {
            case SHADER_STAGE_VERTEX:
                stage_flag = VK_SHADER_STAGE_VERTEX_BIT;
                break;
            case SHADER_STAGE_FRAGMENT:
                stage_flag = VK_SHADER_STAGE_FRAGMENT_BIT;
                break;
            default:
                log_error("vulkan_renderer_create_shader: unsupported shader stage %d.", stages[i]);
                continue;
        }

        out_shader->config.stages[out_shader->config.stage_count].stage = stage_flag;
        string_copy_n(out_shader->config.stages[out_shader->config.stage_count].file_name, stage_files[i], 255);
        out_shader->config.stage_count++;
    }

    memory_zero(out_shader->config.descriptor_sets, sizeof(VulkanDescriptorSetConfig) * 2);
    memory_zero(out_shader->config.attributes, sizeof(VkVertexInputAttributeDescription) * VULKAN_SHADER_MAX_ATTRIBUTES);

    out_shader->config.pool_sizes[0] = (VkDescriptorPoolSize) { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1024 };
    out_shader->config.pool_sizes[1] = (VkDescriptorPoolSize) { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4096 };

    VulkanDescriptorSetConfig global_descriptor_set_config = {0};

    global_descriptor_set_config.bindings[BINDING_INDEX_UBO].binding = BINDING_INDEX_UBO;
    global_descriptor_set_config.bindings[BINDING_INDEX_UBO].descriptorCount = 1;
    global_descriptor_set_config.bindings[BINDING_INDEX_UBO].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    global_descriptor_set_config.bindings[BINDING_INDEX_UBO].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    global_descriptor_set_config.binding_count++;

    out_shader->config.descriptor_sets[DESC_SET_INDEX_GLOBAL] = global_descriptor_set_config;
    out_shader->config.descriptor_set_count++;

    if (shader->use_instances)
    {
        VulkanDescriptorSetConfig instance_descriptor_set_config = {0};

        instance_descriptor_set_config.bindings[BINDING_INDEX_UBO].binding = BINDING_INDEX_UBO;
        instance_descriptor_set_config.bindings[BINDING_INDEX_UBO].descriptorCount = 1;
        instance_descriptor_set_config.bindings[BINDING_INDEX_UBO].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        instance_descriptor_set_config.bindings[BINDING_INDEX_UBO].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        instance_descriptor_set_config.binding_count++;

        out_shader->config.descriptor_sets[DESC_SET_INDEX_INSTANCE] = instance_descriptor_set_config;
        out_shader->config.descriptor_set_count++;
    }

    for (u32 i = 0; i < 1024; ++i)
    {
        out_shader->instance_states[i].id = INVALID_ID;
    }

    return true;
}

void vulkan_renderer_destroy_shader(struct Shader* shader)
{
    if (shader == NULL || shader->internal_data == NULL) 
    {
        return;
    }

    VulkanShader* vk_shader = (VulkanShader*) shader->internal_data;
    if (shader == NULL)
    {
        log_error("vulkan_renderer_destroy_shader: Invalid shader.");
        return;
    }

    VkDevice logical_device = context.device.logical_device;
    VkAllocationCallbacks* vk_allocator = context.allocator;

    for (u32 i = 0; i < vk_shader->config.descriptor_set_count; ++i)
    {
        if (vk_shader->descriptor_set_layouts[i] == VK_NULL_HANDLE)
        {
            continue;
        }

        vkDestroyDescriptorSetLayout(logical_device, vk_shader->descriptor_set_layouts[i], vk_allocator);
        vk_shader->descriptor_set_layouts[i] = VK_NULL_HANDLE;
    }

    if (vk_shader->descriptor_pool != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool(logical_device, vk_shader->descriptor_pool, vk_allocator);
        vk_shader->descriptor_pool = VK_NULL_HANDLE;
    }

    vulkan_buffer_unlock(&context, &vk_shader->uniform_buffer);
    vk_shader->uniform_buffer_block = NULL;
    vulkan_buffer_destroy(&context, &vk_shader->uniform_buffer);

    vulkan_pipeline_destroy(&context, &vk_shader->pipeline);

    for (u32 i = 0; i < vk_shader->config.stage_count; ++i)
    {
        vkDestroyShaderModule(logical_device, vk_shader->stages[i].module, vk_allocator);
    }

    memory_zero(&vk_shader->config, sizeof(VulkanShaderConfig));

    memory_free(shader->internal_data, sizeof(VulkanShader), MEMORY_TAG_RENDERER);
    shader->internal_data = NULL;
}

bool vulkan_renderer_shader_init(struct Shader* shader)
{
    VkDevice logical_device = context.device.logical_device;
    VkAllocationCallbacks* vk_allocator = context.allocator;
    VulkanShader* vk_shader = (VulkanShader*) shader->internal_data;

    memory_zero(vk_shader->stages, sizeof(VulkanShaderStage) * VULKAN_SHADER_MAX_STAGES);
    for (u32 i = 0; i < vk_shader->config.stage_count; ++i)
    {
        if (!create_module(vk_shader, vk_shader->config.stages[i], &vk_shader->stages[i]))
        {
            log_error("vulkan_renderer_shader_init: Failed to create %s shader module for %s.", vk_shader->config.stages[i].file_name, shader->name);
            return false;
        }
    }

    static VkFormat* types = NULL;
    static VkFormat t[11];
    if (types == NULL)
    {
        t[SHADER_ATTRIB_TYPE_FLOAT32] = VK_FORMAT_R32_SFLOAT;
        t[SHADER_ATTRIB_TYPE_FLOAT32_2] = VK_FORMAT_R32G32_SFLOAT;
        t[SHADER_ATTRIB_TYPE_FLOAT32_3] = VK_FORMAT_R32G32B32_SFLOAT;
        t[SHADER_ATTRIB_TYPE_FLOAT32_4] = VK_FORMAT_R32G32B32A32_SFLOAT;
        t[SHADER_ATTRIB_TYPE_INT8] = VK_FORMAT_R8_SINT;
        t[SHADER_ATTRIB_TYPE_UINT8] = VK_FORMAT_R8_UINT;
        t[SHADER_ATTRIB_TYPE_INT16] = VK_FORMAT_R16_SINT;
        t[SHADER_ATTRIB_TYPE_UINT16] = VK_FORMAT_R16_UINT;
        t[SHADER_ATTRIB_TYPE_INT32] = VK_FORMAT_R32_SINT;
        t[SHADER_ATTRIB_TYPE_UINT32] = VK_FORMAT_R32_UINT;
        types = t;
    }

    u32 attribute_count = dynarray_length(shader->attributes);
    u64 offset = 0;
    for (u32 i = 0; i < attribute_count; ++i)
    {
        VkVertexInputAttributeDescription desc = 
        {
            .binding = 0,
            .location = i,
            .offset = offset,
            .format = types[shader->attributes[i].type]
        };

        vk_shader->config.attributes[i] = desc;
        offset += shader->attributes[i].size;
    }

    u32 uniform_count = dynarray_length(shader->uniforms);
    for (u32 i = 0; i < uniform_count; ++i)
    {
        if (shader->uniforms[i].type != SHADER_UNIFORM_TYPE_SAMPLER)
        {
            continue;
        }

        const u32 set_index = (shader->uniforms[i].scope == SHADER_SCOPE_GLOBAL ? DESC_SET_INDEX_GLOBAL : DESC_SET_INDEX_INSTANCE);
        VulkanDescriptorSetConfig* set_config = &vk_shader->config.descriptor_sets[set_index];
        if (set_config->binding_count < 2)
        {
            set_config->bindings[BINDING_INDEX_SAMPLER].binding = BINDING_INDEX_SAMPLER;
            set_config->bindings[BINDING_INDEX_SAMPLER].descriptorCount = 1;
            set_config->bindings[BINDING_INDEX_SAMPLER].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            set_config->bindings[BINDING_INDEX_SAMPLER].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
            set_config->binding_count++;
        }
        else 
        {
            set_config->bindings[BINDING_INDEX_SAMPLER].descriptorCount++;
        }
    }

    VkDescriptorPoolCreateInfo pool_info = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    pool_info.poolSizeCount = 2;
    pool_info.pPoolSizes = vk_shader->config.pool_sizes;
    pool_info.maxSets = vk_shader->config.max_descriptor_set_count;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

    VkResult result = vkCreateDescriptorPool(logical_device, &pool_info, vk_allocator, &vk_shader->descriptor_pool);
    if (!vulkan_result_is_successful(result))
    {
        log_error("vulkan_renderer_shader_init: Failed to create descriptor pool. %s", vulkan_result_string(result, true));
        return false;
    }

    memory_zero(vk_shader->descriptor_set_layouts, vk_shader->config.descriptor_set_count * sizeof(VkDescriptorSetLayout));
    for (u32 i = 0; i < vk_shader->config.descriptor_set_count; ++i)
    {
        VkDescriptorSetLayoutCreateInfo layout_info = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
        layout_info.bindingCount = vk_shader->config.descriptor_sets[i].binding_count;
        layout_info.pBindings = vk_shader->config.descriptor_sets[i].bindings;
        result = vkCreateDescriptorSetLayout(logical_device, &layout_info, vk_allocator, &vk_shader->descriptor_set_layouts[i]);
        if (!vulkan_result_is_successful(result))
        {
            log_error("vulkan_renderer_shader_init: Failed to create descriptor set layout. %s", vulkan_result_string(result, true));
            return false;
        }
    }   

    VkViewport viewport;
    viewport.x = 0.0f;
    viewport.y = (f32) context.framebuffer_height;
    viewport.width = (f32) context.framebuffer_width;
    viewport.height = -(f32) context.framebuffer_height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor;
    scissor.offset = (VkOffset2D) {0, 0};
    scissor.extent = (VkExtent2D) {context.framebuffer_width, context.framebuffer_height};

    VkPipelineShaderStageCreateInfo stage_create_infos[VULKAN_SHADER_MAX_STAGES] = {0};
    memory_zero(stage_create_infos, sizeof(VkPipelineShaderStageCreateInfo) * VULKAN_SHADER_MAX_STAGES);
    for (u32 i = 0; i < vk_shader->config.stage_count; ++i)
    {
        stage_create_infos[i] = vk_shader->stages[i].stage_info;
    }

    bool pipeline_result = vulkan_pipeline_create(
        &context,
        vk_shader->render_pass,
        shader->attribute_stride,
        dynarray_length(shader->attributes),
        vk_shader->config.attributes,
        vk_shader->config.descriptor_set_count,
        vk_shader->descriptor_set_layouts,
        vk_shader->config.stage_count,
        stage_create_infos,
        viewport,
        scissor,
        false,
        true,
        shader->push_constant_range_count,
        shader->push_constant_ranges,
        &vk_shader->pipeline
    );

    if (!pipeline_result)
    {
        log_error("vulkan_renderer_shader_init: Failed to create pipeline.");
        return false;
    }

    shader->required_uniform_alignment = context.device.properties.limits.minUniformBufferOffsetAlignment;
    shader->global_uniform_stride = get_aligned(shader->global_uniform_size, shader->required_uniform_alignment);
    shader->instance_uniform_stride = get_aligned(shader->instance_uniform_size, shader->required_uniform_alignment);

    u32 device_local_bits = context.device.supports_device_local_host_visible ? VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT : 0;
    u64 total_buffer_size = shader->global_uniform_stride + (shader->instance_uniform_stride * VULKAN_MAX_MATERIAL_COUNT);
    if (!vulkan_buffer_create(
        &context,
        total_buffer_size,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | device_local_bits,
        true,
        true,
        &vk_shader->uniform_buffer
    ))
    {
        log_error("vulkan_renderer_shader_init: Failed to create uniform buffer.");
        return false;
    }

    if (!vulkan_buffer_alloc(&vk_shader->uniform_buffer, shader->global_uniform_stride, &shader->global_uniform_offset))
    {
        log_error("vulkan_renderer_shader_init: Failed to allocate global uniform buffer.");
        return false;
    }   

    vk_shader->uniform_buffer_block = vulkan_buffer_lock(&context, &vk_shader->uniform_buffer, 0, VK_WHOLE_SIZE, 0);

    VkDescriptorSetLayout global_layouts[3] = 
    {
        vk_shader->descriptor_set_layouts[DESC_SET_INDEX_GLOBAL],
        vk_shader->descriptor_set_layouts[DESC_SET_INDEX_GLOBAL],
        vk_shader->descriptor_set_layouts[DESC_SET_INDEX_GLOBAL]
    };
    VkDescriptorSetAllocateInfo alloc_info = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    alloc_info.descriptorPool = vk_shader->descriptor_pool;
    alloc_info.descriptorSetCount = 3;
    alloc_info.pSetLayouts = global_layouts;
    VK_ASSERT(vkAllocateDescriptorSets(logical_device, &alloc_info, vk_shader->global_descriptor_sets));

    return true;
}

#ifdef _DEBUG
#define SHADER_VERIFY_SHADER_ID(shader_id)                                          \
    if (shader_id == INVALID_ID || context.shaders[shader_id].id == INVALID_ID)     \
    {                                                                               \
        return false;                                                               \
    }
#else
#define SHADER_VERIFY_SHADER_ID(shader_id)
#endif

bool vulkan_renderer_shader_use(struct Shader* shader)
{
    VulkanShader* vk_shader = (VulkanShader*) shader->internal_data;
    vulkan_pipeline_bind(&context.graphics_command_buffers[context.image_index], VK_PIPELINE_BIND_POINT_GRAPHICS, &vk_shader->pipeline);
    return true;
}

bool vulkan_renderer_shader_bind_globals(struct Shader* shader)
{
    if (shader == NULL)
    {
        return false;
    }

    shader->bound_uniform_offset = shader->global_uniform_offset;
    return true;
}

bool vulkan_renderer_shader_bind_instance(struct Shader* shader, u64 instance_id)
{
    if (shader == NULL)
    {
        log_error("vulkan_renderer_shader_bind_instance: Invalid shader pointer.");
        return false;
    }

    VulkanShader* vk_shader = (VulkanShader*) shader->internal_data;
    shader->bound_instance_id = instance_id;
    VulkanShaderInstanceState* object_state = &vk_shader->instance_states[instance_id];
    shader->bound_uniform_offset = object_state->offset;
    return true;
}

bool vulkan_renderer_shader_apply_globals(struct Shader* shader)
{
    u32 image_index = context.image_index;
    VulkanShader* vk_shader = (VulkanShader*) shader->internal_data;
    VkCommandBuffer command_buffer = context.graphics_command_buffers[image_index].command_buffer;
    VkDescriptorSet global_descriptor = vk_shader->global_descriptor_sets[image_index];

    VkDescriptorBufferInfo buffer_info = {0};
    buffer_info.buffer = vk_shader->uniform_buffer.buffer;
    buffer_info.offset = shader->global_uniform_offset;
    buffer_info.range = shader->global_uniform_stride;

    VkWriteDescriptorSet descriptor_write = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    descriptor_write.dstSet = global_descriptor;
    descriptor_write.dstBinding = 0;
    descriptor_write.dstArrayElement = 0;
    descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptor_write.descriptorCount = 1;
    descriptor_write.pBufferInfo = &buffer_info;

    VkWriteDescriptorSet writes[2] = {0};
    writes[0] = descriptor_write;

    u32 global_set_binding_count = vk_shader->config.descriptor_sets[DESC_SET_INDEX_GLOBAL].binding_count;
    if (global_set_binding_count > 1)
    {
        global_set_binding_count = 1;
    }

    vkUpdateDescriptorSets((VkDevice) context.device.logical_device, global_set_binding_count, writes, 0, NULL);
    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_shader->pipeline.layout, 0, 1, &global_descriptor, 0, NULL);
    return true;
}

bool vulkan_renderer_shader_apply_instance(struct Shader* shader)
{
    if (!shader->use_instances)
    {
        log_error("vulkan_renderer_shader_apply_instance: Shader does not support instances.");
        return false;
    }

    VulkanShader* vk_shader = (VulkanShader*) shader->internal_data;
    u32 image_index = context.image_index;
    VkCommandBuffer command_buffer = context.graphics_command_buffers[image_index].command_buffer;

    VulkanShaderInstanceState* instance_state = &vk_shader->instance_states[shader->bound_instance_id];
    VkDescriptorSet instance_descriptor = instance_state->descriptor_set_state.descriptor_sets[image_index];

    VkWriteDescriptorSet writes[2] = {0};
    memory_zero(writes, sizeof(VkWriteDescriptorSet) * 2);
    u32 descriptor_count = 0;
    u32 descriptor_index = 0;

    u8* instance_uniform_generation = &(instance_state->descriptor_set_state.descriptor_states[descriptor_index].generations[image_index]);
    if (*instance_uniform_generation == INVALID_ID_U8)
    {
        VkDescriptorBufferInfo buffer_info = {0};
        buffer_info.buffer = vk_shader->uniform_buffer.buffer;
        buffer_info.offset = instance_state->offset;
        buffer_info.range = shader->instance_uniform_stride;

        VkWriteDescriptorSet descriptor_write = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        descriptor_write.dstSet = instance_descriptor;
        descriptor_write.dstBinding = descriptor_index;
        descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptor_write.descriptorCount = 1;
        descriptor_write.pBufferInfo = &buffer_info;

        writes[descriptor_count] = descriptor_write;
        descriptor_count++;

        *instance_uniform_generation = 1;
    }

    descriptor_index++;

    if (vk_shader->config.descriptor_sets[DESC_SET_INDEX_INSTANCE].binding_count > 1)
    {
        u32 total_sampler_count = vk_shader->config.descriptor_sets[DESC_SET_INDEX_INSTANCE].bindings[BINDING_INDEX_SAMPLER].descriptorCount;
        u32 update_sampler_count = 0;
        VkDescriptorImageInfo image_infos[VULKAN_SHADER_MAX_GLOBAL_TEXTURES] = {0};
        for (u32 i = 0; i < total_sampler_count; ++i)
        {
            Texture* t = vk_shader->instance_states[shader->bound_instance_id].instance_textures[i];
            VulkanTexture* vt = (VulkanTexture*) t->data;
            image_infos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            image_infos[i].imageView = vt->image.view;
            image_infos[i].sampler = vt->sampler;

            update_sampler_count++;
        }

        VkWriteDescriptorSet sampler_write = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        sampler_write.dstSet = instance_descriptor;
        sampler_write.dstBinding = descriptor_index;
        sampler_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        sampler_write.descriptorCount = update_sampler_count;
        sampler_write.pImageInfo = image_infos;

        writes[descriptor_count] = sampler_write;
        descriptor_count++;
    }

    if (descriptor_count > 0)
    {
        vkUpdateDescriptorSets((VkDevice) context.device.logical_device, descriptor_count, writes, 0, NULL);
    }

    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_shader->pipeline.layout, 1, 1, &instance_descriptor, 0, NULL);
    return true;
}

bool vulkan_renderer_shader_acquire_instance_resources(struct Shader* shader, u64* out_instance_id)
{
    VulkanShader* vk_shader = (VulkanShader*) shader->internal_data;
    *out_instance_id = INVALID_ID;
    for (u32 i = 0; i < 1024; ++i)
    {
        if (vk_shader->instance_states[i].id == INVALID_ID)
        {
            vk_shader->instance_states[i].id = i;
            *out_instance_id = i;
            break;
        }
    }
    if (*out_instance_id == INVALID_ID)
    {
        log_error("vulkan_renderer_shader_acquire_instance_resources: Failed to acquire instance id.");
        return false;
    }

    VulkanShaderInstanceState* instance_state = &vk_shader->instance_states[*out_instance_id];
    u32 instance_texture_count = vk_shader->config.descriptor_sets[DESC_SET_INDEX_INSTANCE].bindings[BINDING_INDEX_SAMPLER].descriptorCount;
    instance_state->instance_textures = memory_alloc(sizeof(Texture*) * shader->instance_texture_count, MEMORY_TAG_RENDERER);
    Texture* default_texture = texture_system_get_default();
    for (u32 i = 0; i < instance_texture_count; ++i)
    {
        instance_state->instance_textures[i] = default_texture;
    }

    u64 size = shader->instance_uniform_stride;
    if (!vulkan_buffer_alloc(&vk_shader->uniform_buffer, size, &instance_state->offset))
    {
        log_error("vulkan_renderer_shader_acquire_instance_resources: Failed to allocate uniform buffer memory.");
        return false;
    }

    VulkanShaderDescriptorSetState* set_state = &instance_state->descriptor_set_state;

    u32 binding_count = vk_shader->config.descriptor_sets[DESC_SET_INDEX_INSTANCE].binding_count;
    memory_zero(set_state->descriptor_states, sizeof(VulkanDescriptorState) * VULKAN_SHADER_MAX_BINDINGS);
    for (u32 i = 0; i < binding_count; ++i)
    {
        for (u32 j = 0; j < 3; ++j)
        {
            set_state->descriptor_states[i].generations[j] = INVALID_ID_U8;
            set_state->descriptor_states[i].ids[j] = INVALID_ID;
        }
    }

    VkDescriptorSetLayout layouts[3] =
    {
        vk_shader->descriptor_set_layouts[DESC_SET_INDEX_INSTANCE],
        vk_shader->descriptor_set_layouts[DESC_SET_INDEX_INSTANCE],
        vk_shader->descriptor_set_layouts[DESC_SET_INDEX_INSTANCE]
    };
    VkDescriptorSetAllocateInfo alloc_info = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    alloc_info.descriptorPool = vk_shader->descriptor_pool;
    alloc_info.descriptorSetCount = 3;
    alloc_info.pSetLayouts = layouts;

    VkResult result = vkAllocateDescriptorSets(context.device.logical_device, &alloc_info, instance_state->descriptor_set_state.descriptor_sets);
    if (!vulkan_result_is_successful(result))
    {
        log_error("vulkan_renderer_shader_acquire_instance_resources: Failed to allocate descriptor sets. %s", vulkan_result_string(result, true));
        return false;
    }

    return true;
}

bool vulkan_renderer_shader_release_instance_resources(struct Shader* shader, u64 instance_id)
{
    VulkanShader* vk_shader = (VulkanShader*) shader->internal_data;
    VulkanShaderInstanceState* instance_state = &vk_shader->instance_states[instance_id];
    
    vkDeviceWaitIdle(context.device.logical_device);

    VkResult result = vkFreeDescriptorSets(
        context.device.logical_device,
        vk_shader->descriptor_pool,
        3,
        instance_state->descriptor_set_state.descriptor_sets
    );
    if (!vulkan_result_is_successful(result))
    {
        log_error("vulkan_renderer_shader_release_instance_resources: Failed to free descriptor sets. %s", vulkan_result_string(result, true));
        return false;
    }

    memory_zero(instance_state->descriptor_set_state.descriptor_states, sizeof(VulkanDescriptorState) * VULKAN_SHADER_MAX_BINDINGS);

    if (instance_state->instance_textures != NULL)
    {
        memory_free(instance_state->instance_textures, sizeof(Texture*) * shader->instance_texture_count, MEMORY_TAG_RENDERER);
        instance_state->instance_textures = NULL;
    }

    vulkan_buffer_free(&vk_shader->uniform_buffer, shader->instance_uniform_stride, instance_state->offset);
    instance_state->offset = INVALID_ID;
    instance_state->id = INVALID_ID;

    return true;
}

bool vulkan_renderer_set_uniform(struct Shader* shader, struct ShaderUniform* uniform, const void* value)
{
    VulkanShader* vk_shader = (VulkanShader*) shader->internal_data;
    if (uniform->type == SHADER_UNIFORM_TYPE_SAMPLER)
    {
        if (uniform->scope == SHADER_SCOPE_GLOBAL)
        {
            shader->global_textures[uniform->location] = (Texture*) value;
        }
        else 
        {
            vk_shader->instance_states[shader->bound_instance_id].instance_textures[uniform->location] = (Texture*) value;
        }
    }
    else 
    {
        if (uniform->scope == SHADER_SCOPE_LOCAL)
        {
            VkCommandBuffer command_buffer = context.graphics_command_buffers[context.image_index].command_buffer;
            vkCmdPushConstants(command_buffer, vk_shader->pipeline.layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, uniform->offset, uniform->size, value);
        }
        else 
        {
            u64 addr = (u64) vk_shader->uniform_buffer_block;
            addr += shader->bound_uniform_offset + uniform->offset;
            memory_copy((void*) addr, value, uniform->size);
        }
    }

    return true;
}

bool create_module(VulkanShader* shader, VulkanShaderStageConfig config, VulkanShaderStage* shader_stage)
{
    Resource binary_resource;
    if (!resource_system_load(config.file_name, RESOURCE_TYPE_BINARY, &binary_resource))
    {
        log_error("create_module: Failed to load shader binary resource %s.", config.file_name);
        return false;
    }

    memory_zero(&shader_stage->create_info, sizeof(VkShaderModuleCreateInfo));
    shader_stage->create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shader_stage->create_info.codeSize = binary_resource.size;
    shader_stage->create_info.pCode = (u32*) binary_resource.data;

    VK_ASSERT(vkCreateShaderModule(context.device.logical_device, &shader_stage->create_info, context.allocator, &shader_stage->module));

    resource_system_unload(&binary_resource);

    memory_zero(&shader_stage->stage_info, sizeof(VkPipelineShaderStageCreateInfo));
    shader_stage->stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stage->stage_info.stage = config.stage;
    shader_stage->stage_info.module = shader_stage->module;
    shader_stage->stage_info.pName = "main";

    return true;
}