// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#ifndef _VGRAPHPLAY_VGRAPHPLAY_GRAPHICS_H_
#define _VGRAPHPLAY_VGRAPHPLAY_GRAPHICS_H_

#include <vector>

#include "vulkan.h"

namespace vgraphplay {
    class System;

    struct CommandQueueInfo {
        uint32_t family;
        VkCommandPool pool;
        VkCommandBuffer buffer;
    };

    struct SwapchainImageInfo {
        VkImage image;
        VkImageView view;
    };

    struct SwapchainInfo {
        VkSwapchainKHR swapchain;
        std::vector<SwapchainImageInfo> images;
    };

    struct ImageInfo {
        VkImage image;
        VkImageView view;
        VkDeviceMemory memory;
    };

    class Graphics {
    public:
        VkInstance instance;
        VkPhysicalDevice physical_device;
        VkDevice device;
        VkSurfaceKHR surface;

        CommandQueueInfo queue;
        SwapchainInfo swapchain;
        ImageInfo depth_buffer;

        // Create the object; doesn't set up the Vulkan stuff.
        Graphics();
        ~Graphics();

        // Set up all the Vulkan stuff.
        bool initialize(System *sys);

    protected:
        // The sub-parts of initialize.
        bool initInstance(System *sys);
        bool initDevice();
        bool initCommandQueue();
        bool initSwapchain();
        bool initDepthBuffer();
        bool initUniformBuffer();
    };
}

#endif
