// -*- mode: c++; c-basic-offset: 4; -*-

#include <cstdio>
#include <vulkan/vulkan.h>

int main(int argc, char **argv) {
    VkInstanceCreateInfo vk_info;
    VkInstance inst = 0;
    VkResult res;

    vk_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    vk_info.pNext = NULL;
    vk_info.pApplicationInfo = NULL;
    vk_info.enabledLayerCount = 0;
    vk_info.ppEnabledLayerNames = NULL;
    vk_info.enabledExtensionCount = 0;
    vk_info.ppEnabledExtensionNames = NULL;
    res = vkCreateInstance(&vk_info, NULL, &inst);

    if (res != VK_SUCCESS) {
        std::printf("Error: %d\n", res);
        return 1;
    };

    std::printf("Device created: %p\n", inst);

    vkDestroyInstance(inst, NULL);
    return 0;
}
