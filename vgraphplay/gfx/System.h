// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#ifndef _VGRAPHPLAY_VGRAPHPLAY_GFX_SYSTEM_H_
#define _VGRAPHPLAY_VGRAPHPLAY_GFX_SYSTEM_H_

#include "../vulkan.h"

#include "Device.h"

namespace vgraphplay {
    namespace gfx {
        struct ChosenDeviceInfo {
            VkPhysicalDevice dev;
            uint32_t graphics_queue_family;
            uint32_t present_queue_family;
        };

        class System {
        public:
            System(GLFWwindow *window);
            ~System();

            bool initialize(bool debug);
            void dispose();

            void drawFrame();

        private:
            bool initInstance(bool debug);
            void cleanupInstance();

            bool initDebugCallback();
            void cleanupDebugCallback();

            bool initDevice();
            void cleanupDevice();
            ChosenDeviceInfo choosePhysicalDevice(std::vector<VkPhysicalDevice> &devices, VkSurfaceKHR &surface);

            bool initSurface();
            void cleanupSurface();

            bool initSwapchain();
            void cleanupSwapchain();
            VkSurfaceFormatKHR chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &formats);
            VkPresentModeKHR choosePresentMode(const std::vector<VkPresentModeKHR> &modes);
            VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &surf_caps);

            GLFWwindow *m_window;

            // Instance, device, and debug callback.
            VkInstance m_instance;
            VkDebugReportCallbackEXT m_debug_callback;
            VkDevice m_device;
            VkPhysicalDevice m_physical_device;

            // Command queues / buffers / pool.
            uint32_t m_graphics_queue_family;
            uint32_t m_present_queue_family;
            VkQueue m_graphics_queue;
            VkQueue m_present_queue;
            VkCommandPool m_command_pool;
            std::vector<VkCommandBuffer> m_command_buffers;

            // Presentation-related structures.
            VkSurfaceKHR m_surface;
            VkSwapchainKHR m_swapchain;
            std::vector<VkImage> m_swapchain_images;
            std::vector<VkImageView> m_swapchain_image_views;
            VkSurfaceFormatKHR m_swapchain_format;
            VkExtent2D m_swapchain_extent;

            // Pipeline-related structures.
            VkShaderModule m_vertex_shader_module, m_fragment_shader_module;
            VkPipelineLayout m_pipeline_layout;
            VkRenderPass m_render_pass;
            VkPipeline m_pipeline;
            std::vector<VkFramebuffer> m_swapchain_framebuffers;

            // Semaphores.
            VkSemaphore m_image_available_semaphore;
            VkSemaphore m_render_finished_semaphore;
        };
    }
}

#endif
