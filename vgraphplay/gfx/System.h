// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#ifndef _VGRAPHPLAY_VGRAPHPLAY_GFX_SYSTEM_H_
#define _VGRAPHPLAY_VGRAPHPLAY_GFX_SYSTEM_H_

#include <array>
#include <optional>
#include <utility>
#include <vector>

#include "../glm.h"
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

        struct Transformations {
            glm::mat4x4 model;
            glm::mat4x4 view;
            glm::mat4x4 projection;
        };

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

            void initDescriptorSetLayout();
            void initDescriptorPool();
            void initDescriptorSets();
            void initPipeline();
            
            void initVertexBuffer();
            void initIndexBuffer();
            void initUniformBuffers();

            void initTextureImage();
            void initTextureImageView();
            void initTextureSampler();

            void initCommandPool();
            void initCommandBuffer();
            
            void initSynchronizationObjects();

            void updateUniformBuffer(uint32_t image_index);
            void recordRenderInCommandBuffer(uint32_t image_index);

            std::pair<vk::raii::Buffer, vk::raii::DeviceMemory> createBuffer(
                vk::DeviceSize size, 
                vk::BufferUsageFlags usage, 
                vk::MemoryPropertyFlags properties,
                std::optional<const char *> name = std::nullopt
            );
            void copyBuffer(vk::raii::Buffer &src, vk::raii::Buffer &dst, vk::DeviceSize size);

            std::pair<vk::raii::Image, vk::raii::DeviceMemory> createImage(
                uint32_t width, uint32_t height,
                vk::Format format,
                vk::ImageTiling tiling,
                vk::ImageUsageFlags usage,
                vk::MemoryPropertyFlags properties
            );
            vk::raii::ImageView createImageView(
                const vk::Image image, 
                vk::Format format, 
                vk::ImageAspectFlags aspect_mask
            );
            void transitionImageLayout(
                vk::raii::CommandBuffer &command_buffer,
                const vk::Image &image,
                vk::ImageLayout old_layout,
                vk::ImageLayout new_layout,
                vk::AccessFlags2 src_access_mask,
                vk::AccessFlags2 dst_access_mask, 
                vk::PipelineStageFlags2 src_stage_mask,
                vk::PipelineStageFlags2 dst_stage_mask
            );
            void copyBufferToImage(
                vk::raii::CommandBuffer &command_buffer,
                const vk::Buffer buffer,
                const vk::Image image,
                uint32_t width,
                uint32_t height
            );

            vk::raii::CommandBuffer beginOneTimeCommands();
            void endOneTimeCommands(vk::raii::CommandBuffer &&commands);
            
            /* bool initDepthResources();
            void cleanupDepthResources();
            VkFormat chooseDepthFormat(); */

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
            vk::raii::Buffer m_vertex_buffer, m_index_buffer;
            std::vector<vk::raii::Buffer> m_uniform_buffers;
            vk::raii::DeviceMemory m_vertex_buffer_memory, m_index_buffer_memory;
            std::vector<vk::raii::DeviceMemory> m_uniform_buffers_memory;
            std::vector<void *> m_uniform_buffers_mapped;
            vk::raii::Image m_texture_image;
            vk::raii::DeviceMemory m_texture_image_memory;
            vk::raii::ImageView m_texture_image_view;
            vk::raii::Sampler m_texture_sampler;

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
            vk::raii::DescriptorSetLayout m_descriptor_set_layout;
            vk::raii::DescriptorPool m_descriptor_pool;
            std::vector<vk::raii::DescriptorSet> m_descriptor_sets;
            
            // Synchronization objects.
            std::vector<vk::raii::Semaphore> m_present_complete_semaphores;
            std::vector<vk::raii::Semaphore> m_render_finished_semaphores;
            std::vector<vk::raii::Fence> m_draw_fences;
        };
    }
}

#endif
