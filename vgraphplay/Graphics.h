// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#ifndef _VGRAPHPLAY_VGRAPHPLAY_GRAPHICS_H_
#define _VGRAPHPLAY_VGRAPHPLAY_GRAPHICS_H_

#include <vector>

#include "vulkan.h"

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>

namespace vgraphplay {
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

    struct BufferInfo {
        VkBuffer buffer;
        VkDeviceMemory memory;
    };

    struct ShaderInfo {
        std::vector<char> bytecode;
        VkShaderModule module;
    };

    struct Light {
        Light()
            : enabled{false},
              position{0.0, 0.0, 0.0},
              color{1.0, 1.0, 1.0, 1.0},
              specular_exp{0}
        {}

        bool enabled;
        glm::vec3 position;
        glm::vec4 color;
        unsigned int specular_exp;
    };

    struct Uniforms {
        Uniforms()
            : model{1},
              model_inv_trans_3{1},
              view{1},
              view_inv{1},
              projection{1}
        {}

        glm::mat4x4 model;
        glm::mat3x3 model_inv_trans_3;
        glm::mat4x4 view;
        glm::mat4x4 view_inv;
        glm::mat4x4 projection;
        Light lights[10];
    };

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
        class System;
        class Device;
        class Surface;

        class Device {
        public:
            Device(System *parent);
            ~Device();

            bool initialize();

        protected:
            System *m_parent;
            VkDevice m_device;
            VkPhysicalDevice m_physical_device;
        };

        class System {
        public:
            System();
            ~System();

            bool initialize(GLFWwindow *window);

        protected:
            GLFWwindow *m_window;
            VkInstance m_instance;
            Device m_device;
        };
    }
}

#endif
