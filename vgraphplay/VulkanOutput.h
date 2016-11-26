// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#ifndef _VGRAHPLAY_VGRAPHPLAY_VULKAN_OUTPUT_H_
#define _VGRAHPLAY_VGRAPHPLAY_VULKAN_OUTPUT_H_

#include <string>

#include "vulkan.h"

namespace vgraphplay {
    void logGlobalExtensions();
    void logGlobalLayers();

    std::ostream& operator<<(std::ostream&, const VkExtensionProperties&);
    std::ostream& operator<<(std::ostream&, const VkLayerProperties&);
    // std::ostream& operator<<(std::ostream &stream, const VkPhysicalDeviceProperties &device_props);
    // std::ostream& operator<<(std::ostream &stream, const VkPhysicalDeviceType &device_type);
    // std::ostream& operator<<(std::ostream &stream, const VkQueueFamilyProperties &queue_family_props);
    // std::ostream& operator<<(std::ostream &stream, const VkMemoryType &mem_type);
    // std::ostream& operator<<(std::ostream &stream, const VkMemoryHeap &mem_heap);
    // std::ostream& operator<<(std::ostream &stream, const VkPresentModeKHR &modes);
    // std::ostream& operator<<(std::ostream &stream, const VkSurfaceCapabilitiesKHR &surf_caps);

    // std::ostream& outputSurfaceTransformFlags(std::ostream &stream, const VkSurfaceTransformFlagsKHR &modes);
    // std::ostream& outputCompositeAlphaFlags(std::ostream &stream, const VkCompositeAlphaFlagsKHR &modes);
    // std::ostream& outputImageUsageFlags(std::ostream &stream, const VkImageUsageFlags &modes);
}

#endif
