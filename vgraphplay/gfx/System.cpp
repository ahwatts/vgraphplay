// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#include <set>
#include <vector>

#include <boost/log/trivial.hpp>

#include "../vulkan.h"

#include "System.h"
#include "../VulkanOutput.h"

const Resource UNLIT_VERT_BYTECODE = LOAD_RESOURCE(unlit_vert_spv);
const Resource UNLIT_FRAG_BYTECODE = LOAD_RESOURCE(unlit_frag_spv);

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugReportFlagsEXT flags,
    VkDebugReportObjectTypeEXT object_type,
    uint64_t object,
    size_t location,
    int32_t code,
    const char *layer_prefix,
    const char *message,
    void *user_data
) {
    if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
        BOOST_LOG_TRIVIAL(error) << "Vulkan error: " << layer_prefix << ": " << message;
    } else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT) {
        BOOST_LOG_TRIVIAL(warning) << "Vulkan warning: " << layer_prefix << ": " << message;
    } else if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT) {
        BOOST_LOG_TRIVIAL(warning) << "Vulkan performance warning: " << layer_prefix << ": " << message;
    } else if (flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT) {
        BOOST_LOG_TRIVIAL(info) << "Vulkan info: " << layer_prefix << ": " << message;
    } else if (flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT) {
        BOOST_LOG_TRIVIAL(debug) << "Vulkan debug: " << layer_prefix << ": " << message;
    } else {
        BOOST_LOG_TRIVIAL(warning) << "Vulkan unknown level: " << layer_prefix << ": " << message;
    }

    return VK_FALSE;
}

vgraphplay::gfx::System::System(GLFWwindow *window)
    : m_window{window},
      m_instance{VK_NULL_HANDLE},
      m_debug_callback{VK_NULL_HANDLE},
      m_device{VK_NULL_HANDLE},
      m_physical_device{VK_NULL_HANDLE},
      m_graphics_queue_family{0},
      m_present_queue_family{0},
      m_graphics_queue{VK_NULL_HANDLE},
      m_present_queue{VK_NULL_HANDLE},
      m_command_pool{VK_NULL_HANDLE},
      m_command_buffers{},
      m_surface{VK_NULL_HANDLE},
      m_swapchain{VK_NULL_HANDLE},
      m_swapchain_images{},
      m_swapchain_image_views{},
      m_swapchain_format{VK_FORMAT_UNDEFINED, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR},
      m_swapchain_extent{0, 0},
      m_vertex_shader_module{VK_NULL_HANDLE},
      m_fragment_shader_module{VK_NULL_HANDLE},
      m_pipeline_layout{VK_NULL_HANDLE},
      m_render_pass{VK_NULL_HANDLE},
      m_pipeline{VK_NULL_HANDLE},
      m_swapchain_framebuffers{},
      m_image_available_semaphore{VK_NULL_HANDLE},
      m_render_finished_semaphore{VK_NULL_HANDLE}
{}

vgraphplay::gfx::System::~System() {
    dispose();
}

bool vgraphplay::gfx::System::initialize(bool debug) {
    bool rv = initInstance(debug);

    if (rv && debug) {
        rv = rv && initDebugCallback();
    }

    rv = rv && initSurface();
    rv = rv && initDevice();
    rv = rv && initSwapchain();
    rv = rv && initShaderModules();
    rv = rv && initRenderPass();
    rv = rv && initPipelineLayout();
    rv = rv && initPipeline();
    rv = rv && initSwapchainFramebuffers();
    rv = rv && initSemaphores();
    rv = rv && initCommandPool();
    rv = rv && initCommandBuffers();
    rv = rv && recordCommandBuffers();

    return rv;
}

void vgraphplay::gfx::System::dispose() {
    if (m_device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(m_device);
    }

    cleanupCommandPool();
    cleanupSemaphores();
    cleanupSwapchainFramebuffers();
    cleanupPipeline();
    cleanupPipelineLayout();
    cleanupRenderPass();
    cleanupShaderModules();
    cleanupSwapchain();
    cleanupDevice();
    cleanupSurface();
    cleanupDebugCallback();
    cleanupInstance();
}

bool vgraphplay::gfx::System::initInstance(bool debug) {
    if (m_instance != VK_NULL_HANDLE) {
        return true;
    }

    logGlobalExtensions();
    logGlobalLayers();

    // The list of extensions we need.
    std::vector<const char*> extension_names;

    // Add the extensions GLFW wants to the list.
    uint32_t glfw_extension_count;
    const char **glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
    for (unsigned int i = 0; i < glfw_extension_count; ++i) {
        BOOST_LOG_TRIVIAL(trace) << "GLFW requires extension: " << glfw_extensions[i];
        extension_names.emplace_back(glfw_extensions[i]);
    }

    // Add the debug report extension if we're running in debug mode.
    if (debug) {
        extension_names.emplace_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
    }

    // Make sure we have the extensions we need.
    uint32_t num_extensions;
    vkEnumerateInstanceExtensionProperties(nullptr, &num_extensions, nullptr);
    std::vector<VkExtensionProperties> instance_extensions{num_extensions};
    vkEnumerateInstanceExtensionProperties(nullptr, &num_extensions, instance_extensions.data());
    for (unsigned int i = 0; i < extension_names.size(); ++i) {
        bool found = false;
        std::string wanted_extension_name{extension_names[i]};
        for (unsigned int j = 0; j < instance_extensions.size(); ++j) {
            std::string extension_name{instance_extensions[j].extensionName};
            if (wanted_extension_name == extension_name) {
                found = true;
                break;
            }
        }

        if (!found) {
            BOOST_LOG_TRIVIAL(error) << "Unable to find extension " << extension_names[i] << ". Cannot continue.";
            return false;
        }
    }

    // The layers we need.
    std::vector<const char*> layer_names;

    // Add the standard validation layers if we're running in debug mode.
    if (debug) {
        layer_names.emplace_back("VK_LAYER_LUNARG_standard_validation");
    }

    // Make sure we have the layers we need.
    uint32_t num_layers;
    vkEnumerateInstanceLayerProperties(&num_layers, nullptr);
    std::vector<VkLayerProperties> instance_layers{num_layers};
    vkEnumerateInstanceLayerProperties(&num_layers, instance_layers.data());
    for (unsigned int i = 0; i < layer_names.size(); ++i) {
        bool found = false;
        std::string wanted_layer_name{layer_names[i]};
        for (unsigned int j = 0; j < instance_layers.size(); ++j) {
            std::string layer_name{instance_layers[j].layerName};
            if (wanted_layer_name == layer_name) {
                found = true;
                break;
            }
        }

        if (!found) {
            BOOST_LOG_TRIVIAL(error) << "Unable to find layer " << layer_names[i] << ". Cannot continue.";
            return false;
        }
    }

    VkInstanceCreateInfo inst_ci;
    inst_ci.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    inst_ci.pNext = nullptr;
    inst_ci.flags = 0;
    inst_ci.pApplicationInfo = nullptr;
    inst_ci.enabledLayerCount = (uint32_t)layer_names.size();
    inst_ci.ppEnabledLayerNames = layer_names.data();
    inst_ci.enabledExtensionCount = static_cast<uint32_t>(extension_names.size());
    inst_ci.ppEnabledExtensionNames = extension_names.data();

    VkResult rslt = vkCreateInstance(&inst_ci, nullptr, &m_instance);
    if (rslt == VK_SUCCESS) {
        BOOST_LOG_TRIVIAL(trace) << "Vulkan instance created: " << m_instance;
        return true;
    } else {
        BOOST_LOG_TRIVIAL(error) << "Error creating Vulkan instance: " << rslt;
        return false;
    }
}

void vgraphplay::gfx::System::cleanupInstance() {
    if (m_instance != VK_NULL_HANDLE) {
        BOOST_LOG_TRIVIAL(trace) << "Destroying Vulkan instance: " << m_instance;
        vkDestroyInstance(m_instance, nullptr);
        m_instance = VK_NULL_HANDLE;
    }
}

bool vgraphplay::gfx::System::initDebugCallback() {
    if (m_instance == VK_NULL_HANDLE || m_debug_callback != VK_NULL_HANDLE) {
        return true;
    }

    VkDebugReportCallbackCreateInfoEXT drc_ci;
    drc_ci.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
    drc_ci.pNext = nullptr;
    drc_ci.flags =
        VK_DEBUG_REPORT_ERROR_BIT_EXT |
        VK_DEBUG_REPORT_WARNING_BIT_EXT |
        VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT |
        VK_DEBUG_REPORT_INFORMATION_BIT_EXT |
        VK_DEBUG_REPORT_DEBUG_BIT_EXT;
    drc_ci.pfnCallback = debugCallback;
    drc_ci.pUserData = nullptr;

    VkResult rslt = vkCreateDebugReportCallbackEXT(m_instance, &drc_ci, nullptr, &m_debug_callback);
    if (rslt == VK_SUCCESS) {
        BOOST_LOG_TRIVIAL(trace) << "Debug report callback created: " << m_debug_callback;
        return true;
    } else {
        BOOST_LOG_TRIVIAL(error) << "Error creating debug report callback: " << rslt;
        return false;
    }
}

void vgraphplay::gfx::System::cleanupDebugCallback() {
    if (m_instance != VK_NULL_HANDLE && m_debug_callback != VK_NULL_HANDLE) {
        BOOST_LOG_TRIVIAL(trace) << "Destroying debug report callback: " << m_debug_callback;
        vkDestroyDebugReportCallbackEXT(m_instance, m_debug_callback, nullptr);
        m_debug_callback = VK_NULL_HANDLE;
    }
}

bool vgraphplay::gfx::System::initDevice() {
    if (m_device != VK_NULL_HANDLE) {
        return true;
    }

    if (m_instance == VK_NULL_HANDLE || m_surface == VK_NULL_HANDLE) {
        BOOST_LOG_TRIVIAL(error) << "Things have been initialized out of order. Cannot create device.";
        return false;
    }

    logPhysicalDevices(m_instance);

    uint32_t num_devices;
    vkEnumeratePhysicalDevices(m_instance, &num_devices, nullptr);
    std::vector<VkPhysicalDevice> devices(num_devices);
    vkEnumeratePhysicalDevices(m_instance, &num_devices, devices.data());

    ChosenDeviceInfo chosen = choosePhysicalDevice(devices, m_surface);

    if (chosen.dev == VK_NULL_HANDLE) {
        return false;
    }

    m_physical_device = chosen.dev;

    BOOST_LOG_TRIVIAL(trace) << "physical device = " << m_physical_device
                             << " graphics queue family = " << chosen.graphics_queue_family
                             << " present queue family = " << chosen.present_queue_family;

    float queue_priority = 1.0;
    std::vector<VkDeviceQueueCreateInfo> queue_cis;
    VkDeviceQueueCreateInfo queue_ci;
    queue_ci.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_ci.pNext = nullptr;
    queue_ci.flags = 0;
    queue_ci.queueFamilyIndex = chosen.graphics_queue_family;
    queue_ci.queueCount = 1;
    queue_ci.pQueuePriorities = &queue_priority;
    queue_cis.push_back(queue_ci);

    if (chosen.present_queue_family != chosen.graphics_queue_family) {
        VkDeviceQueueCreateInfo queue_ci;
        queue_ci.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_ci.pNext = nullptr;
        queue_ci.flags = 0;
        queue_ci.queueFamilyIndex = chosen.present_queue_family;
        queue_ci.queueCount = 1;
        queue_ci.pQueuePriorities = &queue_priority;
        queue_cis.push_back(queue_ci);
    }

    std::vector<const char*> extension_names;
    extension_names.emplace_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

    VkDeviceCreateInfo device_ci;
    device_ci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_ci.pNext = nullptr;
    device_ci.flags = 0;
    device_ci.queueCreateInfoCount = (uint32_t)queue_cis.size();
    device_ci.pQueueCreateInfos = queue_cis.data();
    device_ci.enabledLayerCount = 0;
    device_ci.ppEnabledLayerNames = nullptr;
    device_ci.pEnabledFeatures = nullptr;
    device_ci.enabledExtensionCount = (uint32_t)extension_names.size();
    device_ci.ppEnabledExtensionNames = extension_names.data();

    VkResult rslt = vkCreateDevice(m_physical_device, &device_ci, nullptr, &m_device);
    if (rslt == VK_SUCCESS) {
        BOOST_LOG_TRIVIAL(trace) << "Device created: " << m_device;
        m_graphics_queue_family = chosen.graphics_queue_family;
        m_present_queue_family = chosen.present_queue_family;
        vkGetDeviceQueue(m_device, m_graphics_queue_family, 0, &m_graphics_queue);
        BOOST_LOG_TRIVIAL(trace) << "Graphics queue: " << m_graphics_queue;
        vkGetDeviceQueue(m_device, m_present_queue_family, 0, &m_present_queue);
        BOOST_LOG_TRIVIAL(trace) << "Present queue: " << m_present_queue;
        return true;
    } else {
        BOOST_LOG_TRIVIAL(error) << "Error creating device " << rslt;
        return false;
    }
}

void vgraphplay::gfx::System::cleanupDevice() {
    if (m_device != VK_NULL_HANDLE) {
        BOOST_LOG_TRIVIAL(trace) << "Destroying device: " << m_device;
        vkDestroyDevice(m_device, nullptr);
        m_device = VK_NULL_HANDLE;
    }
}

vgraphplay::gfx::ChosenDeviceInfo vgraphplay::gfx::System::choosePhysicalDevice(std::vector<VkPhysicalDevice> &devices, VkSurfaceKHR &surface) {
    const uint32_t MAX_INT = std::numeric_limits<uint32_t>::max();
    
    for (auto &dev : devices) {
        // Do we have queue families suitable for graphics / presentation?
        uint32_t graphics_queue = MAX_INT, present_queue = MAX_INT, num_queue_families = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(dev, &num_queue_families, nullptr);
        std::vector<VkQueueFamilyProperties> queue_families{num_queue_families};
        vkGetPhysicalDeviceQueueFamilyProperties(dev, &num_queue_families, queue_families.data());

        for (uint32_t id = 0; id < queue_families.size(); ++id) {
            if (graphics_queue == MAX_INT && queue_families[id].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                graphics_queue = id;
            }

            if (present_queue == MAX_INT) {
                VkBool32 supports_present = false;
                vkGetPhysicalDeviceSurfaceSupportKHR(dev, id, surface, &supports_present);
                if (supports_present) {
                    present_queue = id;
                }
            }
        }

        if (graphics_queue == MAX_INT && present_queue == MAX_INT) {
            continue;
        }

        // Are the extensions / layers we want supported?
        uint32_t num_extensions = 0;
        vkEnumerateDeviceExtensionProperties(dev, nullptr, &num_extensions, nullptr);
        std::vector<VkExtensionProperties> extensions{num_extensions};
        vkEnumerateDeviceExtensionProperties(dev, nullptr, &num_extensions, extensions.data());

        std::set<std::string> required_extensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };

        for (const auto &extension : extensions) {
            required_extensions.erase(extension.extensionName);
        }

        if (!required_extensions.empty()) {
            continue;
        }
        
        // Are swapchains supported, and are there surface formats / present modes we can use?
        uint32_t num_formats = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(dev, surface, &num_formats, nullptr);
        uint32_t num_present_modes = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(dev, surface, &num_present_modes, nullptr);

        if (num_formats > 0 && num_present_modes > 0) {
            return {
                dev,
                graphics_queue,
                present_queue,
            };
        }
    }

    return {
        VK_NULL_HANDLE,
        MAX_INT,
        MAX_INT,
    };
}

bool vgraphplay::gfx::System::initSurface() {
    if (m_surface != VK_NULL_HANDLE) {
        return true;
    }

    if (m_instance == VK_NULL_HANDLE) {
        BOOST_LOG_TRIVIAL(error) << "Things have been initialized out of order. Cannot create surface.";
        return false;
    }

    VkResult rslt = glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface);
    if (rslt == VK_SUCCESS) {
        BOOST_LOG_TRIVIAL(trace) << "Created surface: " << m_surface;
        return true;
    } else {
        BOOST_LOG_TRIVIAL(error) << "Error creating surface: " << rslt;
        return false;
    }
}

void vgraphplay::gfx::System::cleanupSurface() {
    if (m_instance != VK_NULL_HANDLE && m_surface != VK_NULL_HANDLE) {
        BOOST_LOG_TRIVIAL(trace) << "Destroying surface: " << m_surface;
        vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
        m_surface = VK_NULL_HANDLE;
    }
}

bool vgraphplay::gfx::System::initSwapchain() {
    if (m_swapchain != VK_NULL_HANDLE) {
        return true;
    }

    if (m_physical_device == VK_NULL_HANDLE || m_surface == VK_NULL_HANDLE || m_device == VK_NULL_HANDLE) {
        BOOST_LOG_TRIVIAL(error) << "Things have been initialized out of order. Cannot create swapchain.";
        return false;
    }

    logSurfaceCapabilities(m_physical_device, m_surface);

    VkSurfaceCapabilitiesKHR surf_caps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physical_device, m_surface, &surf_caps);

    m_swapchain_extent = chooseSwapExtent(surf_caps);

    // Use one more than the minimum, unless that would
    // put us over the maximum.
    uint32_t image_count = surf_caps.minImageCount + 1;
    if (surf_caps.maxImageCount > 0 && image_count > surf_caps.maxImageCount) {
        image_count = surf_caps.maxImageCount;
    }

    uint32_t num_formats;
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_physical_device, m_surface, &num_formats, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(num_formats);
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_physical_device, m_surface, &num_formats, formats.data());

    m_swapchain_format = chooseSurfaceFormat(formats);

    uint32_t num_modes;
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_physical_device, m_surface, &num_modes, nullptr);
    std::vector<VkPresentModeKHR> modes(num_modes);
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_physical_device, m_surface, &num_modes, modes.data());

    VkPresentModeKHR present_mode = choosePresentMode(modes);

    std::vector<uint32_t> queue_families;
    VkSharingMode sharing_mode = VK_SHARING_MODE_EXCLUSIVE;
    queue_families.emplace_back(m_graphics_queue_family);
    if (m_graphics_queue_family != m_present_queue_family) {
        queue_families.emplace_back(m_present_queue_family);
        sharing_mode = VK_SHARING_MODE_CONCURRENT;
    }

    VkSwapchainCreateInfoKHR swapchain_ci;
    swapchain_ci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_ci.pNext = nullptr;
    swapchain_ci.flags = 0;
    swapchain_ci.surface = m_surface;
    swapchain_ci.minImageCount = image_count;
    swapchain_ci.imageFormat = m_swapchain_format.format;
    swapchain_ci.imageColorSpace = m_swapchain_format.colorSpace;
    swapchain_ci.imageExtent = m_swapchain_extent;
    swapchain_ci.imageArrayLayers = 1;
    swapchain_ci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchain_ci.imageSharingMode = sharing_mode;
    swapchain_ci.queueFamilyIndexCount = (uint32_t)queue_families.size();
    swapchain_ci.pQueueFamilyIndices = queue_families.data();
    swapchain_ci.preTransform = surf_caps.currentTransform;
    swapchain_ci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_ci.presentMode = present_mode;
    swapchain_ci.clipped = VK_TRUE;
    swapchain_ci.oldSwapchain = VK_NULL_HANDLE;

    VkResult rslt = vkCreateSwapchainKHR(m_device, &swapchain_ci, nullptr, &m_swapchain);
    if (rslt == VK_SUCCESS) {
        BOOST_LOG_TRIVIAL(trace) << "Created swapchain: " << m_swapchain;
    } else {
        BOOST_LOG_TRIVIAL(error) << "Error creating swapchain: " << rslt;
        return false;
    }

    uint32_t num_swapchain_images = 0;
    vkGetSwapchainImagesKHR(m_device, m_swapchain, &num_swapchain_images, nullptr);
    m_swapchain_images.resize(num_swapchain_images, VK_NULL_HANDLE);
    vkGetSwapchainImagesKHR(m_device, m_swapchain, &num_swapchain_images, m_swapchain_images.data());

    m_swapchain_image_views.resize(num_swapchain_images, VK_NULL_HANDLE);
    for (unsigned int i = 0; i < m_swapchain_images.size(); ++i) {
        VkImageViewCreateInfo iv_ci;
        iv_ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        iv_ci.pNext = nullptr;
        iv_ci.flags = 0;
        iv_ci.image = m_swapchain_images[i];
        iv_ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
        iv_ci.format = m_swapchain_format.format;
        iv_ci.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        iv_ci.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        iv_ci.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        iv_ci.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        iv_ci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        iv_ci.subresourceRange.baseMipLevel = 0;
        iv_ci.subresourceRange.levelCount = 1;
        iv_ci.subresourceRange.baseArrayLayer = 0;
        iv_ci.subresourceRange.layerCount = 1;

        rslt = vkCreateImageView(m_device, &iv_ci, nullptr, &m_swapchain_image_views[i]);
        if (rslt == VK_SUCCESS) {
            BOOST_LOG_TRIVIAL(trace) << "Created swapchain image view " << i << ": " << m_swapchain_image_views[i];
        } else {
            BOOST_LOG_TRIVIAL(error) << "Error creating swapchain image view " << i << ": " << rslt;
            return false;
        }
    }

    return true;
}

void vgraphplay::gfx::System::cleanupSwapchain() {
    if (m_device != VK_NULL_HANDLE) {
        for (auto&& view : m_swapchain_image_views) {
            if (view != VK_NULL_HANDLE) {
                BOOST_LOG_TRIVIAL(trace) << "Destroying swapchain image view: " << view;
                vkDestroyImageView(m_device, view, nullptr);
            }
        }
        m_swapchain_image_views.clear();
    }

    if (m_device != VK_NULL_HANDLE && m_swapchain != VK_NULL_HANDLE) {
        BOOST_LOG_TRIVIAL(trace) << "Destroying swapchain: " << m_swapchain;
        vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
        m_swapchain = VK_NULL_HANDLE;
    }

    m_swapchain_images.clear();
    m_swapchain_format = { VK_FORMAT_UNDEFINED, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
    m_swapchain_extent = { 0, 0 };
}

VkSurfaceFormatKHR vgraphplay::gfx::System::chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &formats) {
    // If it doesn't care, go with what we want.
    if (formats.size() == 1 && formats[0].format == VK_FORMAT_UNDEFINED) {
        return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
    }

    // If what we want is available, go with it.
    for (const auto &format : formats) {
        if (format.format == VK_FORMAT_B8G8R8A8_UNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return format;
        }
    }

    // Otherwise, just settle for the first one?
    return formats[0];
}

VkPresentModeKHR vgraphplay::gfx::System::choosePresentMode(const std::vector<VkPresentModeKHR> &modes) {
    // Prefer mailbox over fifo, if it's available.
    for (const auto &mode: modes) {
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return mode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D vgraphplay::gfx::System::chooseSwapExtent(const VkSurfaceCapabilitiesKHR &surf_caps) {
    if (surf_caps.currentExtent.width != UINT32_MAX) {
        return surf_caps.currentExtent;
    }

    int width, height;
    glfwGetFramebufferSize(m_window, &width, &height);

    VkExtent2D extent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
    extent.width  = std::max(surf_caps.minImageExtent.width,  std::min(surf_caps.maxImageExtent.width,  extent.width));
    extent.height = std::max(surf_caps.minImageExtent.height, std::min(surf_caps.maxImageExtent.height, extent.height));
    return extent;
}

bool vgraphplay::gfx::System::initRenderPass() {
    if (m_render_pass != VK_NULL_HANDLE) {
        return true;
    }

    if (m_device == VK_NULL_HANDLE) {
        BOOST_LOG_TRIVIAL(error) << "Things have been initialized out of order. Cannot create render pass.";
        return false;
    }

    VkAttachmentDescription color_att;
    color_att.flags = 0;
    color_att.format = m_swapchain_format.format;
    color_att.samples = VK_SAMPLE_COUNT_1_BIT;
    color_att.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_att.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_att.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_att.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_att.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_att.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference color_ref;
    color_ref.attachment = 0;
    color_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass;
    subpass.flags = 0;
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.inputAttachmentCount = 0;
    subpass.pInputAttachments = nullptr;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_ref;
    subpass.pResolveAttachments = nullptr;
    subpass.pDepthStencilAttachment = nullptr;
    subpass.preserveAttachmentCount = 0;
    subpass.pPreserveAttachments = nullptr;

    VkSubpassDependency sd;
    sd.dependencyFlags = 0;
    sd.srcSubpass = VK_SUBPASS_EXTERNAL;
    sd.dstSubpass = 0;
    sd.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    sd.srcAccessMask = 0;
    sd.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    sd.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo rp_ci;
    rp_ci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rp_ci.pNext = nullptr;
    rp_ci.flags = 0;
    rp_ci.attachmentCount = 1;
    rp_ci.pAttachments = &color_att;
    rp_ci.subpassCount = 1;
    rp_ci.pSubpasses = &subpass;
    rp_ci.dependencyCount = 1;
    rp_ci.pDependencies = &sd;

    VkResult rslt = vkCreateRenderPass(m_device, &rp_ci, nullptr, &m_render_pass);
    if (rslt == VK_SUCCESS) {
        BOOST_LOG_TRIVIAL(trace) << "Created render pass: " << m_render_pass;
        return true;
    } else {
        BOOST_LOG_TRIVIAL(error) << "Error creating render pass: " << rslt;
        return false;
    }
}

void vgraphplay::gfx::System::cleanupRenderPass() {
    if (m_device != VK_NULL_HANDLE && m_render_pass != VK_NULL_HANDLE) {
        BOOST_LOG_TRIVIAL(trace) << "Destroying render pass: " << m_render_pass;
        vkDestroyRenderPass(m_device, m_render_pass, nullptr);
        m_render_pass = VK_NULL_HANDLE;
    }
}

bool vgraphplay::gfx::System::initShaderModules() {
    m_vertex_shader_module = createShaderModule(UNLIT_VERT_BYTECODE);
    m_fragment_shader_module = createShaderModule(UNLIT_FRAG_BYTECODE);

    if (m_vertex_shader_module == VK_NULL_HANDLE || m_fragment_shader_module == VK_NULL_HANDLE) {
        BOOST_LOG_TRIVIAL(trace) << "Unable to create all of the shader modules.";
        return false;
    } else {
        return true;
    }
}

void vgraphplay::gfx::System::cleanupShaderModules() {
    if (m_device != VK_NULL_HANDLE && m_vertex_shader_module != VK_NULL_HANDLE) {
        BOOST_LOG_TRIVIAL(trace) << "Destroying vertex shader module: " << m_vertex_shader_module;
        vkDestroyShaderModule(m_device, m_vertex_shader_module, nullptr);
        m_vertex_shader_module = VK_NULL_HANDLE;
    }

    if (m_device != VK_NULL_HANDLE && m_fragment_shader_module != VK_NULL_HANDLE) {
        BOOST_LOG_TRIVIAL(trace) << "Destroying fragment shader module: " << m_fragment_shader_module;
        vkDestroyShaderModule(m_device, m_fragment_shader_module, nullptr);
        m_fragment_shader_module = VK_NULL_HANDLE;
    }
}

VkShaderModule vgraphplay::gfx::System::createShaderModule(const Resource &rsrc) {
    VkShaderModule rv = VK_NULL_HANDLE;

    if (m_device == VK_NULL_HANDLE) {
        return rv;
    }

    VkShaderModuleCreateInfo sm_ci;
    sm_ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    sm_ci.pNext = nullptr;
    sm_ci.flags = 0;
    sm_ci.codeSize = rsrc.size();
    sm_ci.pCode = reinterpret_cast<const uint32_t*>(rsrc.data());

    VkResult rslt = vkCreateShaderModule(m_device, &sm_ci, nullptr, &rv);
    if (rslt == VK_SUCCESS) {
        BOOST_LOG_TRIVIAL(trace) << "Created shader module: " << rv;
    } else {
        BOOST_LOG_TRIVIAL(trace) << "Failed to create shader module: " << rslt;
    }

    return rv;
}

bool vgraphplay::gfx::System::initPipelineLayout() {
    if (m_pipeline_layout != VK_NULL_HANDLE) {
        return true;
    }

    if (m_device == VK_NULL_HANDLE) {
        BOOST_LOG_TRIVIAL(error) << "Things have been initialized out of order. Cannot create pipeline layout.";
        return false;
    }

    VkPipelineLayoutCreateInfo pl_layout_ci;
    pl_layout_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pl_layout_ci.pNext = nullptr;
    pl_layout_ci.flags = 0;
    pl_layout_ci.setLayoutCount = 0;
    pl_layout_ci.pSetLayouts = nullptr;
    pl_layout_ci.pushConstantRangeCount = 0;
    pl_layout_ci.pPushConstantRanges = nullptr;

    VkResult rslt = vkCreatePipelineLayout(m_device, &pl_layout_ci, nullptr, &m_pipeline_layout);
    if (rslt == VK_SUCCESS) {
        BOOST_LOG_TRIVIAL(trace) << "Created pipeline layout: " << m_pipeline_layout;
        return true;
    } else {
        BOOST_LOG_TRIVIAL(error) << "Error creating pipeline layout: " << rslt;
        return false;
    }
}

void vgraphplay::gfx::System::cleanupPipelineLayout() {
    if (m_device != VK_NULL_HANDLE && m_pipeline_layout != VK_NULL_HANDLE) {
        BOOST_LOG_TRIVIAL(trace) << "Destroying pipeline layout: " << m_pipeline_layout;
        vkDestroyPipelineLayout(m_device, m_pipeline_layout, nullptr);
        m_pipeline_layout = VK_NULL_HANDLE;
    }
}

bool vgraphplay::gfx::System::initPipeline() {
    if (m_pipeline != VK_NULL_HANDLE) {
        return true;
    }

    if (m_device == VK_NULL_HANDLE ||
        m_vertex_shader_module == VK_NULL_HANDLE ||
        m_fragment_shader_module == VK_NULL_HANDLE ||
        m_pipeline_layout == VK_NULL_HANDLE ||
        m_render_pass == VK_NULL_HANDLE) {
        BOOST_LOG_TRIVIAL(error) << "Things have been initialized out of order. Cannot create pipeline.";
    }

    VkPipelineShaderStageCreateInfo ss_ci[2];
    ss_ci[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    ss_ci[0].pNext = nullptr;
    ss_ci[0].flags = 0;
    ss_ci[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    ss_ci[0].module = m_vertex_shader_module;
    ss_ci[0].pName = "main";
    ss_ci[0].pSpecializationInfo = nullptr;

    ss_ci[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    ss_ci[1].pNext = nullptr;
    ss_ci[1].flags = 0;
    ss_ci[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    ss_ci[1].module = m_fragment_shader_module;
    ss_ci[1].pName = "main";
    ss_ci[1].pSpecializationInfo = nullptr;

    VkPipelineVertexInputStateCreateInfo vert_in_ci;
    vert_in_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vert_in_ci.pNext = nullptr;
    vert_in_ci.flags = 0;
    vert_in_ci.vertexBindingDescriptionCount = 0;
    vert_in_ci.pVertexBindingDescriptions = nullptr;
    vert_in_ci.vertexAttributeDescriptionCount = 0;
    vert_in_ci.pVertexAttributeDescriptions = nullptr;

    VkPipelineInputAssemblyStateCreateInfo input_asm_ci;
    input_asm_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_asm_ci.pNext = nullptr;
    input_asm_ci.flags = 0;
    input_asm_ci.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_asm_ci.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport;
    viewport.x = 0.0;
    viewport.y = 0.0;
    viewport.width = static_cast<float>(m_swapchain_extent.width);
    viewport.height = static_cast<float>(m_swapchain_extent.height);
    viewport.minDepth = 0.0;
    viewport.maxDepth = 1.0;

    VkRect2D scissor;
    scissor.offset = { 0, 0 };
    scissor.extent = m_swapchain_extent;

    VkPipelineViewportStateCreateInfo vp_ci;
    vp_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vp_ci.pNext = nullptr;
    vp_ci.flags = 0;
    vp_ci.viewportCount = 1;
    vp_ci.pViewports = &viewport;
    vp_ci.scissorCount = 1;
    vp_ci.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo raster_ci;
    raster_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    raster_ci.pNext = nullptr;
    raster_ci.flags = 0;
    raster_ci.depthClampEnable = VK_FALSE;
    raster_ci.rasterizerDiscardEnable = VK_FALSE;
    raster_ci.polygonMode = VK_POLYGON_MODE_FILL;
    raster_ci.cullMode = VK_CULL_MODE_BACK_BIT;
    raster_ci.frontFace = VK_FRONT_FACE_CLOCKWISE;
    raster_ci.depthBiasEnable = VK_FALSE;
    raster_ci.depthBiasConstantFactor = 0.0;
    raster_ci.depthBiasClamp = 0.0;
    raster_ci.depthBiasSlopeFactor = 0.0;
    raster_ci.lineWidth = 1.0;

    VkPipelineMultisampleStateCreateInfo msamp_ci;
    msamp_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    msamp_ci.pNext = nullptr;
    msamp_ci.flags = 0;
    msamp_ci.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    msamp_ci.sampleShadingEnable = VK_FALSE;
    msamp_ci.minSampleShading = 0.0;
    msamp_ci.pSampleMask = nullptr;
    msamp_ci.alphaToCoverageEnable = VK_FALSE;
    msamp_ci.alphaToOneEnable = VK_FALSE;

    // VkPipelineDepthStencilStateCreateInfo depth_ci;
    // depth_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    // depth_ci.pNext = nullptr;
    // depth_ci.flags = 0;

    VkPipelineColorBlendAttachmentState blender;
    blender.blendEnable = VK_FALSE;
    blender.srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    blender.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    blender.colorBlendOp = VK_BLEND_OP_ADD;
    blender.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    blender.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    blender.alphaBlendOp = VK_BLEND_OP_ADD;
    blender.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo blend_ci;
    blend_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blend_ci.pNext = nullptr;
    blend_ci.flags = 0;
    blend_ci.logicOpEnable = VK_FALSE;
    blend_ci.logicOp = VK_LOGIC_OP_COPY;
    blend_ci.attachmentCount = 1;
    blend_ci.pAttachments = &blender;
    blend_ci.blendConstants[0] = 0.0;
    blend_ci.blendConstants[1] = 0.0;
    blend_ci.blendConstants[2] = 0.0;
    blend_ci.blendConstants[3] = 0.0;

    // VkPipelineDynamicStateCreateInfo dyn_state_ci;
    // dyn_state_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    // dyn_state_ci.pNext = nullptr;
    // dyn_state_ci.flags = 0;

    VkGraphicsPipelineCreateInfo pipeline_ci;
    pipeline_ci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_ci.pNext = nullptr;
    pipeline_ci.flags = 0;
    pipeline_ci.stageCount = 2;
    pipeline_ci.pStages = ss_ci;
    pipeline_ci.pVertexInputState = &vert_in_ci;
    pipeline_ci.pInputAssemblyState = &input_asm_ci;
    pipeline_ci.pTessellationState = nullptr;
    pipeline_ci.pViewportState = &vp_ci;
    pipeline_ci.pRasterizationState = &raster_ci;
    pipeline_ci.pMultisampleState = &msamp_ci;
    pipeline_ci.pDepthStencilState = nullptr;
    pipeline_ci.pColorBlendState = &blend_ci;
    pipeline_ci.pDynamicState = nullptr;
    pipeline_ci.layout = m_pipeline_layout;
    pipeline_ci.renderPass = m_render_pass;
    pipeline_ci.subpass = 0;
    pipeline_ci.basePipelineHandle = VK_NULL_HANDLE;
    pipeline_ci.basePipelineIndex = -1;

    VkResult rslt = vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipeline_ci, nullptr, &m_pipeline);
    if (rslt == VK_SUCCESS) {
        BOOST_LOG_TRIVIAL(trace) << "Created graphics pipeline: " << m_pipeline;
        return true;
    } else {
        BOOST_LOG_TRIVIAL(error) << "Error creating graphics pipeline: " << rslt;
        return false;
    }
}

void vgraphplay::gfx::System::cleanupPipeline() {
    if (m_device != VK_NULL_HANDLE && m_pipeline != VK_NULL_HANDLE) {
        BOOST_LOG_TRIVIAL(trace) << "Destroying pipeline: " << m_pipeline;
        vkDestroyPipeline(m_device, m_pipeline, nullptr);
    }
}

bool vgraphplay::gfx::System::initSwapchainFramebuffers() {
    if (m_swapchain_framebuffers.size() > 0) {
        return true;
    }

    if (m_device == VK_NULL_HANDLE || m_render_pass == VK_NULL_HANDLE) {
        BOOST_LOG_TRIVIAL(error) << "Things have been initialized out of order. Cannot create framebuffer.";
        return false;
    }

    m_swapchain_framebuffers.resize(m_swapchain_image_views.size());
    for (unsigned int i = 0; i < m_swapchain_image_views.size(); ++i) {
        VkFramebufferCreateInfo fb_ci;
        fb_ci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fb_ci.pNext = nullptr;
        fb_ci.flags = 0;
        fb_ci.renderPass = m_render_pass;
        fb_ci.attachmentCount = 1;
        fb_ci.pAttachments = &m_swapchain_image_views[i];
        fb_ci.width = m_swapchain_extent.width;
        fb_ci.height = m_swapchain_extent.height;
        fb_ci.layers = 1;

        VkResult rslt = vkCreateFramebuffer(m_device, &fb_ci, nullptr, &m_swapchain_framebuffers[i]);
        if (rslt == VK_SUCCESS) {
            BOOST_LOG_TRIVIAL(trace) << "Created swapchain framebuffer " << i << ": " << m_swapchain_framebuffers[i];
        } else {
            BOOST_LOG_TRIVIAL(error) << "Error creating swapchain framebuffer " << i << ": " << rslt;
            return false;
        }
    }

    return true;
}

void vgraphplay::gfx::System::cleanupSwapchainFramebuffers() {
    if (m_device != VK_NULL_HANDLE && m_swapchain_framebuffers.size() > 0) {
        for (unsigned int i = 0; i < m_swapchain_framebuffers.size(); ++i) {
            BOOST_LOG_TRIVIAL(trace) << "Destroying swapchain framebuffer " << i << ": " << m_swapchain_framebuffers[i];
            vkDestroyFramebuffer(m_device, m_swapchain_framebuffers[i], nullptr);
        }
    }
}

bool vgraphplay::gfx::System::initSemaphores() {
    if (m_image_available_semaphore != VK_NULL_HANDLE &&
        m_render_finished_semaphore != VK_NULL_HANDLE) {
        return true;
    }

    if (m_device == VK_NULL_HANDLE) {
        BOOST_LOG_TRIVIAL(error) << "Things have been initialized out of order. Cannot create semaphores.";
        return false;
    }

    VkSemaphoreCreateInfo sem_ci;
    sem_ci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    sem_ci.pNext = nullptr;
    sem_ci.flags = 0;

    if (m_image_available_semaphore == VK_NULL_HANDLE) {
        VkResult rslt = vkCreateSemaphore(m_device, &sem_ci, nullptr, &m_image_available_semaphore);
        if (rslt == VK_SUCCESS) {
            BOOST_LOG_TRIVIAL(trace) << "Created image available semaphore: " << m_image_available_semaphore;
        } else {
            BOOST_LOG_TRIVIAL(error) << "Error creating image available semaphore: " << rslt;
            return false;
        }
    }

    if (m_render_finished_semaphore == VK_NULL_HANDLE) {
        VkResult rslt = vkCreateSemaphore(m_device, &sem_ci, nullptr, &m_render_finished_semaphore);
        if (rslt == VK_SUCCESS) {
            BOOST_LOG_TRIVIAL(trace) << "Created render finished semaphore: " << m_render_finished_semaphore;
        } else {
            BOOST_LOG_TRIVIAL(error) << "Error creating render finished semaphore: " << rslt;
            return false;
        }
    }

    return true;
}

void vgraphplay::gfx::System::cleanupSemaphores() {
    if (m_device != VK_NULL_HANDLE && m_image_available_semaphore != VK_NULL_HANDLE) {
        BOOST_LOG_TRIVIAL(trace) << "Destroying image available semaphore: " << m_image_available_semaphore;
        vkDestroySemaphore(m_device, m_image_available_semaphore, nullptr);
    }

    if (m_device != VK_NULL_HANDLE && m_render_finished_semaphore != VK_NULL_HANDLE) {
        BOOST_LOG_TRIVIAL(trace) << "Destroying render finished semaphore: " << m_render_finished_semaphore;
        vkDestroySemaphore(m_device, m_render_finished_semaphore, nullptr);
    }
}

bool vgraphplay::gfx::System::initCommandPool() {
    if (m_command_pool != VK_NULL_HANDLE) {
        return true;
    }

    if (m_device == VK_NULL_HANDLE) {
        BOOST_LOG_TRIVIAL(error) << "Things have been initialized out of order. Cannot create command pool.";
        return false;
    }

    VkCommandPoolCreateInfo cp_ci;
    cp_ci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cp_ci.pNext = nullptr;
    cp_ci.flags = 0;
    cp_ci.queueFamilyIndex = m_graphics_queue_family;

    VkResult rslt = vkCreateCommandPool(m_device, &cp_ci, nullptr, &m_command_pool);
    if (rslt == VK_SUCCESS) {
        BOOST_LOG_TRIVIAL(trace) << "Created command pool: " << m_command_pool;
        return true;
    } else {
        BOOST_LOG_TRIVIAL(error) << "Error creating command pool " << rslt;
        return false;
    }
}

void vgraphplay::gfx::System::cleanupCommandPool() {
    if (m_device != VK_NULL_HANDLE && m_command_pool != VK_NULL_HANDLE) {
        BOOST_LOG_TRIVIAL(trace) << "Destroying command pool: " << m_command_pool;
        vkDestroyCommandPool(m_device, m_command_pool, nullptr);
    }
}

bool vgraphplay::gfx::System::initCommandBuffers() {
    if (m_command_buffers.size() > 0) {
        return true;
    }

    if (m_device == VK_NULL_HANDLE || m_command_pool == VK_NULL_HANDLE) {
        BOOST_LOG_TRIVIAL(error) << "Things have been initialized out of order. Cannot create command buffers.";
        return false;
    }

    size_t num_buffers = m_swapchain_framebuffers.size();
    m_command_buffers.resize(num_buffers);

    VkCommandBufferAllocateInfo cb_ai;
    cb_ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cb_ai.pNext = nullptr;
    cb_ai.commandPool = m_command_pool;
    cb_ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cb_ai.commandBufferCount = static_cast<uint32_t>(num_buffers);

    VkResult rslt = vkAllocateCommandBuffers(m_device, &cb_ai, m_command_buffers.data());
    if (rslt == VK_SUCCESS) {
        BOOST_LOG_TRIVIAL(trace) << "Allocated " << m_command_buffers.size() << " command buffers";
        return true;
    } else {
        BOOST_LOG_TRIVIAL(error) << "Error creating command buffers " << rslt;
        return false;
    }
}

void vgraphplay::gfx::System::cleanupCommandBuffers() {
    if (m_command_buffers.size() > 0 &&
        m_device != VK_NULL_HANDLE &&
        m_command_pool != VK_NULL_HANDLE) {
        BOOST_LOG_TRIVIAL(trace) << "Freeing " << m_command_buffers.size() << " command buffers";
        vkFreeCommandBuffers(m_device, m_command_pool, m_command_buffers.size(), m_command_buffers.data());
        m_command_buffers.clear();
    }
}

bool vgraphplay::gfx::System::recordCommandBuffers() {
    for (unsigned int i = 0; i < m_command_buffers.size(); ++i) {
        VkCommandBufferBeginInfo cb_bi;
        cb_bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        cb_bi.pNext = nullptr;
        cb_bi.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
        cb_bi.pInheritanceInfo = nullptr;

        VkResult rslt = vkBeginCommandBuffer(m_command_buffers[i], &cb_bi);
        if (rslt == VK_SUCCESS) {
            BOOST_LOG_TRIVIAL(trace) << "Beginning recording to command buffer " << m_command_buffers[i];
        } else {
            BOOST_LOG_TRIVIAL(error) << "Error beginning command buffer recording: " << rslt;
            return false;
        }

        VkRenderPassBeginInfo rp_bi;
        rp_bi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        rp_bi.pNext = nullptr;
        rp_bi.renderPass = m_render_pass;
        rp_bi.framebuffer = m_swapchain_framebuffers[i];
        rp_bi.renderArea.offset = { 0, 0 };
        rp_bi.renderArea.extent = m_swapchain_extent;

        VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
        rp_bi.clearValueCount = 1;
        rp_bi.pClearValues = &clearColor;

        vkCmdBeginRenderPass(m_command_buffers[i], &rp_bi, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(m_command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
        vkCmdDraw(m_command_buffers[i], 3, 1, 0, 0);
        vkCmdEndRenderPass(m_command_buffers[i]);

        rslt = vkEndCommandBuffer(m_command_buffers[i]);
        if (rslt == VK_SUCCESS) {
            BOOST_LOG_TRIVIAL(trace) << "Finished recording to command buffer " << m_command_buffers[i];
        } else {
            BOOST_LOG_TRIVIAL(trace) << "Error finishing command buffer recording: " << rslt;
            return false;
        }
    }

    return true;
}

void vgraphplay::gfx::System::drawFrame() {
    uint32_t image_index;
    vkAcquireNextImageKHR(m_device, m_swapchain, std::numeric_limits<uint64_t>::max(),
                          m_image_available_semaphore, VK_NULL_HANDLE, &image_index);

    VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    VkSubmitInfo si;
    si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    si.pNext = nullptr;
    si.waitSemaphoreCount = 1;
    si.pWaitSemaphores = &m_image_available_semaphore;
    si.pWaitDstStageMask = wait_stages;
    si.commandBufferCount = 1;
    si.pCommandBuffers = &m_command_buffers[image_index];
    si.signalSemaphoreCount = 1;
    si.pSignalSemaphores = &m_render_finished_semaphore;

    VkResult rslt = vkQueueSubmit(m_graphics_queue, 1, &si, VK_NULL_HANDLE);
    if (rslt != VK_SUCCESS) {
        BOOST_LOG_TRIVIAL(error) << "Error submitting draw command buffer: " << rslt;
    }

    VkPresentInfoKHR pi;
    pi.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    pi.pNext = nullptr;
    pi.waitSemaphoreCount = 1;
    pi.pWaitSemaphores = &m_render_finished_semaphore;
    pi.swapchainCount = 1;
    pi.pSwapchains = &m_swapchain;
    pi.pImageIndices = &image_index;
    pi.pResults = nullptr;

    rslt = vkQueuePresentKHR(m_present_queue, &pi);
    if (rslt != VK_SUCCESS) {
        BOOST_LOG_TRIVIAL(error) << "Error submitting swapchain update: " << rslt;
    }
}
