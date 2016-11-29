// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#include "Graphics.h"

#include <fstream>
#include <iostream>

#include <boost/log/trivial.hpp>

#include "VulkanOutput.h"

namespace vgraphplay {
    namespace gfx {
        CommandQueues::CommandQueues(Device *parent)
            : m_parent{parent},
              m_graphics_queue_family{UINT32_MAX},
              m_graphics_queue{VK_NULL_HANDLE},
              m_present_queue_family{UINT32_MAX},
              m_present_queue{VK_NULL_HANDLE}
        {}

        CommandQueues::~CommandQueues() {
            dispose();
        }

        bool CommandQueues::initialize(uint32_t gq_fam, uint32_t pq_fam) {
            VkDevice &dev = device();

            m_graphics_queue_family = gq_fam;
            vkGetDeviceQueue(dev, m_graphics_queue_family, 0, &m_graphics_queue);
            BOOST_LOG_TRIVIAL(trace) << "Graphics queue: " << m_graphics_queue;

            m_present_queue_family = pq_fam;
            vkGetDeviceQueue(dev, m_present_queue_family, 0, &m_present_queue);
            BOOST_LOG_TRIVIAL(trace) << "Present queue: " << m_present_queue;

            return true;
        }

        void CommandQueues::dispose() {
            // Nothing to do here, yet.
        }

        VkDevice& CommandQueues::device() {
            return m_parent->device();
        }

        Device::Device(System *parent)
            : m_parent{parent},
              m_device{VK_NULL_HANDLE},
              m_physical_device{VK_NULL_HANDLE},
              m_queues{this},
              m_present{this}
        {}

        Device::~Device() {
            dispose();
        }

        bool Device::initialize() {
            if (m_device != VK_NULL_HANDLE) {
                return true;
            }

            VkInstance &inst = instance();
            VkSurfaceKHR &surf = surface();

            if (inst == VK_NULL_HANDLE || surf == VK_NULL_HANDLE) {
                BOOST_LOG_TRIVIAL(error) << "Things have been initialized out of order. Cannot create device.";
                return false;
            }

            logPhysicalDevices(inst);

            uint32_t num_devices;
            vkEnumeratePhysicalDevices(inst, &num_devices, nullptr);
            std::vector<VkPhysicalDevice> devices(num_devices);
            vkEnumeratePhysicalDevices(inst, &num_devices, devices.data());
            m_physical_device = choosePhysicalDevice(devices);

            uint32_t num_queue_families = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(m_physical_device, &num_queue_families, nullptr);
            std::vector<VkQueueFamilyProperties> queue_families(num_queue_families);
            vkGetPhysicalDeviceQueueFamilyProperties(m_physical_device, &num_queue_families, queue_families.data());
            uint32_t graphics_queue_family = chooseGraphicsQueueFamily(m_physical_device, queue_families);
            uint32_t present_queue_family = choosePresentQueueFamily(m_physical_device, queue_families, surf);

            BOOST_LOG_TRIVIAL(trace) << "physical device = " << m_physical_device
                                     << " graphics queue family = " << graphics_queue_family
                                     << " present queue family = " << present_queue_family;

            float queue_priority = 1.0;
            std::vector<VkDeviceQueueCreateInfo> queue_cis;
            VkDeviceQueueCreateInfo queue_ci;
            queue_ci.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queue_ci.pNext = nullptr;
            queue_ci.flags = 0;
            queue_ci.queueFamilyIndex = graphics_queue_family;
            queue_ci.queueCount = 1;
            queue_ci.pQueuePriorities = &queue_priority;
            queue_cis.push_back(queue_ci);

            if (present_queue_family != graphics_queue_family) {
                VkDeviceQueueCreateInfo queue_ci;
                queue_ci.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                queue_ci.pNext = nullptr;
                queue_ci.flags = 0;
                queue_ci.queueFamilyIndex = present_queue_family;
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
            device_ci.queueCreateInfoCount = queue_cis.size();
            device_ci.pQueueCreateInfos = queue_cis.data();
            device_ci.enabledLayerCount = 0;
            device_ci.ppEnabledLayerNames = nullptr;
            device_ci.pEnabledFeatures = nullptr;
            device_ci.enabledExtensionCount = extension_names.size();
            device_ci.ppEnabledExtensionNames = extension_names.data();

            VkResult rslt = vkCreateDevice(m_physical_device, &device_ci, nullptr, &m_device);

            if (rslt == VK_SUCCESS) {
                BOOST_LOG_TRIVIAL(trace) << "Device created: " << m_device;
            } else {
                BOOST_LOG_TRIVIAL(error) << "Error creating device " << rslt;
                return false;
            }

            return m_queues.initialize(graphics_queue_family, present_queue_family) &&
                m_present.initialize();
        }

        void Device::dispose() {
            m_present.dispose();
            m_queues.dispose();

            if (m_device != VK_NULL_HANDLE) {
                BOOST_LOG_TRIVIAL(trace) << "Destroying device: " << m_device;
                vkDestroyDevice(m_device, nullptr);
                m_device = VK_NULL_HANDLE;
            }

            // No need to actually destroy the rest of these; they're
            // managed by Vulkan.
            m_physical_device = VK_NULL_HANDLE;
        }

        VkPhysicalDevice Device::choosePhysicalDevice(const std::vector<VkPhysicalDevice> &devices) {
            // We probably want to make a better decision than this...
            return devices[0];
        }

        uint32_t Device::chooseGraphicsQueueFamily(VkPhysicalDevice &device, const std::vector<VkQueueFamilyProperties> &families) {
            for (uint32_t i = 0; i < families.size(); ++i) {
                if (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                    return i;
                }
            }
            return UINT32_MAX;
        }

        uint32_t Device::choosePresentQueueFamily(VkPhysicalDevice &device, const std::vector<VkQueueFamilyProperties> &families, VkSurfaceKHR &surf) {
            for (uint32_t i = 0; i < families.size(); ++i) {
                VkBool32 supports_present = false;
                vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surf, &supports_present);
                if (supports_present) {
                    return i;
                }
            }
            return UINT32_MAX;
        }

        GLFWwindow* Device::window() {
            return m_parent->window();
        }

        VkInstance& Device::instance() {
            return m_parent->instance();
        }

        VkPhysicalDevice& Device::physicalDevice() {
            return m_physical_device;
        }

        VkDevice& Device::device() {
            return m_device;
        }

        VkSurfaceKHR& Device::surface() {
            return m_parent->surface();
        }

        CommandQueues& Device::queues() {
            return m_queues;
        }

        Presentation::Presentation(Device *parent)
            : m_parent{parent},
              m_swapchain{VK_NULL_HANDLE},
              m_swapchain_images{},
              m_swapchain_image_views{},
              m_format{VK_FORMAT_UNDEFINED, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR},
              m_extent{0, 0}
        {}

        Presentation::~Presentation() {
            dispose();
        }

        bool Presentation::initialize() {
            if (m_swapchain == VK_NULL_HANDLE) {
                VkPhysicalDevice pdev = physicalDevice();
                VkDevice dev = device();
                VkSurfaceKHR surf = surface();
                CommandQueues &qs = queues();

                logSurfaceCapabilities(pdev, surf);

                VkSurfaceCapabilitiesKHR surf_caps;
                vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pdev, surf, &surf_caps);

                m_extent = chooseSwapExtent(surf_caps);

                // Use one more than the minimum, unless that would
                // put us over the maximum.
                uint32_t image_count = surf_caps.minImageCount + 1;
                if (surf_caps.maxImageCount > 0 && image_count > surf_caps.maxImageCount) {
                    image_count = surf_caps.maxImageCount;
                }

                uint32_t num_formats;
                vkGetPhysicalDeviceSurfaceFormatsKHR(pdev, surf, &num_formats, nullptr);
                std::vector<VkSurfaceFormatKHR> formats(num_formats);
                vkGetPhysicalDeviceSurfaceFormatsKHR(pdev, surf, &num_formats, formats.data());

                m_format = chooseSurfaceFormat(formats);

                uint32_t num_modes;
                vkGetPhysicalDeviceSurfacePresentModesKHR(pdev, surf, &num_modes, nullptr);
                std::vector<VkPresentModeKHR> modes(num_modes);
                vkGetPhysicalDeviceSurfacePresentModesKHR(pdev, surf, &num_modes, modes.data());

                VkPresentModeKHR present_mode = choosePresentMode(modes);

                std::vector<uint32_t> queue_families;
                VkSharingMode sharing_mode = VK_SHARING_MODE_EXCLUSIVE;
                queue_families.emplace_back(qs.graphicsQueueFamily());
                if (qs.graphicsQueueFamily() != qs.presentQueueFamily()) {
                    queue_families.emplace_back(qs.presentQueueFamily());
                    sharing_mode = VK_SHARING_MODE_CONCURRENT;
                }

                VkSwapchainCreateInfoKHR swapchain_ci;
                swapchain_ci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
                swapchain_ci.pNext = nullptr;
                swapchain_ci.flags = 0;
                swapchain_ci.surface = surf;
                swapchain_ci.minImageCount = image_count;
                swapchain_ci.imageFormat = m_format.format;
                swapchain_ci.imageColorSpace = m_format.colorSpace;
                swapchain_ci.imageExtent = m_extent;
                swapchain_ci.imageArrayLayers = 1;
                swapchain_ci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
                swapchain_ci.imageSharingMode = sharing_mode;
                swapchain_ci.queueFamilyIndexCount = queue_families.size();
                swapchain_ci.pQueueFamilyIndices = queue_families.data();
                swapchain_ci.preTransform = surf_caps.currentTransform;
                swapchain_ci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
                swapchain_ci.presentMode = present_mode;
                swapchain_ci.clipped = VK_TRUE;
                swapchain_ci.oldSwapchain = VK_NULL_HANDLE;

                VkResult rslt = vkCreateSwapchainKHR(dev, &swapchain_ci, nullptr, &m_swapchain);
                if (rslt == VK_SUCCESS) {
                    BOOST_LOG_TRIVIAL(trace) << "Created swapchain: " << m_swapchain;
                } else {
                    BOOST_LOG_TRIVIAL(error) << "Error creating swapchain: " << rslt;
                    return false;
                }

                uint32_t num_swapchain_images = 0;
                vkGetSwapchainImagesKHR(dev, m_swapchain, &num_swapchain_images, nullptr);
                m_swapchain_images.resize(num_swapchain_images, VK_NULL_HANDLE);
                vkGetSwapchainImagesKHR(dev, m_swapchain, &num_swapchain_images, m_swapchain_images.data());

                m_swapchain_image_views.resize(num_swapchain_images, VK_NULL_HANDLE);
                for (unsigned int i = 0; i < m_swapchain_images.size(); ++i) {
                    VkImageViewCreateInfo iv_ci;
                    iv_ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                    iv_ci.pNext = nullptr;
                    iv_ci.flags = 0;
                    iv_ci.image = m_swapchain_images[i];
                    iv_ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
                    iv_ci.format = m_format.format;
                    iv_ci.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
                    iv_ci.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
                    iv_ci.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
                    iv_ci.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
                    iv_ci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    iv_ci.subresourceRange.baseMipLevel = 0;
                    iv_ci.subresourceRange.levelCount = 1;
                    iv_ci.subresourceRange.baseArrayLayer = 0;
                    iv_ci.subresourceRange.layerCount = 1;

                    rslt = vkCreateImageView(dev, &iv_ci, nullptr, &m_swapchain_image_views[i]);
                    if (rslt == VK_SUCCESS) {
                        BOOST_LOG_TRIVIAL(trace) << "Created swapchain image view " << i << ": " << m_swapchain_image_views[i];
                    } else {
                        BOOST_LOG_TRIVIAL(error) << "Error creating swapchain image view " << i << ": " << rslt;
                        return false;
                    }
                }
            }

            return true;
        }

        void Presentation::dispose() {
            VkDevice &dev = device();
            if (dev != VK_NULL_HANDLE) {
                for (auto&& view : m_swapchain_image_views) {
                    if (view != VK_NULL_HANDLE) {
                        BOOST_LOG_TRIVIAL(trace) << "Destroying swapchain image view: " << view;
                        vkDestroyImageView(dev, view, nullptr);
                    }
                }
                m_swapchain_image_views.clear();
            }

            if (dev != VK_NULL_HANDLE && m_swapchain != VK_NULL_HANDLE) {
                BOOST_LOG_TRIVIAL(trace) << "Destroying swapchain: " << m_swapchain;
                vkDestroySwapchainKHR(dev, m_swapchain, nullptr);
                m_swapchain = VK_NULL_HANDLE;
            }

            m_swapchain_images.clear();
            m_format = { VK_FORMAT_UNDEFINED, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
            m_extent = { 0, 0 };
        }

        VkSurfaceFormatKHR Presentation::chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &formats) {
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

        VkPresentModeKHR Presentation::choosePresentMode(const std::vector<VkPresentModeKHR> &modes) {
            // Prefer mailbox over fifo, if it's available.
            for (const auto &mode: modes) {
                if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
                    return mode;
                }
            }

            return VK_PRESENT_MODE_FIFO_KHR;
        }

        VkExtent2D Presentation::chooseSwapExtent(const VkSurfaceCapabilitiesKHR &surf_caps) {
            if (surf_caps.currentExtent.width != UINT32_MAX) {
                return surf_caps.currentExtent;
            }

            GLFWwindow *wnd = window();
            int width, height;
            glfwGetWindowSize(wnd, &width, &height);

            VkExtent2D extent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
            extent.width  = std::max(surf_caps.minImageExtent.width,  std::min(surf_caps.maxImageExtent.width,  extent.width));
            extent.height = std::max(surf_caps.minImageExtent.height, std::min(surf_caps.maxImageExtent.height, extent.height));
            return extent;
        }

        GLFWwindow* Presentation::window() {
            return m_parent->window();
        }

        VkPhysicalDevice& Presentation::physicalDevice() {
            return m_parent->physicalDevice();
        }

        VkDevice& Presentation::device() {
            return m_parent->device();
        }

        VkSurfaceKHR& Presentation::surface() {
            return m_parent->surface();
        }

        CommandQueues& Presentation::queues() {
            return m_parent->queues();
        }

        System::System(GLFWwindow *window)
            : m_window{window},
              m_instance{VK_NULL_HANDLE},
              m_device{this}
        {}

        System::~System() {
            dispose();
        }

        bool System::initialize() {
            if (m_instance == VK_NULL_HANDLE) {
                logGlobalExtensions();
                logGlobalLayers();

                std::vector<const char*> extension_names;
                uint32_t glfw_extension_count;
                const char **glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
                for (unsigned int i = 0; i < glfw_extension_count; ++i) {
                    BOOST_LOG_TRIVIAL(trace) << "GLFW requires extension: " << glfw_extensions[i];
                    extension_names.emplace_back(glfw_extensions[i]);
                }

                std::vector<const char*> layer_names;
                layer_names.emplace_back("VK_LAYER_LUNARG_standard_validation");

                VkInstanceCreateInfo inst_ci;
                inst_ci.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
                inst_ci.pNext = nullptr;
                inst_ci.flags = 0;
                inst_ci.pApplicationInfo = nullptr;
                inst_ci.enabledLayerCount = layer_names.size();
                inst_ci.ppEnabledLayerNames = layer_names.data();
                inst_ci.enabledExtensionCount = static_cast<uint32_t>(extension_names.size());
                inst_ci.ppEnabledExtensionNames = extension_names.data();

                VkResult rslt = vkCreateInstance(&inst_ci, nullptr, &m_instance);
                if (rslt == VK_SUCCESS) {
                    BOOST_LOG_TRIVIAL(trace) << "Vulkan instance created: " << m_instance;
                } else {
                    BOOST_LOG_TRIVIAL(error) << "Error creating Vulkan instance: " << rslt;
                    return false;
                }
            }

            if (m_surface == VK_NULL_HANDLE) {
                VkResult rslt = glfwCreateWindowSurface(instance(), window(), nullptr, &m_surface);
                if (rslt == VK_SUCCESS) {
                    BOOST_LOG_TRIVIAL(trace) << "Created surface: " << m_surface;
                } else {
                    BOOST_LOG_TRIVIAL(error) << "Error creating surface: " << rslt;
                    return false;
                }
            }

            return m_device.initialize();
        }

        void System::dispose() {
            m_device.dispose();

            if (m_instance != VK_NULL_HANDLE && m_surface != VK_NULL_HANDLE) {
                BOOST_LOG_TRIVIAL(trace) << "Destroying surface: " << m_surface;
                vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
                m_surface = VK_NULL_HANDLE;
            }

            if (m_instance != VK_NULL_HANDLE) {
                BOOST_LOG_TRIVIAL(trace) << "Destroying Vulkan instance: " << m_instance;
                vkDestroyInstance(m_instance, nullptr);
                m_instance = VK_NULL_HANDLE;
            }
        }
    }

    // bool Graphics::initCommandQueue() {
    //     VkCommandPoolCreateInfo cmd_pool_info;
    //     cmd_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    //     cmd_pool_info.pNext = nullptr;
    //     cmd_pool_info.flags = 0;
    //     cmd_pool_info.queueFamilyIndex = queue.family;

    //     VkResult rslt = vkCreateCommandPool(device, &cmd_pool_info, nullptr, &queue.pool);

    //     if (rslt == VK_SUCCESS) {
    //         std::cout << "Command pool created: " << queue.pool << std::endl;
    //     } else {
    //         std::cerr << "Error: " << rslt << " command pool = " << queue.pool << std::endl;
    //         return false;
    //     }

    //     VkCommandBufferAllocateInfo cmd_buf_info;
    //     cmd_buf_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    //     cmd_buf_info.pNext = nullptr;
    //     cmd_buf_info.commandPool = queue.pool;
    //     cmd_buf_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    //     cmd_buf_info.commandBufferCount = 1;

    //     rslt = vkAllocateCommandBuffers(device, &cmd_buf_info, &queue.buffer);

    //     if (rslt == VK_SUCCESS) {
    //         std::cout << "Command buffer created: " << queue.buffer << std::endl;
    //         return true;
    //     } else {
    //         std::cerr << "Error: " << rslt << " command buf = " << queue.buffer << std::endl;
    //         return false;
    //     }
    // }

    // bool Graphics::initSwapchain() {
    // }

    // bool Graphics::initDepthBuffer() {
    //     VkSurfaceCapabilitiesKHR surf_caps;
    //     vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &surf_caps);

    //     VkImageCreateInfo create_info;
    //     create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    //     create_info.pNext = nullptr;
    //     create_info.flags = 0;
    //     create_info.imageType = VK_IMAGE_TYPE_2D;
    //     create_info.format = VK_FORMAT_D16_UNORM;
    //     create_info.extent.width = surf_caps.currentExtent.width;
    //     create_info.extent.height = surf_caps.currentExtent.height;
    //     create_info.extent.depth = 1;
    //     create_info.mipLevels = 1;
    //     create_info.arrayLayers = 1;
    //     create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    //     create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    //     create_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    //     create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    //     create_info.queueFamilyIndexCount = 1;
    //     create_info.pQueueFamilyIndices = &queue.family;
    //     create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    //     VkResult rslt = vkCreateImage(device, &create_info, nullptr, &depth_buffer.image);
    //     if (rslt == VK_SUCCESS) {
    //         std::cout << "Created depth buffer image: " << depth_buffer.image << std::endl;
    //     } else {
    //         std::cerr << "Error: " << rslt << " depth_buffer_image = " << depth_buffer.image << std::endl;
    //         return false;
    //     }

    //     VkMemoryRequirements mem_req;
    //     vkGetImageMemoryRequirements(device, depth_buffer.image, &mem_req);

    //     std::cout << "Depth buffer requires " << mem_req.size << " bytes aligned at " << mem_req.alignment
    //               << " memory type bits = " << mem_req.memoryTypeBits << std::endl;

    //     VkMemoryAllocateInfo alloc_info;
    //     alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    //     alloc_info.pNext = nullptr;
    //     alloc_info.allocationSize = mem_req.size;
    //     alloc_info.memoryTypeIndex = UINT32_MAX;

    //     VkPhysicalDeviceMemoryProperties mem_props;
    //     vkGetPhysicalDeviceMemoryProperties(physical_device, &mem_props);

    //     uint32_t type_bits = mem_req.memoryTypeBits;
    //     for (unsigned int i = 0; i < mem_props.memoryTypeCount; ++i) {
    //         if ((type_bits & 1) == 1) {
    //             alloc_info.memoryTypeIndex = i;
    //             break;
    //         }
    //         type_bits >>= 1;
    //     }

    //     if (alloc_info.memoryTypeIndex >= mem_props.memoryTypeCount) {
    //         std::cerr << "Could not find an appropriate memory type for depth buffer." << std::endl;
    //         return false;
    //     } else {
    //         std::cout << "Chose memory type " << alloc_info.memoryTypeIndex << " for depth buffer." << std::endl;
    //     }

    //     rslt = vkAllocateMemory(device, &alloc_info, nullptr, &depth_buffer.memory);

    //     if (rslt == VK_SUCCESS) {
    //         std::cout << "Allocated memory for depth buffer: " << depth_buffer.memory << std::endl;
    //     } else {
    //         std::cerr << "Could not allocate memory for depth buffer: " << rslt << " depth buffer memory = " << depth_buffer.memory << std::endl;
    //         return false;
    //     }

    //     rslt = vkBindImageMemory(device, depth_buffer.image, depth_buffer.memory, 0);

    //     if (rslt != VK_SUCCESS) {
    //         std::cerr << "Could not bind memory " << depth_buffer.memory << " to depth buffer " << depth_buffer.image << ": " << rslt << std::endl;
    //         return false;
    //     }

    //     VkImageViewCreateInfo iv_create_info;
    //     iv_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    //     iv_create_info.pNext = nullptr;
    //     iv_create_info.flags = 0;
    //     iv_create_info.image = depth_buffer.image;
    //     iv_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    //     iv_create_info.format = create_info.format;
    //     iv_create_info.components.r = VK_COMPONENT_SWIZZLE_R;
    //     iv_create_info.components.g = VK_COMPONENT_SWIZZLE_G;
    //     iv_create_info.components.b = VK_COMPONENT_SWIZZLE_B;
    //     iv_create_info.components.a = VK_COMPONENT_SWIZZLE_A;
    //     iv_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    //     iv_create_info.subresourceRange.baseMipLevel = 0;
    //     iv_create_info.subresourceRange.levelCount = 1;
    //     iv_create_info.subresourceRange.baseArrayLayer = 0;
    //     iv_create_info.subresourceRange.layerCount = 1;

    //     rslt = vkCreateImageView(device, &iv_create_info, nullptr, &depth_buffer.view);

    //     if (rslt == VK_SUCCESS) {
    //         std::cout << "Created depth buffer image view: " << depth_buffer.view << std::endl;
    //     } else {
    //         std::cerr << "Could not create depth buffer image view: " << rslt << " depth buffer image view = " << depth_buffer.view << std::endl;
    //         return false;
    //     }

    //     return true;
    // }

    // bool Graphics::initUniformBuffer() {
    //     VkBufferCreateInfo buf_ci;
    //     buf_ci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    //     buf_ci.pNext = nullptr;
    //     buf_ci.flags = 0;
    //     buf_ci.size = sizeof(Uniforms);
    //     buf_ci.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    //     buf_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    //     buf_ci.queueFamilyIndexCount = 1;
    //     buf_ci.pQueueFamilyIndices = &queue.family;

    //     VkResult rslt = vkCreateBuffer(device, &buf_ci, nullptr, &uniform_buffer.buffer);
    //     if (rslt == VK_SUCCESS) {
    //         std::cout << "Created uniform buffer: " << uniform_buffer.buffer << std::endl;
    //     } else {
    //         std::cerr << "Could not create uniform buffer: " << rslt << " uniform_buffer.buffer = " << uniform_buffer.buffer << std::endl;
    //         return false;
    //     }

    //     VkMemoryRequirements mem_reqs;
    //     vkGetBufferMemoryRequirements(device, uniform_buffer.buffer, &mem_reqs);

    //     std::cout << "Uniform buffer requires " << mem_reqs.size << " bytes aligned at " << mem_reqs.alignment
    //               << " memory type bits = " << mem_reqs.memoryTypeBits << std::endl;

    //     VkMemoryAllocateInfo alloc_info;
    //     alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    //     alloc_info.pNext = nullptr;
    //     alloc_info.allocationSize = mem_reqs.size;
    //     alloc_info.memoryTypeIndex = UINT32_MAX;

    //     VkPhysicalDeviceMemoryProperties mem_props;
    //     vkGetPhysicalDeviceMemoryProperties(physical_device, &mem_props);

    //     uint32_t type_bits = mem_reqs.memoryTypeBits;
    //     for (unsigned int i = 0; i < mem_props.memoryTypeCount; ++i) {
    //         if (type_bits & (1 << i) &&
    //             mem_props.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT &&
    //             mem_props.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
    //         {
    //             alloc_info.memoryTypeIndex = i;
    //             break;
    //         }
    //     }

    //     if (alloc_info.memoryTypeIndex >= mem_props.memoryTypeCount) {
    //         std::cerr << "Could not find an appropriate memory type for uniform buffer." << std::endl;
    //         return false;
    //     } else {
    //         std::cout << "Chose memory type " << alloc_info.memoryTypeIndex << " for uniform buffer." << std::endl;
    //     }

    //     rslt = vkAllocateMemory(device, &alloc_info, nullptr, &uniform_buffer.memory);

    //     if (rslt == VK_SUCCESS) {
    //         std::cout << "Allocated memory for uniform buffer: " << uniform_buffer.memory << std::endl;
    //     } else {
    //         std::cerr << "Could not allocate memory for uniform buffer: " << rslt << " uniform buffer memory = " << uniform_buffer.memory << std::endl;
    //         return false;
    //     }

    //     Uniforms *mapped_uniforms = nullptr;
    //     rslt = vkMapMemory(device, uniform_buffer.memory, 0, mem_reqs.size, 0, (void**)(&mapped_uniforms));

    //     if (rslt != VK_SUCCESS) {
    //         std::cerr << "Could not map uniform buffer memory" << rslt << std::endl;
    //         return false;
    //     }

    //     *mapped_uniforms = uniforms;

    //     vkUnmapMemory(device, uniform_buffer.memory);

    //     rslt = vkBindBufferMemory(device, uniform_buffer.buffer, uniform_buffer.memory, 0);

    //     if (rslt != VK_SUCCESS) {
    //         std::cerr << "Could not bind buffer memory " << uniform_buffer.memory
    //                   << " to uniform buffer " << uniform_buffer.buffer
    //                   << ": " << rslt << std::endl;
    //         return false;
    //     }

    //     return true;
    // }

    // std::vector<char> loadShaderBytecode(const std::string &filename) {
    //     std::ifstream file(filename, std::ios::ate | std::ios::binary);
    //     size_t file_size = (size_t)file.tellg();
    //     std::vector<char> bytecode(file_size);
    //     file.seekg(0);
    //     file.read(bytecode.data(), file_size);
    //     file.close();
    //     return bytecode;
    // }

    // bool Graphics::initPipelineLayout() {
    //     std::vector<char> bytecode = loadShaderBytecode("shaders/unlit.vert.spv");
    //     VkShaderModuleCreateInfo shader_ci;
    //     shader_ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    //     shader_ci.pNext = nullptr;
    //     shader_ci.flags = 0;
    //     shader_ci.codeSize = bytecode.size();
    //     shader_ci.pCode = reinterpret_cast<uint32_t *>(bytecode.data());

    //     VkResult rslt = vkCreateShaderModule(device, &shader_ci, nullptr, &unlit_vertex.module);
    //     if (rslt != VK_SUCCESS) {
    //         std::cerr << "Failed to create unlit vertex shader: " << rslt << " vertex_shader = " << unlit_vertex.module << std::endl;
    //         return false;
    //     } else {
    //         std::cout << "Created unlit vertex shader module " << unlit_vertex.module << std::endl;
    //     }

    //     bytecode = loadShaderBytecode("shaders/unlit.frag.spv");
    //     shader_ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    //     shader_ci.pNext = nullptr;
    //     shader_ci.flags = 0;
    //     shader_ci.codeSize = bytecode.size();
    //     shader_ci.pCode = reinterpret_cast<uint32_t *>(bytecode.data());

    //     rslt = vkCreateShaderModule(device, &shader_ci, nullptr, &unlit_fragment.module);
    //     if (rslt != VK_SUCCESS) {
    //         std::cerr << "Failed to create unlit fragment shader: " << rslt << " vertex_shader = " << unlit_fragment.module << std::endl;
    //         return false;
    //     } else {
    //         std::cout << "Created unlit fragment shader module " << unlit_fragment.module << std::endl;
    //     }

    //     VkPipelineShaderStageCreateInfo vertex_stage_ci;
    //     vertex_stage_ci.sType = VK_STRUCTURE_TYPE_SHADER_STAGE_CREATE_INFO;
    //     vertex_stage_ci.pNext = nullptr;
    //     vertex_stage_ci.flags = 0;
    //     vertex_stage_ci.stage = VK_SHADER_STAGE_VERTEX_BIT;
    //     vertex_stage_ci.module = unlit_vertex.module;
    //     vertex_stage_ci.pName = "main";
    //     vertex_stage_ci.pSpecializationInfo = nullptr;

    //     return true;
    // }
}
