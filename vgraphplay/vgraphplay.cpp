// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#include <iostream>
#include <vector>

#include <vulkan/vulkan.h>

#include "utils.h"

class VGraphplayApp {
public:
    VGraphplayApp()
        : instance{nullptr},
          physical_devices{}
    {}

    ~VGraphplayApp() {
        if (instance != nullptr) {
            vkDestroyInstance(instance, nullptr);
        }
    }

    VkInstance instance;
    std::vector<VkPhysicalDevice> physical_devices;
};

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

    uint32_t num_devices = 0;
    vkEnumeratePhysicalDevices(app.instance, &num_devices, nullptr);
    app.physical_devices.resize(num_devices);
    vkEnumeratePhysicalDevices(app.instance, &num_devices, app.physical_devices.data());

    for (auto&& device : app.physical_devices) {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(device, &props);
        std::cout << "Physical device: " << props << std::endl;
    }
}

int main(int argc, char **argv) {
    VGraphplayApp app;
    initVulkan(app);
    return 0;
}
