// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#ifndef _VGRAPHPLAY_VGRAPHPLAY_GFX_PIPELINE_H_
#define _VGRAPHPLAY_VGRAPHPLAY_GFX_PIPELINE_H_

#include <vector>

#include "../vulkan.h"

#include "Resource.h"

namespace vgraphplay {
    namespace gfx {
        class Presentation;
        class Device;

        class Pipeline {
        public:
            Pipeline(Device *parent);
            ~Pipeline();

            bool initialize();
            void dispose();

            VkShaderModule createShaderModule(const Resource &rsrc);
            
            inline VkShaderModule& vertexShaderModule() { return m_vertex_shader_module; }
            inline VkShaderModule& fragmentShaderModule() { return m_fragment_shader_module; }
            inline VkPipelineLayout& pipelineLayout() { return m_pipeline_layout; }
            inline VkRenderPass& renderPass() { return m_render_pass; }
            inline VkPipeline& pipeline() { return m_pipeline; }
            inline std::vector<VkFramebuffer>& swapchainFramebuffers() { return m_swapchain_framebuffers; }

        protected:
            Device *m_parent;
            VkShaderModule m_vertex_shader_module, m_fragment_shader_module;
            VkPipelineLayout m_pipeline_layout;
            VkRenderPass m_render_pass;
            VkPipeline m_pipeline;
            std::vector<VkFramebuffer> m_swapchain_framebuffers;
        };
    }
}

#endif
