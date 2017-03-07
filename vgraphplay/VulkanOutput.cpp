// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#include "VulkanOutput.h"

#include <sstream>
#include <vector>

#ifndef _WIN32
#define BOOST_LOG_DYN_LINK
#endif
#include <boost/log/trivial.hpp>

namespace vgraphplay {
    void logGlobalExtensions() {
        uint32_t num_extensions;
        vkEnumerateInstanceExtensionProperties(nullptr, &num_extensions, nullptr);
        std::vector<VkExtensionProperties> extensions(num_extensions);
        vkEnumerateInstanceExtensionProperties(nullptr, &num_extensions, extensions.data());

        for (const auto& extension : extensions) {
            std::ostringstream msg;
            msg << "Extension: " << extension;
            BOOST_LOG_TRIVIAL(trace) << msg.str();
        }
    }

    void logGlobalLayers() {
        uint32_t num_layers;
        vkEnumerateInstanceLayerProperties(&num_layers, nullptr);
        std::vector<VkLayerProperties> layers(num_layers);
        vkEnumerateInstanceLayerProperties(&num_layers, layers.data());

        for (const auto& layer : layers) {
            std::ostringstream msg;
            
            msg << "Layer: " << layer;

            uint32_t num_extensions;
            vkEnumerateInstanceExtensionProperties(layer.layerName, &num_extensions, nullptr);
            std::vector<VkExtensionProperties> extensions(num_extensions);
            vkEnumerateInstanceExtensionProperties(layer.layerName, &num_extensions, extensions.data());

            for (const auto& extension : extensions) {
                msg << std::endl << "  Extension: " << extension;
            }

            BOOST_LOG_TRIVIAL(trace) << msg.str();
        }
    }

    void logPhysicalDevices(VkInstance instance) {
        uint32_t num_devices = 0;
        vkEnumeratePhysicalDevices(instance, &num_devices, nullptr);
        std::vector<VkPhysicalDevice> devices(num_devices);
        vkEnumeratePhysicalDevices(instance, &num_devices, devices.data());

        for (const auto& device : devices) {
            std::ostringstream msg;

            VkPhysicalDeviceProperties props;
            vkGetPhysicalDeviceProperties(device, &props);

            msg << "Physical device: (" << device << ") " << props;

            uint32_t num_extensions;
            vkEnumerateDeviceExtensionProperties(device, nullptr, &num_extensions, nullptr);
            std::vector<VkExtensionProperties> extensions(num_extensions);
            vkEnumerateDeviceExtensionProperties(device, nullptr, &num_extensions, extensions.data());

            for (auto&& extension : extensions) {
                msg << std::endl << "  Extension: " << extension;
            }

            VkPhysicalDeviceMemoryProperties mem_props;
            vkGetPhysicalDeviceMemoryProperties(device, &mem_props);

            for (unsigned int i = 0; i < mem_props.memoryHeapCount; ++i) {
                msg << std::endl << "  Memory heap " << i << ": " << mem_props.memoryHeaps[i];
            }

            for (unsigned int i = 0; i < mem_props.memoryTypeCount; ++i) {
                msg << std::endl << "  Memory type " << i << ": " << mem_props.memoryTypes[i];
            }

            uint32_t num_queue_families = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(device, &num_queue_families, nullptr);
            std::vector<VkQueueFamilyProperties> queue_families(num_queue_families);
            vkGetPhysicalDeviceQueueFamilyProperties(device, &num_queue_families, queue_families.data());

            for (unsigned int i = 0; i < queue_families.size(); ++i) {
                msg << std::endl << "  Queue family " << i << ": " << queue_families[i];

                int supports_present = glfwGetPhysicalDevicePresentationSupport(instance, device, i);
                msg << " Can present: " << (supports_present == GLFW_TRUE);
            }

            BOOST_LOG_TRIVIAL(trace) << msg.str();
        }
    }

    void logSurfaceCapabilities(VkPhysicalDevice device, VkSurfaceKHR surface) {
        std::ostringstream msg;
        msg << "Physical device " << device << ", surface " << surface;

        VkSurfaceCapabilitiesKHR surf_caps;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &surf_caps);
        msg << std::endl << "  Capabilities: " << surf_caps;

        uint32_t num_formats;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &num_formats, nullptr);
        std::vector<VkSurfaceFormatKHR> formats(num_formats);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &num_formats, formats.data());

        for (auto&& format : formats) {
            msg << std::endl << "  Surface format: [ format: " << format.format << " color space: " << format.colorSpace << " ]";
        }

        uint32_t num_modes;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &num_modes, nullptr);
        std::vector<VkPresentModeKHR> modes(num_modes);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &num_modes, modes.data());

        for (auto&& mode : modes) {
            msg << std::endl << "  Surface present mode: " << mode;
        }

        BOOST_LOG_TRIVIAL(trace) << msg.str();
    }

    std::ostream& operator<<(std::ostream &stream, const VkExtensionProperties &props) {
        stream << "[ Name: " << props.extensionName
               << ", Spec version: "
               << VK_VERSION_MAJOR(props.specVersion) << "."
               << VK_VERSION_MINOR(props.specVersion) << "."
               << VK_VERSION_PATCH(props.specVersion)
               << " ]";
        return stream;
    }

    std::ostream& operator<<(std::ostream &stream, const VkLayerProperties &props) {
        stream << "[ Name: " << props.layerName
               << ", Spec version: "
               << VK_VERSION_MAJOR(props.specVersion) << "."
               << VK_VERSION_MINOR(props.specVersion) << "."
               << VK_VERSION_PATCH(props.specVersion)
               << ", Implementation version: "
               << VK_VERSION_MAJOR(props.implementationVersion) << "."
               << VK_VERSION_MINOR(props.implementationVersion) << "."
               << VK_VERSION_PATCH(props.implementationVersion)
               << ", Description: " << props.description << " ]";
        return stream;
    }

    std::ostream& operator<<(std::ostream &stream, const VkPhysicalDeviceProperties &props) {
        stream << "[ Name: " << props.deviceName
               << ", API version: "
               << VK_VERSION_MAJOR(props.apiVersion) << "."
               << VK_VERSION_MINOR(props.apiVersion) << "."
               << VK_VERSION_PATCH(props.apiVersion)
               << ", Driver version: "
               << VK_VERSION_MAJOR(props.driverVersion) << "."
               << VK_VERSION_MINOR(props.driverVersion) << "."
               << VK_VERSION_PATCH(props.driverVersion);

        stream << ", Device type: ";
        outputPhysicalDeviceType(stream, props.deviceType);

        stream << ", Vendor ID: " << props.vendorID
               << ", Device ID: " << props.deviceID << " ]";
        return stream;
    }

    std::ostream& operator<<(std::ostream &stream, const VkQueueFamilyProperties &props) {
        stream << "[ Flags:";
        outputQueueFlags(stream, props.queueFlags);

        stream << ", Queue count: " << props.queueCount
               << ", Valid timestamp bits: " << props.timestampValidBits
               << " ]";

        return stream;
    }

    std::ostream& operator<<(std::ostream &stream, const VkMemoryType &mem_type) {
        stream << "[ Heap index: " << mem_type.heapIndex;

        stream << ", Property flags: ";
        outputMemoryPropertyFlags(stream, mem_type.propertyFlags);

        return stream << " ]";
    }

    std::ostream& operator<<(std::ostream &stream, const VkMemoryHeap &mem_heap) {
        stream << "[ Size: " << (mem_heap.size / 1048576) << " MiB";
        
        stream << ", Flags: ";
        outputMemoryHeapFlags(stream, mem_heap.flags);

        return stream << " ]";
    }

    std::ostream& operator<<(std::ostream &stream, const VkSurfaceCapabilitiesKHR &surf_caps) {
        stream << "[ Min image count: " << surf_caps.minImageCount
               << ", Max image count: " << surf_caps.maxImageCount
               << ", Current extent: " << surf_caps.currentExtent.width << "x" << surf_caps.currentExtent.height
               << ", Min image extent: " << surf_caps.minImageExtent.width << "x" << surf_caps.minImageExtent.height
               << ", Max image extent: " << surf_caps.maxImageExtent.width << "x" << surf_caps.maxImageExtent.height
               << ", Max image array layers: " << surf_caps.maxImageArrayLayers;

        stream << ", Supported transforms: ";
        outputSurfaceTransformFlags(stream, surf_caps.supportedTransforms);

        stream << ", Current transform: ";
        outputSurfaceTransformFlags(stream, static_cast<VkSurfaceTransformFlagsKHR>(surf_caps.currentTransform));

        stream << ", Supported alpha compositing: ";
        outputCompositeAlphaFlags(stream, surf_caps.supportedCompositeAlpha);

        stream << ", Supported image usages: ";
        outputImageUsageFlags(stream, surf_caps.supportedUsageFlags);

        return stream << " ]";
    }

    std::ostream& operator<<(std::ostream &stream, const VkPresentModeKHR &mode) {
        switch (mode) {
        case VK_PRESENT_MODE_IMMEDIATE_KHR:
            return stream << "Immediate";
        case VK_PRESENT_MODE_MAILBOX_KHR:
            return stream << "Mailbox";
        case VK_PRESENT_MODE_FIFO_KHR:
            return stream << "FIFO";
        case VK_PRESENT_MODE_FIFO_RELAXED_KHR:
            return stream << "Relaxed FIFO";
        default:
            return stream << mode;
        }
    }

    std::ostream& outputPhysicalDeviceType(std::ostream &stream, const VkPhysicalDeviceType &device_type) {
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

    std::ostream& outputQueueFlags(std::ostream &stream, const VkQueueFlags &flags) {
        stream << "(" << flags << ") [";

        if (flags == 0) {
            stream << " None";
        } else {
            if (flags & VK_QUEUE_GRAPHICS_BIT) {
                stream << " Graphics,";
            }

            if (flags & VK_QUEUE_COMPUTE_BIT) {
                stream << " Compute,";
            }

            if (flags & VK_QUEUE_TRANSFER_BIT) {
                stream << " Transfer,";
            }

            if (flags & VK_QUEUE_SPARSE_BINDING_BIT) {
                stream << " Sparse Binding,";
            }
        }

        return stream << " ]";
    }

    std::ostream& outputMemoryPropertyFlags(std::ostream &stream, const VkMemoryPropertyFlags &flags) {
        stream << "(" << flags << ") [";

        if (flags == 0) {
            stream << " None";
        } else {
            if (flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
                stream << " Device Local,";
            }

            if (flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
                stream << " Host Visible,";
            }

            if (flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) {
                stream << " Host Coherent,";
            }

            if (flags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) {
                stream << " Host Cached,";
            }

            if (flags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT) {
                stream << " Lazily,Allocated,";
            }
        }

        return stream << " ]";
    }

    std::ostream& outputMemoryHeapFlags(std::ostream &stream, const VkMemoryHeapFlags &flags) {
        stream << "(" << flags << ") [";

        if (flags == 0) {
            stream << " None";
        } else {
            if (flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
                stream << " Device Local,";
            }
        }

        return stream << " ]";
    }

    std::ostream& outputSurfaceTransformFlags(std::ostream &stream, const VkSurfaceTransformFlagsKHR &flags) {
        stream << "(" << flags << ") [";

        if (flags == 0) {
            stream << " None";
        } else {
            if (flags & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
                stream << " Identity,";
            }

            if (flags & VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR) {
                stream << " Rotate 90 degrees,";
            }

            if (flags & VK_SURFACE_TRANSFORM_ROTATE_180_BIT_KHR) {
                stream << " Rotate 180 degrees,";
            }

            if (flags & VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR) {
                stream << " Rotate 270 degrees,";
            }

            if (flags & VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_BIT_KHR) {
                stream << "Horizontal mirror,";
            }

            if (flags & VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_90_BIT_KHR) {
                stream << " Rotate horizontal mirror 90 degrees,";
            }

            if (flags & VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_180_BIT_KHR) {
                stream << " Rotate horizontal mirror 90 degrees,";
            }

            if (flags & VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_270_BIT_KHR) {
                stream << " Rotate horizontal mirror 90 degrees,";
            }

            if (flags & VK_SURFACE_TRANSFORM_INHERIT_BIT_KHR) {
                stream << " Inherit,";
            }
        }

        return stream << " ]";
    }

    std::ostream& outputCompositeAlphaFlags(std::ostream &stream, const VkCompositeAlphaFlagsKHR &flags) {
        stream << "(" << flags << ") [";

        if (flags == 0) {
            stream << " None";
        } else {
            if (flags & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR) {
                stream << " Opaque,";
            }

            if (flags & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR) {
                stream << " Pre-multiplied,";
            }

            if (flags & VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR) {
                stream << " Post-multiplied,";
            }

            if (flags & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR) {
                stream << " Inherit,";
            }
        }

        return stream << " ]";
    }

    std::ostream& outputImageUsageFlags(std::ostream &stream, const VkImageUsageFlags &flags) {
        stream << "(" << flags << ") [";

        if (flags == 0) {
            stream << " None";
        } else {
            if (flags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) {
                stream << " Transfer source,";
            }

            if (flags & VK_IMAGE_USAGE_TRANSFER_DST_BIT) {
                stream << " Transfer destination,";
            }

            if (flags & VK_IMAGE_USAGE_SAMPLED_BIT) {
                stream << " Sampled,";
            }

            if (flags & VK_IMAGE_USAGE_STORAGE_BIT) {
                stream << " Storage,";
            }

            if (flags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) {
                stream << " Color attachment,";
            }

            if (flags & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
                stream << " Depth / Stencil attachment,";
            }

            if (flags & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT) {
                stream << " Transient attachment,";
            }

            if (flags & VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT) {
                stream << " Input attachment,";
            }
        }

        return stream << " ]";
    }
}
