// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#ifndef _VGRAPHPLAY_VGRAPHPLAY_GFX_SYSTEM_H_
#define _VGRAPHPLAY_VGRAPHPLAY_GFX_SYSTEM_H_

#include <array>
#include <vector>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <GLFW/glfw3.h>

#include "../vulkan.h"

#include "Resource.h"

namespace vgraphplay {
    namespace gfx {
        struct Vertex {
            glm::vec2 pos;
            glm::vec3 color;
            // glm::vec2 tex;

            static vk::VertexInputBindingDescription bindingDescription();
            static std::array<vk::VertexInputAttributeDescription, 2> attributeDescription();
        };

        // struct Transormations {
        //     glm::mat4x4 model;
        //     glm::mat4x4 view;
        //     glm::mat4x4 projection;
        // };

        class System {
        public:
            static constexpr int MAX_FRAMES_IN_FLIGHT = 2;
            
            System(GLFWwindow *window, bool debug);
            ~System();

            void drawFrame();
            void setFramebufferResized();

        private:
            void initInstance();
            void initDebugMessenger();

            void initDevice();
            void initSurface();
            void initSwapchain();
            void recreateSwapchain();

            void initPipeline();
            void initVertexBuffer();

            void initCommandPool();
            void initCommandBuffer();
            void recordRenderInCommandBuffer(uint32_t image_index);
            
            void initSynchronizationObjects();

            /* bool initDescriptorSetLayout();
            void cleanupDescriptorSetLayout();

            bool initTextureImage();
            bool initTextureImageView();
            bool initTextureSampler();
            void cleanupTextureImage();
            void cleanupTextureImageView();
            void cleanupTextureSampler();

            bool initIndexBuffer();
            void cleanupIndexBuffer();

            bool initUniformBuffers();
            void cleanupUniformBuffers();
            void updateUniformBuffer(uint32_t current_image);
            bool initDescriptorPool();
            void cleanupDescriptorPool();
            bool initDescriptorSets();
            void cleanupDescriptorSets();

            bool initDepthResources();
            void cleanupDepthResources();
            VkFormat chooseDepthFormat();

            bool createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags mem_props, VkBuffer &buffer, VkDeviceMemory &memory);
            bool createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage &image, VkDeviceMemory &memory);
            VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspect_flags);

            bool copyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size);
            bool copyBufferToImage(VkBuffer src, VkImage dst, uint32_t width, uint32_t height);

            VkCommandBuffer beginOneTimeCommands();
            bool endOneTimeCommands(VkCommandBuffer commands); */

            bool m_debug;
            GLFWwindow *m_window;
            uint32_t m_frame_index;

            // Instance, device, and debug callback.
            vk::raii::Context m_context;
            vk::raii::Instance m_instance;
            vk::raii::DebugUtilsMessengerEXT m_debug_messenger;
            vk::raii::PhysicalDevice m_physical_device;
            vk::raii::Device m_device;

            // Command queues / buffers / pool.
            uint32_t m_command_queue_family_index;
            vk::raii::Queue m_command_queue;
            vk::raii::CommandPool m_command_pool;
            std::vector<vk::raii::CommandBuffer> m_command_buffers;

            // Draw data.
            vk::raii::Buffer m_vertex_buffer /*, m_index_buffer */;
            // std::vector<VkBuffer> m_uniform_buffers;
            vk::raii::DeviceMemory m_vertex_buffer_memory /*, m_index_buffer_memory */;
            // std::vector<VkDeviceMemory> m_uniform_buffers_memory;
            // VkImage m_texture_image;
            // VkDeviceMemory m_texture_image_memory;
            // VkImageView m_texture_image_view;
            // VkSampler m_texture_sampler;

            // Presentation-related structures.
            vk::raii::SurfaceKHR m_surface;
            vk::SurfaceFormatKHR m_swapchain_format;
            vk::Extent2D m_swapchain_extent;
            uint32_t m_swapchain_image_count;
            vk::raii::SwapchainKHR m_swapchain;
            std::vector<vk::Image> m_swapchain_images;
            std::vector<vk::raii::ImageView> m_swapchain_image_views;
            bool m_framebuffer_resized;
            // VkImage m_depth_image;
            // VkDeviceMemory m_depth_image_memory;
            // VkImageView m_depth_image_view;

            // Pipeline-related structures.
            vk::raii::PipelineLayout m_pipeline_layout;
            vk::raii::Pipeline m_pipeline;
            // VkDescriptorSetLayout m_descriptor_set_layout;
            // VkDescriptorPool m_descriptor_pool;
            // std::vector<VkDescriptorSet> m_descriptor_sets;
            
            // Synchronization objects.
            std::vector<vk::raii::Semaphore> m_present_complete_semaphores;
            std::vector<vk::raii::Semaphore> m_render_finished_semaphores;
            std::vector<vk::raii::Fence> m_draw_fences;
        };
    }
}

#endif
