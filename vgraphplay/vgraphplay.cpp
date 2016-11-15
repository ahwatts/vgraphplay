// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#include <iostream>
#include <vector>

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

        if (device != VK_NULL_HANDLE) {
            std::cout << "Destroying (logical) device: " << device << std::endl;
            vkDestroyDevice(device, nullptr);
        }

        if (instance != VK_NULL_HANDLE) {
            std::cout << "Destroying instance: " << instance << std::endl;
            vkDestroyInstance(instance, nullptr);
        }
    }

    bool initInstance() {
        VkInstanceCreateInfo create_info;
        create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        create_info.pNext = nullptr;
        create_info.pApplicationInfo = nullptr;
        create_info.enabledLayerCount = 0;
        create_info.ppEnabledLayerNames = nullptr;
        create_info.enabledExtensionCount = 0;
        create_info.ppEnabledExtensionNames = nullptr;

        VkResult rslt = vkCreateInstance(&create_info, nullptr, &instance);
        if (rslt == VK_SUCCESS) {
            std::cout << "Instance created: " << instance << std::endl;
            return true;
        } else {
            std::cerr << "Error: " << rslt << " instance = " << instance << std::endl;
            return false;
        }
    }

    bool initDevice() {
        uint32_t num_devices = 0;
        vkEnumeratePhysicalDevices(instance, &num_devices, nullptr);

        std::vector<VkPhysicalDevice> devices(num_devices);
        vkEnumeratePhysicalDevices(instance, &num_devices, devices.data());

        for (auto&& device : devices) {
            VkPhysicalDeviceProperties props;
            vkGetPhysicalDeviceProperties(device, &props);

            std::cout << "Physical device: " << props << std::endl;

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
        }

        // Choose the first graphics queue...
        for (unsigned int i = 0; i < queue_families.size(); ++i) {
            if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                queue_family = i;
                break;
            }
        }

        std::cout << "physical device = " << physical_device << std::endl
                  << "queue family = " << queue_family << std::endl;

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
        device_info.enabledExtensionCount = 0;
        device_info.ppEnabledExtensionNames = nullptr;
        device_info.enabledLayerCount = 0;
        device_info.ppEnabledLayerNames = nullptr;
        device_info.pEnabledFeatures = nullptr;

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

    VkInstance instance;
    VkPhysicalDevice physical_device;
    VkDevice device;
    VkCommandPool cmd_pool;
    VkCommandBuffer cmd_buf;

    uint32_t queue_family;
};

int main(int argc, char **argv) {
    VGraphplayApp app;

    if (!app.initInstance()) {
        std::exit(1);
    }

    if (!app.initDevice()) {
        std::exit(1);
    }

    if (!app.initCommandBuffer()) {
        std::exit(1);
    }

    return 0;
}
