#pragma once

#include <vulkan/vulkan.h>

#include "defines.h"
#include "core/asserts.h"
#include "renderer/renderer_defines.h"
#include "lib/math/math_defines.h"
#include "lib/memory/freelist.h"
#include "lib/containers/hash_table.h"

#define MAX_INDICES 32
#define MAX_PHYSICAL_DEVICES 32
#define MAX_QUEUE_FAMILIES 32

#define VULKAN_MAX_MATERIAL_COUNT 1024
#define MAX_UI_COUNT 1024

#define VULKAN_SHADER_MAX_STAGES 8
#define VULKAN_SHADER_MAX_ATTRIBUTES 16
#define VULKAN_SHADER_MAX_GLOBAL_TEXTURES 31
#define VULKAN_SHADER_MAX_INSTANCE_TEXTURES 31
#define VULKAN_SHADER_MAX_UNIFORMS 128
#define VULKAN_SHADER_MAX_BINDINGS 2
#define VULKAN_SHADER_MAX_PUSH_CONST_RANGES 32

#define MAX_GEOMETRY_COUNT 4096

#define DYNAMIC_STATE_COUNT 3

#define VK_ASSERT(expr) do { kz_assert((expr) == VK_SUCCESS); } while(0)

struct VulkanContext;

typedef struct VulkanBuffer 
{
    u64 size;
    VkBuffer buffer;
    VkBufferUsageFlagBits usage;
    VkDeviceMemory memory;
    bool locked;
    i32 memory_index;
    u32 memory_property_flags;

    // For dynamic buffers
    FreeList free_list;
    u64 free_list_size;
    void* freelist_memory;
    bool has_freelist;
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
    Vec4 render_area;
    Vec4 clear_color;

    f32 depth;
    u32 stencil;

    u8 clear_flags;
    bool has_prev_pass;
    bool has_next_pass;

    VulkanRenderPassState state;
} VulkanRenderPass;

typedef struct VulkanSwapchain
{
    VkSurfaceFormatKHR image_format;
    u8 max_frames_in_flight;
    VkSwapchainKHR swapchain;
    u32 image_count;
    VkImage* images;
    VkImageView* image_views;

    VulkanImage depth_attachment;

    VkFramebuffer framebuffers[3];
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

typedef struct VulkanGeometryData
{
    u64 id;
    u32 generation;
    
    u64 vertex_count;
    u32 vertex_element_size;
    u64 vertex_buffer_offset;

    u64 index_count;
    u32 index_element_size;
    u64 index_buffer_offset;
} VulkanGeometryData;

typedef struct VulkanShaderStageConfig
{
    VkShaderStageFlagBits stage;
    char file_name[255];
} VulkanShaderStageConfig;

typedef struct VulkanDescriptorSetConfig
{
    u8 binding_count;
    VkDescriptorSetLayoutBinding bindings[VULKAN_SHADER_MAX_BINDINGS];
} VulkanDescriptorSetConfig;

typedef struct VulkanShaderConfig
{
    u8 stage_count;
    VulkanShaderStageConfig stages[VULKAN_SHADER_MAX_STAGES];
    VkDescriptorPoolSize pool_sizes[2];

    u16 max_descriptor_set_count;
    u8 descriptor_set_count;
    VulkanDescriptorSetConfig descriptor_sets[2];

    VkVertexInputAttributeDescription attributes[VULKAN_SHADER_MAX_ATTRIBUTES];
} VulkanShaderConfig;

typedef struct VulkanDescriptorState
{
    u8 generations[3];
    u32 ids[3];
} VulkanDescriptorState;

typedef struct VulkanShaderDescriptorSetState
{
    // Per frame
    VkDescriptorSet descriptor_sets[3];

    // Per descriptor
    VulkanDescriptorState descriptor_states[VULKAN_SHADER_MAX_BINDINGS];
} VulkanShaderDescriptorSetState;

typedef struct VulkanShaderInstanceState
{
    u64 id;
    u64 offset;
    VulkanShaderDescriptorSetState descriptor_set_state;

    struct Texture** instance_textures;
} VulkanShaderInstanceState;

typedef struct VulkanShader
{
    void* uniform_buffer_block;

    u64 id;
    VulkanShaderConfig config;
    VulkanRenderPass* render_pass;
    VulkanShaderStage stages[VULKAN_SHADER_MAX_STAGES];
    VkDescriptorPool descriptor_pool;
    VkDescriptorSetLayout descriptor_set_layouts[2];
    VkDescriptorSet global_descriptor_sets[3];
    VulkanBuffer uniform_buffer;
    VulkanPipeline pipeline;

    u64 instance_count;
    VulkanShaderInstanceState instance_states[VULKAN_MAX_MATERIAL_COUNT];
} VulkanShader;

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
    VulkanRenderPass ui_render_pass;
    
    VulkanCommandBuffer* graphics_command_buffers;

    VulkanBuffer obj_vertex_buffer;
    VulkanBuffer obj_index_buffer;

    VkSemaphore* image_available_semaphores;
    VkSemaphore* queue_complete_semaphores;

    u32 in_flight_fence_count;
    VkFence in_flight_fences[2];
    VkFence images_in_flight[3]; // One per frame

    VulkanGeometryData geometries[MAX_GEOMETRY_COUNT];

    VkFramebuffer world_framebuffers[3]; // One per frame
} VulkanContext;

typedef struct VulkanTexture
{
    VulkanImage image;
    VkSampler sampler;
} VulkanTexture;