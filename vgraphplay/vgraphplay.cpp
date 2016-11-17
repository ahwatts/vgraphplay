// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#include <iostream>
#include <vector>

#ifdef _WIN32

#include "win32.h"

#ifndef VK_USE_PLATFORM_WIN32_KHR
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#endif // _WIN32

#include <vulkan/vulkan.h>

#include "utils.h"

class VGraphplayApp {
public:
    VGraphplayApp()
        : instance{VK_NULL_HANDLE},
          physical_device{VK_NULL_HANDLE},
          device{VK_NULL_HANDLE},
          cmd_pool{VK_NULL_HANDLE},
          cmd_buf{VK_NULL_HANDLE},
          surface{VK_NULL_HANDLE},
          swapchain{VK_NULL_HANDLE},
          queue_family{UINT32_MAX}
    {}

    ~VGraphplayApp() {
        if (device != VK_NULL_HANDLE && cmd_pool != VK_NULL_HANDLE && cmd_buf != VK_NULL_HANDLE) {
            std::cout << "Freeing command buffer: " << cmd_buf << std::endl;
            vkFreeCommandBuffers(device, cmd_pool, 1, &cmd_buf);
        }

        if (device != VK_NULL_HANDLE && cmd_pool != VK_NULL_HANDLE) {
            std::cout << "Destroying command pool: " << cmd_pool << std::endl;
            vkDestroyCommandPool(device, cmd_pool, nullptr);
        }

        if (device != VK_NULL_HANDLE && swapchain != VK_NULL_HANDLE) {
            std::cout << "Destroying swapchain: " << swapchain << std::endl;
            vkDestroySwapchainKHR(device, swapchain, nullptr);
        }

        if (device != VK_NULL_HANDLE) {
            std::cout << "Destroying (logical) device: " << device << std::endl;
            vkDestroyDevice(device, nullptr);
        }

        if (instance != VK_NULL_HANDLE && surface != VK_NULL_HANDLE) {
            std::cout << "Destroying surface: " << surface << std::endl;
            vkDestroySurfaceKHR(instance, surface, nullptr);
        }

        if (instance != VK_NULL_HANDLE) {
            std::cout << "Destroying instance: " << instance << std::endl;
            vkDestroyInstance(instance, nullptr);
        }
    }

    bool initInstance() {
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

            // uint32_t num_extensions;
            // vkEnumerateInstanceExtensionProperties(layer.layerName, &num_extensions, nullptr);
            // std::vector<VkExtensionProperties> extensions(num_extensions);
            // vkEnumerateInstanceExtensionProperties(layer.layerName, &num_extensions, extensions.data());

            // for (auto&& extension : extensions) {
            //     std::cout << "  Extension: " << extension << std::endl;
            // }
        }

        VkInstanceCreateInfo create_info;
        create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        create_info.pNext = nullptr;
        create_info.pApplicationInfo = nullptr;
        create_info.enabledLayerCount = 0;
        create_info.ppEnabledLayerNames = nullptr;

        std::vector<const char*> extension_names;
        extension_names.emplace_back(VK_KHR_SURFACE_EXTENSION_NAME);
        extension_names.emplace_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
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

#ifdef _WIN32
    bool initSurface(HINSTANCE hInstance, HWND hWnd) {
        VkWin32SurfaceCreateInfoKHR create_info;
        create_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        create_info.pNext = nullptr;
        create_info.flags = 0;
        create_info.hinstance = hInstance;
        create_info.hwnd = hWnd;

        VkResult rslt = vkCreateWin32SurfaceKHR(instance, &create_info, nullptr, &surface);
        if (rslt == VK_SUCCESS) {
            std::cout << "Surface created: " << surface << std::endl;
            return true;
        } else {
            std::cerr << "Error: " << rslt << " surface = " << surface << std::endl;
            return false;
        }
    }
#endif

    bool chooseDevice() {
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
                queue_family = i;
                break;
            }
        }

        std::cout << "physical device = " << physical_device << std::endl
                  << "queue family = " << queue_family << std::endl;

        return true;
    }

    bool initDevice() {
        float queue_priority = 1.0;
        VkDeviceQueueCreateInfo queue_info;
        queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_info.pNext = nullptr;
        queue_info.flags = 0;
        queue_info.queueFamilyIndex = queue_family;
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

    bool initCommandBuffer() {
        VkCommandPoolCreateInfo cmd_pool_info;
        cmd_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        cmd_pool_info.pNext = nullptr;
        cmd_pool_info.flags = 0;
        cmd_pool_info.queueFamilyIndex = queue_family;

        VkResult rslt = vkCreateCommandPool(device, &cmd_pool_info, nullptr, &cmd_pool);

        if (rslt == VK_SUCCESS) {
            std::cout << "Command pool created: " << cmd_pool << std::endl;
        } else {
            std::cerr << "Error: " << rslt << " command pool = " << cmd_pool << std::endl;
            return false;
        }

        VkCommandBufferAllocateInfo cmd_buf_info;
        cmd_buf_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmd_buf_info.pNext = nullptr;
        cmd_buf_info.commandPool = cmd_pool;
        cmd_buf_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmd_buf_info.commandBufferCount = 1;

        rslt = vkAllocateCommandBuffers(device, &cmd_buf_info, &cmd_buf);

        if (rslt == VK_SUCCESS) {
            std::cout << "Command buffer created: " << cmd_buf << std::endl;
            return true;
        } else {
            std::cerr << "Error: " << rslt << " command buf = " << cmd_buf << std::endl;
            return false;
        }
    }

    bool initSwapchain() {
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
        create_info.pQueueFamilyIndices = &queue_family;
        create_info.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        create_info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
        create_info.clipped = VK_TRUE;
        create_info.oldSwapchain = VK_NULL_HANDLE;

        VkResult rslt = vkCreateSwapchainKHR(device, &create_info, nullptr, &swapchain);
        if (rslt == VK_SUCCESS) {
            std::cout << "Created swapchain: " << swapchain << std::endl;
            return true;
        } else {
            std::cerr << "Error: " << rslt << " swapchain = " << swapchain << std::endl;
            return false;
        }
    }

    VkInstance instance;
    VkPhysicalDevice physical_device;
    VkDevice device;
    VkCommandPool cmd_pool;
    VkCommandBuffer cmd_buf;
    VkSurfaceKHR surface;
    VkSwapchainKHR swapchain;

    uint32_t queue_family;
};

int main(int argc, char **argv) {
    VGraphplayApp app;

#ifdef _WIN32
    Win32Context win_ctx;
#endif

    if (!app.initInstance()) {
        std::exit(1);
    }

#ifdef _WIN32
    if (!win_ctx.initWindow(800, 600)) {
        std::exit(1);
    }

    if (!app.initSurface(win_ctx.hInstance, win_ctx.hWnd)) {
        std::exit(1);
    }
#endif

    if (!app.chooseDevice()) {
        std::exit(1);
    }

    if (!app.initDevice()) {
        std::exit(1);
    }

    if (!app.initSwapchain()) {
        std::exit(1);
    }

    if (!app.initCommandBuffer()) {
        std::exit(1);
    }

#ifdef _WIN32
    win_ctx.mainLoop();
#endif

    return 0;
}
