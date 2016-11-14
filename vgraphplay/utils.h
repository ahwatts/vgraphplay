// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#include <iostream>

#include <vulkan/vulkan.h>

std::ostream& operator<<(std::ostream &stream, VkPhysicalDeviceProperties &device_props);
std::ostream& operator<<(std::ostream &stream, VkPhysicalDeviceType &device_type);
std::ostream& operator<<(std::ostream &stream, VkQueueFamilyProperties &queue_family_props);
std::ostream& operator<<(std::ostream &stream, VkMemoryType &mem_type);
std::ostream& operator<<(std::ostream &stream, VkMemoryHeap &mem_heap);
