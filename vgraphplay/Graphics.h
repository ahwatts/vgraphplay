// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#ifndef _VGRAPHPLAY_VGRAPHPLAY_GRAPHICS_H_
#define _VGRAPHPLAY_VGRAPHPLAY_GRAPHICS_H_

#include <vector>

#include "vulkan.h"

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>

#include "AssetFinder.h"

namespace vgraphplay {
    // struct CommandQueueInfo {
    //     uint32_t family;
    //     VkCommandPool pool;
    //     VkCommandBuffer buffer;
    // };

    // struct SwapchainImageInfo {
    //     VkImage image;
    //     VkImageView view;
    // };

    // struct SwapchainInfo {
    //     VkSwapchainKHR swapchain;
    //     std::vector<SwapchainImageInfo> images;
    // };

    // struct ImageInfo {
    //     VkImage image;
    //     VkImageView view;
    //     VkDeviceMemory memory;
    // };

    // struct BufferInfo {
    //     VkBuffer buffer;
    //     VkDeviceMemory memory;
    // };

    // struct ShaderInfo {
    //     std::vector<char> bytecode;
    //     VkShaderModule module;
    // };

    // struct Light {
    //     Light()
    //         : enabled{false},
    //           position{0.0, 0.0, 0.0},
    //           color{1.0, 1.0, 1.0, 1.0},
    //           specular_exp{0}
    //     {}

    //     bool enabled;
    //     glm::vec3 position;
    //     glm::vec4 color;
    //     unsigned int specular_exp;
    // };

    // struct Uniforms {
    //     Uniforms()
    //         : model{1},
    //           model_inv_trans_3{1},
    //           view{1},
    //           view_inv{1},
    //           projection{1}
    //     {}

    //     glm::mat4x4 model;
    //     glm::mat3x3 model_inv_trans_3;
    //     glm::mat4x4 view;
    //     glm::mat4x4 view_inv;
    //     glm::mat4x4 projection;
    //     Light lights[10];
    // };

    // class Graphics {
    // public:
    //     GLFWwindow *window;

    //     VkInstance instance;
    //     VkPhysicalDevice physical_device;
    //     VkDevice device;
    //     VkSurfaceKHR surface;

    //     CommandQueueInfo queue;
    //     SwapchainInfo swapchain;
    //     ImageInfo depth_buffer;

    //     Uniforms uniforms;
    //     BufferInfo uniform_buffer;

    //     ShaderInfo unlit_vertex, unlit_fragment;

    //     // Create the object; doesn't set up the Vulkan stuff.
    //     Graphics();
    //     ~Graphics();

    //     // Set up all the Vulkan stuff.
    //     bool initialize(GLFWwindow *window);
    //     void shutDown();

    // protected:
    //     // The sub-parts of initialize.
    //     bool initInstance();
    //     bool initDevice();
    //     bool initCommandQueue();
    //     bool initSwapchain();
    //     bool initDepthBuffer();
    //     bool initUniformBuffer();
    //     bool initPipelineLayout();
    // };

    namespace gfx {
        class Device;
        class Pipeline;
        class Presentation;
        class CommandQueues;
        class System;

        class CommandQueues {
        public:
            CommandQueues(Device *parent);
            ~CommandQueues();

            bool initialize(uint32_t gq_fam, uint32_t pq_fam);
            void dispose();

            VkDevice& device();

            inline uint32_t graphicsQueueFamily() const { return m_graphics_queue_family; }
            inline uint32_t presentQueueFamily() const { return m_present_queue_family; }

        protected:
            Device *m_parent;
            uint32_t m_graphics_queue_family;
            VkQueue m_graphics_queue;
            uint32_t m_present_queue_family;
            VkQueue m_present_queue;
        };

        class Pipeline {
        public:
            Pipeline(Device *parent);
            ~Pipeline();

            bool initialize();
            void dispose();

            VkDevice& device();

            VkShaderModule createShaderModule(const char *filename);

            AssetFinder& assetFinder();
            Presentation& presentation();

        protected:
            Device *m_parent;
            VkShaderModule m_vertex_shader_module, m_fragment_shader_module;
        };

        class Presentation {
        public:
            Presentation(Device *parent);
            ~Presentation();

            bool initialize();
            void dispose();

            VkSurfaceFormatKHR chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &formats);
            VkPresentModeKHR choosePresentMode(const std::vector<VkPresentModeKHR> &modes);
            VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &surf_caps);

            GLFWwindow* window();
            VkDevice& device();
            VkPhysicalDevice& physicalDevice();
            VkSurfaceKHR& surface();
            VkExtent2D& swapchainExtent();

            CommandQueues& queues();

        protected:
            Device *m_parent;
            VkSwapchainKHR m_swapchain;
            std::vector<VkImage> m_swapchain_images;
            std::vector<VkImageView> m_swapchain_image_views;
            VkSurfaceFormatKHR m_format;
            VkExtent2D m_extent;
        };

        class Device {
        public:
            Device(System *parent);
            ~Device();

            bool initialize();
            void dispose();

            VkPhysicalDevice choosePhysicalDevice(const std::vector<VkPhysicalDevice> &devices);
            uint32_t chooseGraphicsQueueFamily(VkPhysicalDevice &device, const std::vector<VkQueueFamilyProperties> &families);
            uint32_t choosePresentQueueFamily(VkPhysicalDevice &device, const std::vector<VkQueueFamilyProperties> &families, VkSurfaceKHR &surf);

            GLFWwindow* window();
            VkDevice& device();
            VkInstance& instance();
            VkPhysicalDevice& physicalDevice();
            VkSurfaceKHR& surface();

            CommandQueues& queues();
            AssetFinder& assetFinder();
            Presentation& presentation();

        protected:
            System *m_parent;
            VkDevice m_device;
            VkPhysicalDevice m_physical_device;
            CommandQueues m_queues;
            Presentation m_present;
            Pipeline m_pipeline;
        };

        class System {
        public:
            System(GLFWwindow *window, const AssetFinder &asset_finder);
            ~System();

            bool initialize();
            void dispose();

            inline GLFWwindow* window() { return m_window; }
            inline VkDevice& device() { return m_device.device(); }
            inline VkInstance& instance() { return m_instance; }
            inline VkPhysicalDevice& physicalDevice() { return m_device.physicalDevice(); }
            inline VkSurfaceKHR& surface() { return m_surface; }

            inline CommandQueues& queues() { return m_device.queues(); }
            inline AssetFinder& assetFinder() { return m_asset_finder; }

        protected:
            GLFWwindow *m_window;
            AssetFinder m_asset_finder;
            VkInstance m_instance;
            VkSurfaceKHR m_surface;
            Device m_device;
        };
    }
}

#endif
