// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#include <format>
#include <sstream>
#include <string>
#include <vector>

#include <boost/log/trivial.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "vulkan.h"

#include "VulkanOutput.h"

namespace vgraphplay {
    std::string versionMessage(uint32_t version) {
        return std::format(
            "{}.{}.{}",
            VK_VERSION_MAJOR(version),
            VK_VERSION_MINOR(version),
            VK_VERSION_PATCH(version)
        );
    }

    std::string extensionMessage(const vk::ExtensionProperties &ext_props) {
        return std::format(
            "{}, spec version {}", 
            ext_props.extensionName.data(), 
            versionMessage(ext_props.specVersion)
        );
    }

    std::string layerMessage(const vk::LayerProperties &layer_props) {
        return std::format(
            "{}, spec version: {}, implementation version: {} - {}", 
            layer_props.layerName.data(),
            versionMessage(layer_props.specVersion),
            versionMessage(layer_props.implementationVersion),
            layer_props.description.data()
        );
    }

    void logInstanceExtensions(const vk::raii::Context &context) {
        const std::vector<vk::ExtensionProperties> extensions = context.enumerateInstanceExtensionProperties();
        std::string msg = std::format("There are {} instance extensions:", extensions.size());
        for (const auto& extension : extensions) {
            msg += "\n  - " + extensionMessage(extension);
        }
        BOOST_LOG_TRIVIAL(trace) << msg;
    }

    void logInstanceLayers(const vk::raii::Context &context) {
        const std::vector<vk::LayerProperties> layers = context.enumerateInstanceLayerProperties();
        std::string msg = std::format("There are {} instance layers:", layers.size());
        for (const auto& layer : layers) {
            msg += "\n  - " + layerMessage(layer);

            const std::vector<vk::ExtensionProperties> layer_extensions = 
                context.enumerateInstanceExtensionProperties(std::string{layer.layerName.cbegin(), layer.layerName.cend()});

            if (!layer_extensions.empty()) {
                msg += std::format(", with {} layer extensions:", layer_extensions.size());
                for (const auto& extension : layer_extensions) {
                    msg += "\n    - " + extensionMessage(extension);
                }
            }
        }

        BOOST_LOG_TRIVIAL(trace) << msg;
    }

    void logPhysicalDevices(const vk::raii::Instance &instance) {
        std::vector<vk::raii::PhysicalDevice> devices = instance.enumeratePhysicalDevices();
        std::string msg = std::format("There are {} physical device(s):", devices.size());

        for (const auto &device : devices) {
            auto props = device.getProperties();
            auto features = device.getFeatures();

            msg += std::format(
                "\n  - Name: {}\n    Type: {}\n    Device ID: {}\n    Vendor ID: {}\n    API Version: {}\n    Driver Version: {}",
                props.deviceName.data(),
                vk::to_string(props.deviceType),
                props.deviceID,
                props.vendorID,
                versionMessage(props.apiVersion),
                versionMessage(props.driverVersion)
            );

            const std::vector<vk::ExtensionProperties> extensions = device.enumerateDeviceExtensionProperties();
            msg += std::format("\n    Supports {} device extensions:", extensions.size());
            for (const auto& extension : extensions) {
                msg += "\n    - " + extensionMessage(extension);
            }

            const std::vector<vk::LayerProperties> layers = device.enumerateDeviceLayerProperties();
            msg += std::format("\n    Supports {} device layers:", layers.size());
            for (const auto &layer : layers) {
                msg += "\n    - " + layerMessage(layer);

                const std::vector<vk::ExtensionProperties> layer_extensions = 
                    device.enumerateDeviceExtensionProperties(std::string{layer.layerName.cbegin(), layer.layerName.cend()});

                if (!layer_extensions.empty()) {
                    msg += std::format(", with {} layer extensions:", layer_extensions.size());
                    for (const auto& extension : layer_extensions) {
                        msg += "\n      - " + extensionMessage(extension);
                    }
                }
            }
        }

        BOOST_LOG_TRIVIAL(trace) << msg;
        /*
        for (const auto& device : devices) {

            VkPhysicalDeviceMemoryProperties mem_props;
            vkGetPhysicalDeviceMemoryProperties(device, &mem_props);

            for (unsigned int i = 0; i < mem_props.memoryHeapCount; ++i) {
                msg << std::endl << "    Memory heap " << i << ": " << mem_props.memoryHeaps[i];
            }

            for (unsigned int i = 0; i < mem_props.memoryTypeCount; ++i) {
                msg << std::endl << "    Memory type " << i << ": " << mem_props.memoryTypes[i];
            }

            uint32_t num_queue_families = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(device, &num_queue_families, nullptr);
            std::vector<VkQueueFamilyProperties> queue_families(num_queue_families);
            vkGetPhysicalDeviceQueueFamilyProperties(device, &num_queue_families, queue_families.data());

            for (unsigned int i = 0; i < queue_families.size(); ++i) {
                msg << std::endl << "    Queue family " << i << ": " << queue_families[i];

                int supports_present = glfwGetPhysicalDevicePresentationSupport(instance, device, i);
                msg << " Can present: " << (supports_present == GLFW_TRUE);
            }
        }

        BOOST_LOG_TRIVIAL(trace) << msg.str(); */
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
        case VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR:
            return stream << "Shared demand refresh";
        case VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR:
            return stream << "Shared continuous refresh";
        case 1000361000: // VK_PRESENT_MODE_FIFO_LATEST_READY_KHR
            return stream << "FIFO latest ready";
        default:
            return stream << "Unknown: " << ((uint64_t)mode);
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
            return stream << "Unknown: " << ((uint64_t)device_type);
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
