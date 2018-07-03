// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#ifndef _VGRAHPLAY_VGRAPHPLAY_GFX_VULKAN_OUTPUT_H_
#define _VGRAHPLAY_VGRAPHPLAY_GFX_VULKAN_OUTPUT_H_

#include <string>

#include "../vulkan.h"

namespace vgraphplay {
    void logGlobalExtensions();
    void logGlobalLayers();
    void logPhysicalDevices(VkInstance instance);
    void logSurfaceCapabilities(VkPhysicalDevice device, VkSurfaceKHR surface);

    // Structs.
    std::ostream& operator<<(std::ostream&, const VkExtensionProperties&);
    std::ostream& operator<<(std::ostream&, const VkLayerProperties&);
    std::ostream& operator<<(std::ostream&, const VkPhysicalDeviceProperties&);
    std::ostream& operator<<(std::ostream&, const VkQueueFamilyProperties&);
    std::ostream& operator<<(std::ostream&, const VkMemoryType&);
    std::ostream& operator<<(std::ostream&, const VkMemoryHeap&);
    std::ostream& operator<<(std::ostream&, const VkPresentModeKHR&);
    std::ostream& operator<<(std::ostream&, const VkSurfaceCapabilitiesKHR&);

    // Enums.
    std::ostream& outputPhysicalDeviceType(std::ostream&, const VkPhysicalDeviceType&);

    // Flags.
    std::ostream& outputQueueFlags(std::ostream&, const VkQueueFlags&);
    std::ostream& outputMemoryPropertyFlags(std::ostream&, const VkMemoryPropertyFlags&);
    std::ostream& outputMemoryHeapFlags(std::ostream&, const VkMemoryHeapFlags&);
    std::ostream& outputSurfaceTransformFlags(std::ostream&, const VkSurfaceTransformFlagsKHR&);
    std::ostream& outputCompositeAlphaFlags(std::ostream&, const VkCompositeAlphaFlagsKHR&);
    std::ostream& outputImageUsageFlags(std::ostream&, const VkImageUsageFlags&);
}

#endif
