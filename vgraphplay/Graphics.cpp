// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#include "Graphics.h"

#include <fstream>
#include <iostream>

#include "VulkanOutput.h"

namespace vgraphplay {
    Graphics::Graphics()
        : window{nullptr},
          instance{VK_NULL_HANDLE},
          physical_device{VK_NULL_HANDLE},
          device{VK_NULL_HANDLE},
          surface{VK_NULL_HANDLE},
          queue{UINT32_MAX, VK_NULL_HANDLE, VK_NULL_HANDLE},
          swapchain{VK_NULL_HANDLE, {}},
          depth_buffer{VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE},
          uniforms{},
          uniform_buffer{VK_NULL_HANDLE, VK_NULL_HANDLE},
          unlit_vertex{{}, VK_NULL_HANDLE},
          unlit_fragment{{}, VK_NULL_HANDLE}
    {}

    Graphics::~Graphics() {
        shutDown();
    }

    void Graphics::shutDown() {
        if (device != VK_NULL_HANDLE && unlit_vertex.module != VK_NULL_HANDLE) {
            std::cout << "Destroying unlit vertex shader module: "
                      << unlit_vertex.module << std::endl;
            vkDestroyShaderModule(device, unlit_vertex.module, nullptr);
        }

        if (device != VK_NULL_HANDLE && unlit_fragment.module != VK_NULL_HANDLE) {
            std::cout << "Destroying unlit fragment shader module: "
                      << unlit_fragment.module << std::endl;
            vkDestroyShaderModule(device, unlit_fragment.module, nullptr);
        }

        if (device != VK_NULL_HANDLE && uniform_buffer.buffer != VK_NULL_HANDLE) {
            std::cout << "Destroying uniform buffer: "
                      << uniform_buffer.buffer << std::endl;
            vkDestroyBuffer(device, uniform_buffer.buffer, nullptr);
            uniform_buffer.buffer = VK_NULL_HANDLE;
        }

        if (device != VK_NULL_HANDLE && uniform_buffer.memory != VK_NULL_HANDLE) {
            std::cout << "Freeing uniform buffer memory: "
                      << uniform_buffer.memory << std::endl;
            vkFreeMemory(device, uniform_buffer.memory, nullptr);
            uniform_buffer.memory = VK_NULL_HANDLE;
        }

        if (device != VK_NULL_HANDLE && depth_buffer.view != VK_NULL_HANDLE) {
            std::cout << "Destroying depth buffer image view: "
                      << depth_buffer.view << std::endl;
            vkDestroyImageView(device, depth_buffer.view, nullptr);
            depth_buffer.view = VK_NULL_HANDLE;
        }

        if (device != VK_NULL_HANDLE && depth_buffer.image != VK_NULL_HANDLE) {
            std::cout << "Destroying depth buffer image: "
                      << depth_buffer.image << std::endl;
            vkDestroyImage(device, depth_buffer.image, nullptr);
            depth_buffer.image = VK_NULL_HANDLE;
        }

        if (device != VK_NULL_HANDLE && depth_buffer.memory != VK_NULL_HANDLE) {
            std::cout << "Freeing depth buffer memory: "
                      << depth_buffer.memory << std::endl;
            vkFreeMemory(device, depth_buffer.memory, nullptr);
            depth_buffer.memory = VK_NULL_HANDLE;
        }

        if (device != VK_NULL_HANDLE && queue.pool != VK_NULL_HANDLE) {
            if (queue.buffer != VK_NULL_HANDLE) {
                std::cout << "Freeing command buffer: "
                          << queue.buffer << std::endl;
                vkFreeCommandBuffers(device, queue.pool, 1, &queue.buffer);
                queue.buffer = VK_NULL_HANDLE;
            }

            std::cout << "Destroying command pool: "
                      << queue.pool << std::endl;
            vkDestroyCommandPool(device, queue.pool, nullptr);
            queue.pool = VK_NULL_HANDLE;
        }

        if (device != VK_NULL_HANDLE && swapchain.swapchain != VK_NULL_HANDLE) {
            for (auto&& image : swapchain.images) {
                std::cout << "Destroying swapchain image view: "
                          << image.view << std::endl;
                vkDestroyImageView(device, image.view, nullptr);
                image.view = VK_NULL_HANDLE;
            }

            std::cout << "Destroying swapchain: "
                      << swapchain.swapchain << std::endl;
            vkDestroySwapchainKHR(device, swapchain.swapchain, nullptr);
            swapchain.swapchain = VK_NULL_HANDLE;
        }

        if (device != VK_NULL_HANDLE) {
            std::cout << "Destroying device: "
                      << device << std::endl;
            vkDestroyDevice(device, nullptr);
            device = VK_NULL_HANDLE;
        }

        if (instance != VK_NULL_HANDLE && surface != VK_NULL_HANDLE) {
            std::cout << "Destroying surface: "
                      << surface << std::endl;
            vkDestroySurfaceKHR(instance, surface, nullptr);
            surface = VK_NULL_HANDLE;
        }

        if (instance != VK_NULL_HANDLE) {
            std::cout << "Destroying instance: "
                      << instance << std::endl;
            vkDestroyInstance(instance, nullptr);
            instance = VK_NULL_HANDLE;
        }
    }

    bool Graphics::initialize(GLFWwindow *_window) {
        if (!initInstance()) {
            std::cerr << "Failed to create Vulkan instance." << std::endl;
            return false;
        }

        window = _window;
        glfwCreateWindowSurface(instance, window, nullptr, &surface);

        if (!initDevice()) {
            std::cerr << "Failed to create Vulkan device." << std::endl;
            return false;
        }

        if (!initCommandQueue()) {
            std::cerr << "Failed to create Vulkan command queue." << std::endl;
            return false;
        }

        if (!initSwapchain()) {
            std::cerr << "Failed to create swapchain." << std::endl;
            return false;
        }

        if (!initDepthBuffer()) {
            std::cerr << "Failed to create depth buffer." << std::endl;
            return false;
        }

        if (!initUniformBuffer()) {
            std::cerr << "Failed to create uniform buffer." << std::endl;
            return false;
        }

        if (!initPipelineLayout()) {
            std::cerr << "Failed to create pipeline layout." << std::endl;
            return false;
        }

        return true;
    }

    bool Graphics::initInstance() {
        uint32_t num_extensions;
        vkEnumerateInstanceExtensionProperties(nullptr, &num_extensions, nullptr);
        std::vector<VkExtensionProperties> extensions(num_extensions);
        vkEnumerateInstanceExtensionProperties(nullptr, &num_extensions, extensions.data());

        for (auto&& extension : extensions) {
            std::cout << "Extension: " << extension << std::endl;
        }

        uint32_t num_layers;
        vkEnumerateInstanceLayerProperties(&num_layers, nullptr);
        std::vector<VkLayerProperties> layers(num_layers);
        vkEnumerateInstanceLayerProperties(&num_layers, layers.data());

        for (auto&& layer : layers) {
            std::cout << "Layer: " << layer << std::endl;

            uint32_t num_extensions;
            vkEnumerateInstanceExtensionProperties(layer.layerName, &num_extensions, nullptr);
            std::vector<VkExtensionProperties> extensions(num_extensions);
            vkEnumerateInstanceExtensionProperties(layer.layerName, &num_extensions, extensions.data());

            for (auto&& extension : extensions) {
                std::cout << "  Extension: " << extension << std::endl;
            }
        }

        VkInstanceCreateInfo create_info;
        create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        create_info.pNext = nullptr;
        create_info.pApplicationInfo = nullptr;
        create_info.enabledLayerCount = 0;
        create_info.ppEnabledLayerNames = nullptr;

        std::vector<const char*> extension_names;
        uint32_t glfw_extension_count;
        const char **glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
        for (unsigned int i = 0; i < glfw_extension_count; ++i) {
            std::cout << "GLFW requires extension: " << glfw_extensions[i] << std::endl;
            extension_names.emplace_back(glfw_extensions[i]);
        }

        create_info.enabledExtensionCount = static_cast<uint32_t>(extension_names.size());
        create_info.ppEnabledExtensionNames = extension_names.data();

        VkResult rslt = vkCreateInstance(&create_info, nullptr, &instance);
        if (rslt == VK_SUCCESS) {
            std::cout << "Instance created: " << instance << std::endl;
            return true;
        } else {
            std::cerr << "Error: " << rslt << " instance = " << instance << std::endl;
            return false;
        }
    }

    bool Graphics::initDevice() {
        uint32_t num_devices = 0;
        vkEnumeratePhysicalDevices(instance, &num_devices, nullptr);

        std::vector<VkPhysicalDevice> devices(num_devices);
        vkEnumeratePhysicalDevices(instance, &num_devices, devices.data());

        for (auto&& device : devices) {
            VkPhysicalDeviceProperties props;
            vkGetPhysicalDeviceProperties(device, &props);

            std::cout << "Physical device: (" << device << ") " << props << std::endl;

            uint32_t num_extensions;
            vkEnumerateDeviceExtensionProperties(device, nullptr, &num_extensions, nullptr);
            std::vector<VkExtensionProperties> extensions(num_extensions);
            vkEnumerateDeviceExtensionProperties(device, nullptr, &num_extensions, extensions.data());

            for (auto&& extension : extensions) {
                std::cout << "  Extension: " << extension << std::endl;
            }

            VkPhysicalDeviceMemoryProperties mem_props;
            vkGetPhysicalDeviceMemoryProperties(device, &mem_props);

            for (unsigned int i = 0; i < mem_props.memoryHeapCount; ++i) {
                std::cout << "  Memory heap " << i << ": " << mem_props.memoryHeaps[i] << std::endl;
            }

            for (unsigned int i = 0; i < mem_props.memoryTypeCount; ++i) {
                std::cout << "  Memory type " << i << ": " << mem_props.memoryTypes[i] << std::endl;
            }
        }

        // We probably want to make a better decision than this...
        physical_device = devices[0];

        uint32_t num_queue_families = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &num_queue_families, nullptr);
        std::vector<VkQueueFamilyProperties> queue_families(num_queue_families);
        vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &num_queue_families, queue_families.data());

        for (unsigned int i = 0; i < queue_families.size(); ++i) {
            std::cout << "  Queue family " << i << ": " << queue_families[i] << std::endl;

            VkBool32 supports_present;
            vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, surface, &supports_present);
            std::cout << "    Can present to surface: " << supports_present << std::endl;
        }

        // Choose the first graphics queue...
        for (unsigned int i = 0; i < queue_families.size(); ++i) {
            VkBool32 supports_present;
            vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, surface, &supports_present);

            if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT && supports_present == VK_TRUE) {
                queue.family = i;
                break;
            }
        }

        std::cout << "physical device = " << physical_device << std::endl
                  << "queue family = " << queue.family << std::endl;

        float queue_priority = 1.0;
        VkDeviceQueueCreateInfo queue_info;
        queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_info.pNext = nullptr;
        queue_info.flags = 0;
        queue_info.queueFamilyIndex = queue.family;
        queue_info.queueCount = 1;
        queue_info.pQueuePriorities = &queue_priority;

        VkDeviceCreateInfo device_info;
        device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        device_info.pNext = nullptr;
        device_info.queueCreateInfoCount = 1;
        device_info.pQueueCreateInfos = &queue_info;
        device_info.enabledLayerCount = 0;
        device_info.ppEnabledLayerNames = nullptr;
        device_info.pEnabledFeatures = nullptr;

        std::vector<const char*> extension_names;
        extension_names.emplace_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
        device_info.enabledExtensionCount = static_cast<uint32_t>(extension_names.size());
        device_info.ppEnabledExtensionNames = extension_names.data();

        VkResult rslt = vkCreateDevice(physical_device, &device_info, nullptr, &device);

        if (rslt == VK_SUCCESS) {
            std::cout << "Device Created: " << device << std::endl;
            return true;
        } else {
            std::cerr << "Error: " << rslt << " device = " << device << std::endl;
            return false;
        }
    }

    bool Graphics::initCommandQueue() {
        VkCommandPoolCreateInfo cmd_pool_info;
        cmd_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        cmd_pool_info.pNext = nullptr;
        cmd_pool_info.flags = 0;
        cmd_pool_info.queueFamilyIndex = queue.family;

        VkResult rslt = vkCreateCommandPool(device, &cmd_pool_info, nullptr, &queue.pool);

        if (rslt == VK_SUCCESS) {
            std::cout << "Command pool created: " << queue.pool << std::endl;
        } else {
            std::cerr << "Error: " << rslt << " command pool = " << queue.pool << std::endl;
            return false;
        }

        VkCommandBufferAllocateInfo cmd_buf_info;
        cmd_buf_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmd_buf_info.pNext = nullptr;
        cmd_buf_info.commandPool = queue.pool;
        cmd_buf_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmd_buf_info.commandBufferCount = 1;

        rslt = vkAllocateCommandBuffers(device, &cmd_buf_info, &queue.buffer);

        if (rslt == VK_SUCCESS) {
            std::cout << "Command buffer created: " << queue.buffer << std::endl;
            return true;
        } else {
            std::cerr << "Error: " << rslt << " command buf = " << queue.buffer << std::endl;
            return false;
        }
    }

    bool Graphics::initSwapchain() {
        VkSurfaceCapabilitiesKHR surf_caps;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &surf_caps);
        std::cout << "Surface capabilities: " << surf_caps << std::endl;

        uint32_t num_formats;
        vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &num_formats, nullptr);
        std::vector<VkSurfaceFormatKHR> formats(num_formats);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &num_formats, formats.data());

        for (auto&& format : formats) {
            std::cout << "Surface format: [ format: " << format.format << " color space: " << format.colorSpace << " ]" << std::endl;
        }

        uint32_t num_modes;
        vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &num_modes, nullptr);
        std::vector<VkPresentModeKHR> modes(num_modes);
        vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &num_modes, modes.data());

        for (auto&& mode : modes) {
            std::cout << "Surface present mode: " << mode << std::endl;
        }

        VkSwapchainCreateInfoKHR create_info;
        create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        create_info.pNext = nullptr;
        create_info.flags = 0;
        create_info.surface = surface;
        create_info.minImageCount = surf_caps.minImageCount;
        create_info.imageFormat = formats[0].format;
        create_info.imageColorSpace = formats[0].colorSpace;
        create_info.imageExtent = surf_caps.currentExtent;
        create_info.imageArrayLayers = 1;
        create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        create_info.queueFamilyIndexCount = 1;
        create_info.pQueueFamilyIndices = &queue.family;
        create_info.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        create_info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
        create_info.clipped = VK_TRUE;
        create_info.oldSwapchain = VK_NULL_HANDLE;

        VkResult rslt = vkCreateSwapchainKHR(device, &create_info, nullptr, &swapchain.swapchain);
        if (rslt == VK_SUCCESS) {
            std::cout << "Created swapchain: " << swapchain.swapchain << std::endl;
        } else {
            std::cerr << "Error: " << rslt << " swapchain = " << swapchain.swapchain << std::endl;
            return false;
        }

        uint32_t num_swapchain_images = 0;
        vkGetSwapchainImagesKHR(device, swapchain.swapchain, &num_swapchain_images, nullptr);

        swapchain.images.resize(num_swapchain_images);

        std::vector<VkImage> swapchain_images(num_swapchain_images);
        vkGetSwapchainImagesKHR(device, swapchain.swapchain, &num_swapchain_images, swapchain_images.data());

        for (unsigned int i = 0; i < swapchain_images.size(); ++i) {
            std::cout << "Swapchain image: " << swapchain_images[i] << std::endl;
            swapchain.images[i].image = swapchain_images[i];

            VkImageViewCreateInfo create_info;
            create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            create_info.pNext = nullptr;
            create_info.flags = 0;
            create_info.image = swapchain.images[i].image;
            create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
            create_info.format = formats[0].format;
            create_info.components.r = VK_COMPONENT_SWIZZLE_R;
            create_info.components.g = VK_COMPONENT_SWIZZLE_G;
            create_info.components.b = VK_COMPONENT_SWIZZLE_B;
            create_info.components.a = VK_COMPONENT_SWIZZLE_A;
            create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            create_info.subresourceRange.baseMipLevel = 0;
            create_info.subresourceRange.levelCount = 1;
            create_info.subresourceRange.baseArrayLayer = 0;
            create_info.subresourceRange.layerCount = 1;

            vkCreateImageView(device, &create_info, nullptr, &swapchain.images[i].view);

            if (rslt == VK_SUCCESS) {
                std::cout << "Created image view for swapchain " << swapchain.swapchain << " image " << i
                          << ": " << swapchain.images[i].view << std::endl;
            } else {
                std::cerr << "Could not create image view for swapchain " << swapchain.swapchain << " image " << i
                          << ": " << rslt << " view = " << swapchain.images[i].view << std::endl;
                return false;
            }
        }

        return true;
    }

    bool Graphics::initDepthBuffer() {
        VkSurfaceCapabilitiesKHR surf_caps;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &surf_caps);

        VkImageCreateInfo create_info;
        create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        create_info.pNext = nullptr;
        create_info.flags = 0;
        create_info.imageType = VK_IMAGE_TYPE_2D;
        create_info.format = VK_FORMAT_D16_UNORM;
        create_info.extent.width = surf_caps.currentExtent.width;
        create_info.extent.height = surf_caps.currentExtent.height;
        create_info.extent.depth = 1;
        create_info.mipLevels = 1;
        create_info.arrayLayers = 1;
        create_info.samples = VK_SAMPLE_COUNT_1_BIT;
        create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
        create_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        create_info.queueFamilyIndexCount = 1;
        create_info.pQueueFamilyIndices = &queue.family;
        create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        VkResult rslt = vkCreateImage(device, &create_info, nullptr, &depth_buffer.image);
        if (rslt == VK_SUCCESS) {
            std::cout << "Created depth buffer image: " << depth_buffer.image << std::endl;
        } else {
            std::cerr << "Error: " << rslt << " depth_buffer_image = " << depth_buffer.image << std::endl;
            return false;
        }

        VkMemoryRequirements mem_req;
        vkGetImageMemoryRequirements(device, depth_buffer.image, &mem_req);

        std::cout << "Depth buffer requires " << mem_req.size << " bytes aligned at " << mem_req.alignment
                  << " memory type bits = " << mem_req.memoryTypeBits << std::endl;

        VkMemoryAllocateInfo alloc_info;
        alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.pNext = nullptr;
        alloc_info.allocationSize = mem_req.size;
        alloc_info.memoryTypeIndex = UINT32_MAX;

        VkPhysicalDeviceMemoryProperties mem_props;
        vkGetPhysicalDeviceMemoryProperties(physical_device, &mem_props);

        uint32_t type_bits = mem_req.memoryTypeBits;
        for (unsigned int i = 0; i < mem_props.memoryTypeCount; ++i) {
            if ((type_bits & 1) == 1) {
                alloc_info.memoryTypeIndex = i;
                break;
            }
            type_bits >>= 1;
        }

        if (alloc_info.memoryTypeIndex >= mem_props.memoryTypeCount) {
            std::cerr << "Could not find an appropriate memory type for depth buffer." << std::endl;
            return false;
        } else {
            std::cout << "Chose memory type " << alloc_info.memoryTypeIndex << " for depth buffer." << std::endl;
        }

        rslt = vkAllocateMemory(device, &alloc_info, nullptr, &depth_buffer.memory);

        if (rslt == VK_SUCCESS) {
            std::cout << "Allocated memory for depth buffer: " << depth_buffer.memory << std::endl;
        } else {
            std::cerr << "Could not allocate memory for depth buffer: " << rslt << " depth buffer memory = " << depth_buffer.memory << std::endl;
            return false;
        }

        rslt = vkBindImageMemory(device, depth_buffer.image, depth_buffer.memory, 0);

        if (rslt != VK_SUCCESS) {
            std::cerr << "Could not bind memory " << depth_buffer.memory << " to depth buffer " << depth_buffer.image << ": " << rslt << std::endl;
            return false;
        }

        VkImageViewCreateInfo iv_create_info;
        iv_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        iv_create_info.pNext = nullptr;
        iv_create_info.flags = 0;
        iv_create_info.image = depth_buffer.image;
        iv_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        iv_create_info.format = create_info.format;
        iv_create_info.components.r = VK_COMPONENT_SWIZZLE_R;
        iv_create_info.components.g = VK_COMPONENT_SWIZZLE_G;
        iv_create_info.components.b = VK_COMPONENT_SWIZZLE_B;
        iv_create_info.components.a = VK_COMPONENT_SWIZZLE_A;
        iv_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        iv_create_info.subresourceRange.baseMipLevel = 0;
        iv_create_info.subresourceRange.levelCount = 1;
        iv_create_info.subresourceRange.baseArrayLayer = 0;
        iv_create_info.subresourceRange.layerCount = 1;

        rslt = vkCreateImageView(device, &iv_create_info, nullptr, &depth_buffer.view);

        if (rslt == VK_SUCCESS) {
            std::cout << "Created depth buffer image view: " << depth_buffer.view << std::endl;
        } else {
            std::cerr << "Could not create depth buffer image view: " << rslt << " depth buffer image view = " << depth_buffer.view << std::endl;
            return false;
        }

        return true;
    }

    bool Graphics::initUniformBuffer() {
        VkBufferCreateInfo buf_ci;
        buf_ci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buf_ci.pNext = nullptr;
        buf_ci.flags = 0;
        buf_ci.size = sizeof(Uniforms);
        buf_ci.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        buf_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        buf_ci.queueFamilyIndexCount = 1;
        buf_ci.pQueueFamilyIndices = &queue.family;

        VkResult rslt = vkCreateBuffer(device, &buf_ci, nullptr, &uniform_buffer.buffer);
        if (rslt == VK_SUCCESS) {
            std::cout << "Created uniform buffer: " << uniform_buffer.buffer << std::endl;
        } else {
            std::cerr << "Could not create uniform buffer: " << rslt << " uniform_buffer.buffer = " << uniform_buffer.buffer << std::endl;
            return false;
        }

        VkMemoryRequirements mem_reqs;
        vkGetBufferMemoryRequirements(device, uniform_buffer.buffer, &mem_reqs);

        std::cout << "Uniform buffer requires " << mem_reqs.size << " bytes aligned at " << mem_reqs.alignment
                  << " memory type bits = " << mem_reqs.memoryTypeBits << std::endl;

        VkMemoryAllocateInfo alloc_info;
        alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.pNext = nullptr;
        alloc_info.allocationSize = mem_reqs.size;
        alloc_info.memoryTypeIndex = UINT32_MAX;

        VkPhysicalDeviceMemoryProperties mem_props;
        vkGetPhysicalDeviceMemoryProperties(physical_device, &mem_props);

        uint32_t type_bits = mem_reqs.memoryTypeBits;
        for (unsigned int i = 0; i < mem_props.memoryTypeCount; ++i) {
            if (type_bits & (1 << i) &&
                mem_props.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT &&
                mem_props.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
            {
                alloc_info.memoryTypeIndex = i;
                break;
            }
        }

        if (alloc_info.memoryTypeIndex >= mem_props.memoryTypeCount) {
            std::cerr << "Could not find an appropriate memory type for uniform buffer." << std::endl;
            return false;
        } else {
            std::cout << "Chose memory type " << alloc_info.memoryTypeIndex << " for uniform buffer." << std::endl;
        }

        rslt = vkAllocateMemory(device, &alloc_info, nullptr, &uniform_buffer.memory);

        if (rslt == VK_SUCCESS) {
            std::cout << "Allocated memory for uniform buffer: " << uniform_buffer.memory << std::endl;
        } else {
            std::cerr << "Could not allocate memory for uniform buffer: " << rslt << " uniform buffer memory = " << uniform_buffer.memory << std::endl;
            return false;
        }

        Uniforms *mapped_uniforms = nullptr;
        rslt = vkMapMemory(device, uniform_buffer.memory, 0, mem_reqs.size, 0, (void**)(&mapped_uniforms));

        if (rslt != VK_SUCCESS) {
            std::cerr << "Could not map uniform buffer memory" << rslt << std::endl;
            return false;
        }

        *mapped_uniforms = uniforms;

        vkUnmapMemory(device, uniform_buffer.memory);

        rslt = vkBindBufferMemory(device, uniform_buffer.buffer, uniform_buffer.memory, 0);

        if (rslt != VK_SUCCESS) {
            std::cerr << "Could not bind buffer memory " << uniform_buffer.memory
                      << " to uniform buffer " << uniform_buffer.buffer
                      << ": " << rslt << std::endl;
            return false;
        }

        return true;
    }

    std::vector<char> loadShaderBytecode(const std::string &filename) {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);
        size_t file_size = (size_t)file.tellg();
        std::vector<char> bytecode(file_size);
        file.seekg(0);
        file.read(bytecode.data(), file_size);
        file.close();
        return bytecode;
    }

    bool Graphics::initPipelineLayout() {
        std::vector<char> bytecode = loadShaderBytecode("shaders/unlit.vert.spv");
        VkShaderModuleCreateInfo shader_ci;
        shader_ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        shader_ci.pNext = nullptr;
        shader_ci.flags = 0;
        shader_ci.codeSize = bytecode.size();
        shader_ci.pCode = reinterpret_cast<uint32_t *>(bytecode.data());

        VkResult rslt = vkCreateShaderModule(device, &shader_ci, nullptr, &unlit_vertex.module);
        if (rslt != VK_SUCCESS) {
            std::cerr << "Failed to create unlit vertex shader: " << rslt << " vertex_shader = " << unlit_vertex.module << std::endl;
            return false;
        } else {
            std::cout << "Created unlit vertex shader module " << unlit_vertex.module << std::endl;
        }

        bytecode = loadShaderBytecode("shaders/unlit.frag.spv");
        shader_ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        shader_ci.pNext = nullptr;
        shader_ci.flags = 0;
        shader_ci.codeSize = bytecode.size();
        shader_ci.pCode = reinterpret_cast<uint32_t *>(bytecode.data());

        rslt = vkCreateShaderModule(device, &shader_ci, nullptr, &unlit_fragment.module);
        if (rslt != VK_SUCCESS) {
            std::cerr << "Failed to create unlit fragment shader: " << rslt << " vertex_shader = " << unlit_fragment.module << std::endl;
            return false;
        } else {
            std::cout << "Created unlit fragment shader module " << unlit_fragment.module << std::endl;
        }

        return true;
    }
}
