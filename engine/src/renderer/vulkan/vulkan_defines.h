#pragma once

#include "defines.h"
#include <vulkan/vulkan.h>
#include "core/asserts.h"
#include "renderer/renderer_defines.h"

#define MAX_INDICES 32
#define MAX_PHYSICAL_DEVICES 32
#define MAX_QUEUE_FAMILIES 32
#define OBJECT_SHADER_STAGE_COUNT 2
#define MAX_OBJECT_COUNT 1024
#define OBJECT_SHADER_DESCRIPTOR_COUNT 2

#define VK_ASSERT(expr) do { kz_assert((expr) == VK_SUCCESS); } while(0)

typedef struct VulkanBuffer 
{
    u64 size;
    VkBuffer buffer;
    VkBufferUsageFlagBits usage;
    VkDeviceMemory memory;
    bool locked;
    i32 memory_index;
    u32 memory_property_flags;
} VulkanBuffer;

typedef struct VulkanSwapchainSupportInfo
{
    VkSurfaceCapabilitiesKHR capabilities;
    VkSurfaceFormatKHR* formats;
    u32 format_count;
    VkPresentModeKHR* present_modes;
    u32 present_mode_count;
} VulkanSwapchainSupportInfo;

typedef struct VulkanDevice
{
    VkPhysicalDevice physical_device;
    VkDevice logical_device;
    VulkanSwapchainSupportInfo swapchain_support;

    i32 graphics_queue_index;
    i32 present_queue_index;
    i32 transfer_queue_index;

    VkQueue graphics_queue;
    VkQueue present_queue;
    VkQueue transfer_queue;

    VkCommandPool graphics_command_pool;

    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceFeatures features;
    VkPhysicalDeviceMemoryProperties memory;

    VkFormat depth_format;

    bool supports_device_local_host_visible;
} VulkanDevice;

typedef struct VulkanImage 
{
    VkImage image;
    VkDeviceMemory memory;
    VkImageView view;
    u32 width;
    u32 height;
} VulkanImage;

typedef enum VulkanRenderPassState
{
    VULKAN_RENDER_PASS_STATE_READY,
    VULKAN_RENDER_PASS_STATE_RECORDING,
    VULKAN_RENDER_PASS_STATE_IN_RENDER_PASS,
    VULKAN_RENDER_PASS_STATE_RECORDING_FINISHED,
    VULKAN_RENDER_PASS_STATE_SUBMITTED,
    VULKAN_RENDER_PASS_STATE_NOT_ALLOCATED
}  VulkanRenderPassState;

typedef struct VulkanRenderPass
{
    VkRenderPass render_pass;
    f32 x, y, w, h;
    f32 r, g, b, a;

    f32 depth;
    u32 stencil;

    VulkanRenderPassState state;
} VulkanRenderPass;

typedef struct VulkanFramebuffer
{
    VkFramebuffer framebuffer;
    u32 attachment_count;
    VkImageView* attachments;
    VulkanRenderPass* render_pass;
} VulkanFramebuffer;

typedef struct VulkanSwapchain
{
    VkSurfaceFormatKHR image_format;
    u8 max_frames_in_flight;
    VkSwapchainKHR swapchain;
    u32 image_count;
    VkImage* images;
    VkImageView* image_views;

    VulkanImage depth_attachment;

    VulkanFramebuffer* framebuffers;
} VulkanSwapchain;

typedef enum VulkanCommandBufferState
{
    VULKAN_COMMAND_BUFFER_STATE_READY,
    VULKAN_COMMAND_BUFFER_STATE_RECORDING,
    VULKAN_COMMAND_BUFFER_STATE_IN_RENDER_PASS,
    VULKAN_COMMAND_BUFFER_STATE_RECORDING_FINISHED,
    VULKAN_COMMAND_BUFFER_STATE_SUBMITTED,
    VULKAN_COMMAND_BUFFER_STATE_NOT_ALLOCATED
} VulkanCommandBufferState;

typedef struct VulkanCommandBuffer
{
    VkCommandBuffer command_buffer;
    VulkanCommandBufferState state;
} VulkanCommandBuffer;

typedef struct VulkanFence
{
    VkFence fence;
    bool signaled;
} VulkanFence;

typedef struct VulkanShaderStage
{
    VkShaderModuleCreateInfo create_info;
    VkShaderModule module;
    VkPipelineShaderStageCreateInfo stage_info;
} VulkanShaderStage;

typedef struct VulkanPipeline
{
    VkPipeline pipeline;
    VkPipelineLayout layout;
} VulkanPipeline;

typedef struct VulkanDescriptorState
{
    // One per frame
    u32 generations[3];
} VulkanDescriptorState;

typedef struct VulkanObjShaderState
{
    // Per frame
    VkDescriptorSet descriptor_sets[3];

    // Per descriptor
    VulkanDescriptorState descriptor_states[OBJECT_SHADER_DESCRIPTOR_COUNT];
} VulkanObjShaderState;

typedef struct VulkanMaterialShader
{
    VulkanShaderStage stages[OBJECT_SHADER_STAGE_COUNT];
    VulkanPipeline pipeline;

    VkDescriptorPool global_descriptor_pool;
    VkDescriptorSetLayout global_descriptor_set_layout;

    VkDescriptorSet global_descriptor_set[3];

    GlobalUniform global_uniform;
    VulkanBuffer global_uniform_buffer;

    VkDescriptorPool local_descriptor_pool;
    VkDescriptorSetLayout local_descriptor_set_layout;
    VulkanBuffer local_uniform_buffer;
    u64 local_uniform_buffer_index;

    VulkanObjShaderState object_states[MAX_OBJECT_COUNT];

    Texture* default_diffuse;
} VulkanMaterialShader;

typedef i32 (*VulkanFindMemoryIndex)(u32 type_filter, u32 property_flags);

typedef struct VulkanContext 
{
    f64 frame_delta_time;

    u32 framebuffer_width;
    u32 framebuffer_height;
    u32 framebuffer_size_generated;
    u32 framebuffer_last_size_generated;

    VkInstance instance;
    VkAllocationCallbacks* allocator;
    VkSurfaceKHR surface;

#if defined(_DEBUG)
    VkDebugUtilsMessengerEXT debug_messenger;
#endif

    VulkanDevice device;

    VulkanSwapchain swapchain;
    u32 image_index;
    u32 current_frame;
    bool recreating_swapchain;

    VulkanFindMemoryIndex find_memory_index;
    VulkanRenderPass main_render_pass;
    VulkanCommandBuffer* graphics_command_buffers;

    VulkanBuffer obj_vertex_buffer;
    VulkanBuffer obj_index_buffer;

    VkSemaphore* image_available_semaphores;
    VkSemaphore* queue_complete_semaphores;

    u32 in_flight_fence_count;
    VulkanFence* in_flight_fences;
    VulkanFence** images_in_flight;

    VulkanMaterialShader material_shader;

    u64 geometry_vertex_offset;
    u64 geometry_index_offset;
} VulkanContext;

typedef struct VulkanTexture
{
    VulkanImage image;
    VkSampler sampler;
} VulkanTexture;