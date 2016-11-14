// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#include <iostream>
#include <vector>

#include <vulkan/vulkan.h>

#include "utils.h"

class VGraphplayApp {
public:
    VGraphplayApp()
        : instance{nullptr},
          physical_device{}
    {}

    ~VGraphplayApp() {
        if (instance != nullptr) {
            vkDestroyInstance(instance, nullptr);
        }
    }

    VkInstance instance;
    VkPhysicalDevice physical_device;
};

VkPhysicalDevice choosePhysicalDevice(VkInstance &instance) {
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

        uint32_t num_queue_families = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &num_queue_families, nullptr);
        std::vector<VkQueueFamilyProperties> queue_families(num_queue_families);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &num_queue_families, queue_families.data());

        for (auto&& queue_family : queue_families) {
            std::cout << "  Queue family: " << queue_family << std::endl;
        }
    }

    // We probably want to make a better decision than this...
    return devices[0];
}

void initVulkan(VGraphplayApp &app) {
    VkInstanceCreateInfo vk_info;
    VkResult rslt;

    vk_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    vk_info.pNext = nullptr;
    vk_info.pApplicationInfo = nullptr;
    vk_info.enabledLayerCount = 0;
    vk_info.ppEnabledLayerNames = nullptr;
    vk_info.enabledExtensionCount = 0;
    vk_info.ppEnabledExtensionNames = nullptr;
    rslt = vkCreateInstance(&vk_info, nullptr, &app.instance);

    if (rslt == VK_SUCCESS) {
        std::cout << "Device created: " << app.instance << std::endl;
    } else {
        std::cerr << "Error: " << rslt << " instance = " << app.instance << std::endl;
        std::exit(1);
    }

    app.physical_device = choosePhysicalDevice(app.instance);
}

int main(int argc, char **argv) {
    VGraphplayApp app;
    initVulkan(app);
    return 0;
}
