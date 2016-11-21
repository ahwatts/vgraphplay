// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#include "VulkanOutput.h"

namespace vgraphplay {

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


    std::ostream& operator<<(std::ostream &stream, const VkExtensionProperties &props) {
        stream << "[ Name: " << props.extensionName
               << ", Spec version: "
               << VK_VERSION_MAJOR(props.specVersion) << "."
               << VK_VERSION_MINOR(props.specVersion) << "."
               << VK_VERSION_PATCH(props.specVersion)
               << " ]";
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
               << VK_VERSION_PATCH(props.driverVersion)
               << ", Device type: " << props.deviceType
               << ", Vendor ID: " << props.vendorID
               << ", Device ID: " << props.deviceID << " ]";
        return stream;
    }

    std::ostream& operator<<(std::ostream &stream, const VkPhysicalDeviceType &device_type) {
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

    std::ostream& operator<<(std::ostream &stream, const VkQueueFamilyProperties &props) {
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

    std::ostream& operator<<(std::ostream &stream, const VkMemoryType &mem_type) {
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

    std::ostream& operator<<(std::ostream &stream, const VkMemoryHeap &mem_heap) {
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

    std::ostream& outputSurfaceTransformFlags(std::ostream &stream, const VkSurfaceTransformFlagsKHR &flags) {
        stream << "[";
    
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

        return stream << " ]";
    }

    std::ostream& outputCompositeAlphaFlags(std::ostream &stream, const VkCompositeAlphaFlagsKHR &flags) {
        stream << "[";

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

        return stream << " ]";
    }

    std::ostream& outputImageUsageFlags(std::ostream &stream, const VkImageUsageFlags &flags) {
        stream << "[";

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

        return stream << " ]";
    }
}
