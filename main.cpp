// -*- mode: c++; c-basic-offset: 4; -*-

#include <iostream>
#include <vulkan/vulkan.h>

class VGraphplayApp {
public:
    VGraphplayApp()
        : m_instance{nullptr}
    {
        VkInstanceCreateInfo vk_info;
        VkResult rslt;

        vk_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        vk_info.pNext = nullptr;
        vk_info.pApplicationInfo = nullptr;
        vk_info.enabledLayerCount = 0;
        vk_info.ppEnabledLayerNames = nullptr;
        vk_info.enabledExtensionCount = 0;
        vk_info.ppEnabledExtensionNames = nullptr;
        rslt = vkCreateInstance(&vk_info, nullptr, &m_instance);

        if (rslt == VK_SUCCESS) {
            std::cout << "Device created: " << m_instance << std::endl;
        } else {
            std::cout << "Error: " << rslt << " m_instance = " << m_instance << std::endl;
        }
    }

    ~VGraphplayApp() {
        if (m_instance != nullptr) {
            vkDestroyInstance(m_instance, nullptr);
        }
    }

private:
    VkInstance m_instance;
};

int main(int argc, char **argv) {
    VGraphplayApp app;
    return 0;
}
