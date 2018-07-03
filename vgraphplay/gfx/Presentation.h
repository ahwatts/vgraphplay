// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#ifndef _VGRAPHPLAY_VGRAPHPLAY_GFX_PRESENTATION_H_
#define _VGRAPHPLAY_VGRAPHPLAY_GFX_PRESENTATION_H_

#include <vector>

#include "../vulkan.h"

namespace vgraphplay {
    namespace gfx {
        class CommandQueues;
        class Device;
        
        class Presentation {
        public:
            Presentation(Device *parent);
            ~Presentation();

            bool initialize();
            void dispose();

            VkSurfaceFormatKHR chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &formats);
            VkPresentModeKHR choosePresentMode(const std::vector<VkPresentModeKHR> &modes);
            VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &surf_caps);

            inline VkSwapchainKHR& swapchain() { return m_swapchain; }
            inline VkExtent2D& swapchainExtent() { return m_extent; }
            inline VkSurfaceFormatKHR& swapchainFormat() { return m_format; }
            inline std::vector<VkImage>& swapchainImages() { return m_swapchain_images; }
            inline std::vector<VkImageView>& swapchainImageViews() { return m_swapchain_image_views; }

        protected:
            Device *m_parent;
            VkSwapchainKHR m_swapchain;
            std::vector<VkImage> m_swapchain_images;
            std::vector<VkImageView> m_swapchain_image_views;
            VkSurfaceFormatKHR m_format;
            VkExtent2D m_extent;
        };
    }
}

#endif
