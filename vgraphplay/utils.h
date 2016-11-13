// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#include <iostream>

#include <vulkan/vulkan.h>

std::ostream& operator<<(std::ostream &stream, VkPhysicalDeviceProperties &device_props);
std::ostream& operator<<(std::ostream &stream, VkPhysicalDeviceType &device_type);
