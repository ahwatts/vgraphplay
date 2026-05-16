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
            glm::vec3 pos;
            glm::vec3 color;
            glm::vec2 tex;

            static VkVertexInputBindingDescription bindingDescription();
            static std::array<VkVertexInputAttributeDescription, 3> attributeDescription();
        };

        struct Transormations {
            glm::mat4x4 model;
            glm::mat4x4 view;
            glm::mat4x4 projection;
        };

        // struct ChosenDeviceInfo {
        //     vk::raii::PhysicalDevice dev;
        //     uint32_t graphics_queue_family;
        //     uint32_t present_queue_family;
        // };

        class System {
        public:
            System(GLFWwindow *window, bool debug);
            ~System();

            void drawFrame();
            // void setFramebufferResized();

        private:
            void initInstance();
            void initDebugMessenger();

            void initDevice();
            void initSurface();
            void initSwapchain();
            // void recreateSwapchain();
            void initPipeline();
            void initCommandPool();
            void initCommandBuffer();
            void initSynchronizationObjects();

            void recordRenderInCommandBuffer(uint32_t image_index);

            /* bool initDescriptorSetLayout();
            void cleanupDescriptorSetLayout();

            bool initTextureImage();
            bool initTextureImageView();
            bool initTextureSampler();
            void cleanupTextureImage();
            void cleanupTextureImageView();
            void cleanupTextureSampler();

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
            vk::raii::CommandBuffer m_command_buffer;

            // // Draw data.
            // VkBuffer m_vertex_buffer, m_index_buffer;
            // std::vector<VkBuffer> m_uniform_buffers;
            // VkDeviceMemory m_vertex_buffer_memory, m_index_buffer_memory;
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
            // bool m_framebuffer_resized;
            // VkImage m_depth_image;
            // VkDeviceMemory m_depth_image_memory;
            // VkImageView m_depth_image_view;

            // // Pipeline-related structures.
            vk::raii::PipelineLayout m_pipeline_layout;
            vk::raii::Pipeline m_pipeline;
            // VkShaderModule m_vertex_shader_module, m_fragment_shader_module;
            // VkDescriptorSetLayout m_descriptor_set_layout;
            // VkDescriptorPool m_descriptor_pool;
            // std::vector<VkDescriptorSet> m_descriptor_sets;
            
            // Synchronization objects.
            vk::raii::Semaphore m_present_complete_semaphore;
            std::vector<vk::raii::Semaphore> m_render_finished_semaphores;
            vk::raii::Fence m_draw_fence;
        };
    }
}

#endif
