// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#ifndef _VGRAHPLAY_VGRAPHPLAY_UTILS_H_
#define _VGRAHPLAY_VGRAPHPLAY_UTILS_H_

#include <iostream>

#include <vulkan/vulkan.h>

std::ostream& operator<<(std::ostream &stream, VkLayerProperties &layer_props);
std::ostream& operator<<(std::ostream &stream, VkExtensionProperties &layer_props);
std::ostream& operator<<(std::ostream &stream, VkPhysicalDeviceProperties &device_props);
std::ostream& operator<<(std::ostream &stream, VkPhysicalDeviceType &device_type);
std::ostream& operator<<(std::ostream &stream, VkQueueFamilyProperties &queue_family_props);
std::ostream& operator<<(std::ostream &stream, VkMemoryType &mem_type);
std::ostream& operator<<(std::ostream &stream, VkMemoryHeap &mem_heap);
std::ostream& operator<<(std::ostream &stream, VkSurfaceCapabilitiesKHR &surf_caps);
std::ostream& operator<<(std::ostream &stream, VkPresentModeKHR &modes);

#endif
