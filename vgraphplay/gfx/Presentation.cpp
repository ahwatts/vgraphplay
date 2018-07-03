// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#include <boost/log/trivial.hpp>

#include "Device.h"
#include "Commands.h"
#include "Presentation.h"
#include "VulkanOutput.h"
#include "System.h"

vgraphplay::gfx::Presentation::Presentation(Device *parent)
    : m_parent{parent},
      m_swapchain{VK_NULL_HANDLE},
      m_swapchain_images{},
      m_swapchain_image_views{},
      m_format{VK_FORMAT_UNDEFINED, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR},
      m_extent{0, 0}
{}

vgraphplay::gfx::Presentation::~Presentation() {
    dispose();
}

bool vgraphplay::gfx::Presentation::initialize() {
    if (m_swapchain == VK_NULL_HANDLE) {
        VkPhysicalDevice pdev = m_parent->physicalDevice();
        VkDevice dev = m_parent->device();
        VkSurfaceKHR surf = m_parent->parent().surface();
        CommandQueues &qs = m_parent->queues();

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
        swapchain_ci.queueFamilyIndexCount = (uint32_t)queue_families.size();
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

void vgraphplay::gfx::Presentation::dispose() {
    VkDevice &dev = m_parent->device();
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

VkSurfaceFormatKHR vgraphplay::gfx::Presentation::chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &formats) {
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

VkPresentModeKHR vgraphplay::gfx::Presentation::choosePresentMode(const std::vector<VkPresentModeKHR> &modes) {
    // Prefer mailbox over fifo, if it's available.
    for (const auto &mode: modes) {
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return mode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D vgraphplay::gfx::Presentation::chooseSwapExtent(const VkSurfaceCapabilitiesKHR &surf_caps) {
    if (surf_caps.currentExtent.width != UINT32_MAX) {
        return surf_caps.currentExtent;
    }

    GLFWwindow *wnd = m_parent->parent().window();
    int width, height;
    glfwGetWindowSize(wnd, &width, &height);

    VkExtent2D extent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
    extent.width  = std::max(surf_caps.minImageExtent.width,  std::min(surf_caps.maxImageExtent.width,  extent.width));
    extent.height = std::max(surf_caps.minImageExtent.height, std::min(surf_caps.maxImageExtent.height, extent.height));
    return extent;
}
