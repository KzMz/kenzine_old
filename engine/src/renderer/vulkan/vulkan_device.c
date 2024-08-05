#include "vulkan_device.h"
#include "core/log.h"
#include "lib/string.h"
#include "core/memory.h"
#include "lib/containers/dyn_array.h"

typedef struct VulkanPhysicalDeviceRequirements 
{
    bool graphics;
    bool present;
    bool compute;
    bool transfer;
    bool sampler_anisotropy;
    bool discrete_gpu;
    const char** device_extension_names;
} VulkanPhysicalDeviceRequirements;

typedef struct VulkanPhysicalDeviceQueueFamilyInfo
{
    u32 graphics_family_index;
    u32 present_family_index;
    u32 compute_family_index;
    u32 transfer_family_index;
} VulkanPhysicalDeviceQueueFamilyInfo;

bool select_physical_device(VulkanContext* context);
bool physical_device_meets_requirements(
    VkPhysicalDevice physical_device,
    VkSurfaceKHR surface,
    const VkPhysicalDeviceProperties* properties,
    const VkPhysicalDeviceFeatures* features,
    const VulkanPhysicalDeviceRequirements* requirements,
    VulkanPhysicalDeviceQueueFamilyInfo* out_queue_info,
    VulkanSwapchainSupportInfo* out_swapchain_support
);

bool vulkan_device_create(VulkanContext* context)
{
    if (!select_physical_device(context))
    {
        log_error("Failed to select a physical device.");
        return false;
    }

    return true;
}

void vulkan_device_destroy(VulkanContext* context)
{

}

bool select_physical_device(VulkanContext* context)
{
    u32 device_count = 0;
    VK_CHECK(vkEnumeratePhysicalDevices(context->instance, &device_count, NULL));
    if (device_count == 0)
    {
        log_fatal("No Vulkan devices found.");
        return false;
    }

    VkPhysicalDevice physical_devices[device_count];
    VK_CHECK(vkEnumeratePhysicalDevices(context->instance, &device_count, physical_devices));

    // TODO: set those as config
    VulkanPhysicalDeviceRequirements requirements = {0};
    requirements.graphics = true;
    requirements.present = true;
    requirements.transfer = true;
    requirements.sampler_anisotropy = true;
    requirements.discrete_gpu = true;
    requirements.device_extension_names = dynarray_create(const char*);
    dynarray_push(requirements.device_extension_names, &VK_KHR_SWAPCHAIN_EXTENSION_NAME);

    for (u32 i = 0; i < device_count; ++i)
    {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(physical_devices[i], &properties);

        VkPhysicalDeviceFeatures features;
        vkGetPhysicalDeviceFeatures(physical_devices[i], &features);

        VkPhysicalDeviceMemoryProperties memory;
        vkGetPhysicalDeviceMemoryProperties(physical_devices[i], &memory);

        VulkanPhysicalDeviceQueueFamilyInfo queue_info = {0};
        bool result = physical_device_meets_requirements(
            physical_devices[i],
            context->surface,
            &properties,
            &features,
            &requirements,
            &queue_info,
            &context->device.swapchain_support
        );

        if (result)
        {
            log_info("Selected physical device: %s", properties.deviceName);

            log_info(
                "GPU Driver version: %d.%d.%d",
                VK_VERSION_MAJOR(properties.driverVersion),
                VK_VERSION_MINOR(properties.driverVersion),
                VK_VERSION_PATCH(properties.driverVersion));

            // Vulkan API version.
            log_info(
                "Vulkan API version: %d.%d.%d",
                VK_VERSION_MAJOR(properties.apiVersion),
                VK_VERSION_MINOR(properties.apiVersion),
                VK_VERSION_PATCH(properties.apiVersion));

            for (u32 j = 0; j < memory.memoryHeapCount; ++j) {
                f32 memory_size_gib = (((f32)memory.memoryHeaps[j].size) / 1024.0f / 1024.0f / 1024.0f);
                if (memory.memoryHeaps[j].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
                    log_info("Local GPU memory: %.2f GiB", memory_size_gib);
                } else {
                    log_info("Shared System memory: %.2f GiB", memory_size_gib);
                }
            }

            context->device.physical_device = physical_devices[i];
            context->device.graphics_queue_index = queue_info.graphics_family_index;
            context->device.present_queue_index = queue_info.present_family_index;
            context->device.transfer_queue_index = queue_info.transfer_family_index;
            // NOTE: set compute index here if needed.

            context->device.properties = properties;
            context->device.features = features;
            context->device.memory = memory;
            break;
        }
    }

    if (!context->device.physical_device)
    {
        log_error("Failed to find a suitable physical device.");
        return false;
    }

    log_info("Physical device selected.");
    return true;
}

bool physical_device_meets_requirements(
    VkPhysicalDevice physical_device,
    VkSurfaceKHR surface,
    const VkPhysicalDeviceProperties* properties,
    const VkPhysicalDeviceFeatures* features,
    const VulkanPhysicalDeviceRequirements* requirements,
    VulkanPhysicalDeviceQueueFamilyInfo* out_queue_info,
    VulkanSwapchainSupportInfo* out_swapchain_support
)
{
    out_queue_info->graphics_family_index = -1;
    out_queue_info->present_family_index = -1;
    out_queue_info->compute_family_index = -1;
    out_queue_info->transfer_family_index = -1;

    if (requirements->discrete_gpu)
    {
        if (properties->deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            return false;
        }
    }

    u32 queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, NULL);
    VkQueueFamilyProperties queue_families[queue_family_count];
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_families);

    log_info("Graphics | Present | Compute | Transfer | Name");
    u8 min_transfer_score = 255;
    for (u32 i = 0; i < queue_family_count; ++i)
    {
        u8 current_transfer_score = 0;

        if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            out_queue_info->graphics_family_index = i;
            ++current_transfer_score;
        }

        if (queue_families[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
        {
            out_queue_info->compute_family_index = i;
            ++current_transfer_score;
        }

        if (queue_families[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
        {
            if (current_transfer_score <= min_transfer_score)
            {
                out_queue_info->transfer_family_index = i;
                min_transfer_score = current_transfer_score;
            }
        }

        VkBool32 present_support = VK_FALSE;
        VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, surface, &present_support));
        if (present_support)
        {
            out_queue_info->present_family_index = i;
        }
    }

    log_info("       %d |       %d |       %d |        %d | %s", 
        out_queue_info->graphics_family_index,
        out_queue_info->present_family_index,
        out_queue_info->compute_family_index,
        out_queue_info->transfer_family_index,
        properties->deviceName
    );

    if (
        (!requirements->graphics || (requirements->graphics && out_queue_info->graphics_family_index != -1)) &&
        (!requirements->present || (requirements->present && out_queue_info->present_family_index != -1)) &&
        (!requirements->compute || (requirements->compute && out_queue_info->compute_family_index != -1)) &&
        (!requirements->transfer || (requirements->transfer && out_queue_info->transfer_family_index != -1))
    ) 
    {
        log_info("Device meets queue requirements.");
        log_trace("Graphics Family Index: %i", out_queue_info->graphics_family_index);
        log_trace("Present Family Index:  %i", out_queue_info->present_family_index);
        log_trace("Transfer Family Index: %i", out_queue_info->transfer_family_index);
        log_trace("Compute Family Index:  %i", out_queue_info->compute_family_index);

        vulkan_device_query_swapchain_support(physical_device, surface, out_swapchain_support);

        if (out_swapchain_support->format_count < 1 || out_swapchain_support->present_mode_count < 1)
        {
            if (out_swapchain_support->formats)
            {
                memory_free(out_swapchain_support->formats, sizeof(VkSurfaceFormatKHR) * out_swapchain_support->format_count, MEMORY_TAG_RENDERER);
            }
            if (out_swapchain_support->present_modes)
            {
                memory_free(out_swapchain_support->present_modes, sizeof(VkPresentModeKHR) * out_swapchain_support->present_mode_count, MEMORY_TAG_RENDERER);
            }
            log_info("Swapchain support not found. Skipping device");
            return false;
        }

        if (requirements->device_extension_names) 
        {
            u32 extension_count = 0;
            VkExtensionProperties* available_extensions = NULL;
            VK_CHECK(vkEnumerateDeviceExtensionProperties(physical_device, NULL, &extension_count, NULL));

            if (extension_count != 0)
            {
                available_extensions = memory_alloc(sizeof(VkExtensionProperties) * extension_count, MEMORY_TAG_RENDERER);
                VK_CHECK(vkEnumerateDeviceExtensionProperties(physical_device, NULL, &extension_count, available_extensions));

                u32 required_extension_count = dynarray_length(requirements->device_extension_names);
                for (u32 i = 0; i < required_extension_count; ++i)
                {
                    bool found = false;
                    for (u32 j = 0; j < extension_count; ++j)
                    {
                        if (string_equals(requirements->device_extension_names[i], available_extensions[j].extensionName))
                        {
                            found = true;
                            break;
                        }
                    }

                    if (!found)
                    {
                        log_info("Device does not support required extension %s", requirements->device_extension_names[i]);
                        memory_free(available_extensions, sizeof(VkExtensionProperties) * extension_count, MEMORY_TAG_RENDERER);
                        return false;
                    }
                }

                memory_free(available_extensions, sizeof(VkExtensionProperties) * extension_count, MEMORY_TAG_RENDERER);
            }

            if (requirements->sampler_anisotropy && !features->samplerAnisotropy)
            {
                log_info("Device does not support sampler anisotropy.");
                return false;
            }

            return true;
        }
    }

    return false;
}

void vulkan_device_query_swapchain_support(
    VkPhysicalDevice physical_device,
    VkSurfaceKHR surface,
    VulkanSwapchainSupportInfo* out_support_info
)
{
    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &out_support_info->capabilities));
    VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &out_support_info->format_count, NULL));

    if (out_support_info->format_count != 0)
    {
        if (!out_support_info->formats)
        {
            out_support_info->formats = memory_alloc(sizeof(VkSurfaceFormatKHR) * out_support_info->format_count, MEMORY_TAG_RENDERER);
        }

        VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &out_support_info->format_count, out_support_info->formats));
    }

    VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &out_support_info->present_mode_count, NULL));

    if (out_support_info->present_mode_count != 0)
    {
        if (!out_support_info->present_modes)
        {
            out_support_info->present_modes = memory_alloc(sizeof(VkPresentModeKHR) * out_support_info->present_mode_count, MEMORY_TAG_RENDERER);
        }

        VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &out_support_info->present_mode_count, out_support_info->present_modes));
    }
}