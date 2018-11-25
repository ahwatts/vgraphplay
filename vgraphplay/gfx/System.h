// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#ifndef _VGRAPHPLAY_VGRAPHPLAY_GFX_SYSTEM_H_
#define _VGRAPHPLAY_VGRAPHPLAY_GFX_SYSTEM_H_

#include <array>
#include <vector>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "../vulkan.h"

#include "Resource.h"

namespace vgraphplay {
    namespace gfx {
        struct Vertex {
            glm::vec2 pos;
            glm::vec3 color;

            static VkVertexInputBindingDescription bindingDescription();
            static std::array<VkVertexInputAttributeDescription, 2> attributeDescription();
        };

        struct Transormations {
            glm::mat4x4 model;
            glm::mat4x4 view;
            glm::mat4x4 projection;
        };

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
            void setFramebufferResized();

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
            void recreateSwapchain();

            bool initRenderPass();
            void cleanupRenderPass();

            bool initShaderModules();
            void cleanupShaderModules();
            VkShaderModule createShaderModule(const Resource &rsrc);

            bool initPipelineLayout();
            void cleanupPipelineLayout();

            bool initDescriptorSetLayout();
            void cleanupDescriptorSetLayout();

            bool initPipeline();
            void cleanupPipeline();

            bool initSwapchainFramebuffers();
            void cleanupSwapchainFramebuffers();

            bool initSemaphores();
            void cleanupSemaphores();

            bool initCommandPool();
            void cleanupCommandPool();

            bool initVertexBuffer();
            void cleanupVertexBuffer();

            bool initIndexBuffer();
            void cleanupIndexBuffer();

            bool initUniformBuffers();
            void cleanupUniformBuffers();
            void updateUniformBuffer(uint32_t current_image);
            bool initDescriptorPool();
            void cleanupDescriptorPool();
            bool initDescriptorSets();
            void cleanupDescriptorSets();

            bool initCommandBuffers();
            void cleanupCommandBuffers();
            bool recordCommandBuffers();

            bool createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags mem_props, VkBuffer &buffer, VkDeviceMemory &memory);
            uint32_t chooseMemoryTypeIndex(uint32_t type_filter, VkMemoryPropertyFlags mem_props);
            bool copyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size);

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

            // Draw data.
            VkBuffer m_vertex_buffer, m_index_buffer;
            std::vector<VkBuffer> m_uniform_buffers;
            VkDeviceMemory m_vertex_buffer_memory, m_index_buffer_memory;
            std::vector<VkDeviceMemory> m_uniform_buffers_memory;

            // Presentation-related structures.
            VkSurfaceKHR m_surface;
            VkSwapchainKHR m_swapchain;
            std::vector<VkImage> m_swapchain_images;
            std::vector<VkImageView> m_swapchain_image_views;
            VkSurfaceFormatKHR m_swapchain_format;
            VkExtent2D m_swapchain_extent;
            bool m_framebuffer_resized;

            // Pipeline-related structures.
            VkShaderModule m_vertex_shader_module, m_fragment_shader_module;
            VkPipelineLayout m_pipeline_layout;
            VkDescriptorSetLayout m_descriptor_set_layout;
            VkDescriptorPool m_descriptor_pool;
            std::vector<VkDescriptorSet> m_descriptor_sets;
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
