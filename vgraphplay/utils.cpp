// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#include "utils.h"

std::ostream& operator<<(std::ostream &stream, VkPhysicalDeviceProperties &props) {
  stream << "[ Name: " << props.deviceName
         << ", API version: "
         << VK_VERSION_MAJOR(props.apiVersion) << "."
         << VK_VERSION_MINOR(props.apiVersion) << "."
         << VK_VERSION_PATCH(props.apiVersion)
         << ", Driver version: "
         << VK_VERSION_MAJOR(props.driverVersion) << "."
         << VK_VERSION_MINOR(props.driverVersion) << "."
         << VK_VERSION_PATCH(props.driverVersion)
         << ", Device type: " << props.deviceType
         << ", Vendor ID: " << props.vendorID
         << ", Device ID: " << props.deviceID << " ]";
}

std::ostream& operator<<(std::ostream &stream, VkPhysicalDeviceType &device_type) {
    switch (device_type) {
    case VK_PHYSICAL_DEVICE_TYPE_OTHER:
        return stream << "Other";
    case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
        return stream << "Integrated GPU";
    case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
        return stream << "Discrete GPU";
    case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
        return stream << "Virtual GPU";
    case VK_PHYSICAL_DEVICE_TYPE_CPU:
        return stream << "CPU";
    default:
        return stream << "Unknown: " << device_type;
    }
}
