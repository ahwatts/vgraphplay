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
  return stream;
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

std::ostream& operator<<(std::ostream &stream, VkQueueFamilyProperties &props) {
    stream << "[ Flags:";

    if (props.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
        stream << " Graphics";
    }

    if (props.queueFlags & VK_QUEUE_COMPUTE_BIT) {
        stream << " Compute";
    }

    if (props.queueFlags & VK_QUEUE_TRANSFER_BIT) {
        stream << " Transfer";
    }

    if (props.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) {
        stream << " Sparse-Binding";
    }

    stream << ", Queue count: " << props.queueCount
           << ", Valid timestamp bits: " << props.timestampValidBits
           << " ]";

    return stream;
}

std::ostream& operator<<(std::ostream &stream, VkMemoryType &mem_type) {
    stream << "[ Heap index: " << mem_type.heapIndex << ", Flags: " << mem_type.propertyFlags << " (";

    if (mem_type.propertyFlags == 0) {
        stream << " None";
    } else {
        if (mem_type.propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
            stream << " Device-Local";
        }

        if (mem_type.propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
            stream << " Host-Visible";
        }

        if (mem_type.propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) {
            stream << " Host-Coherent";
        }

        if (mem_type.propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) {
            stream << " Host-Cached";
        }

        if (mem_type.propertyFlags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT) {
            stream << " Lazily-Allocated";
        }
    }

    stream << " ) ]";

    return stream;
}

std::ostream& operator<<(std::ostream &stream, VkMemoryHeap &mem_heap) {
    stream << "[ Size: " << (mem_heap.size / 1048576) << " MiB, Flags: " << mem_heap.flags << " (";

    if (mem_heap.flags == 0) {
        stream << " None";
    } else {
        if (mem_heap.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
            stream << " Device-Local";
        }
    }

    stream << " ) ]";

    return stream;
}
