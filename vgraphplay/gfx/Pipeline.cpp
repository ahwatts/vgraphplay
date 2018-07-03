// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#include <boost/log/trivial.hpp>

#include "../vulkan.h"

#include "AssetFinder.h"
#include "Device.h"
#include "Pipeline.h"
#include "Presentation.h"
#include "System.h"

vgraphplay::gfx::Pipeline::Pipeline(Device *parent)
    : m_parent{parent},
      m_vertex_shader_module{VK_NULL_HANDLE},
      m_fragment_shader_module{VK_NULL_HANDLE},
      m_pipeline_layout{VK_NULL_HANDLE},
      m_render_pass{VK_NULL_HANDLE},
      m_pipeline{VK_NULL_HANDLE},
      m_swapchain_framebuffers{}
{}

vgraphplay::gfx::Pipeline::~Pipeline() {
    dispose();
}

bool vgraphplay::gfx::Pipeline::initialize() {
    Presentation &presentation = m_parent->presentation();
    VkDevice &dev = m_parent->device();

    m_vertex_shader_module = createShaderModule("unlit.vert.spv");
    m_fragment_shader_module = createShaderModule("unlit.frag.spv");
    if (m_vertex_shader_module == VK_NULL_HANDLE || m_fragment_shader_module == VK_NULL_HANDLE) {
        BOOST_LOG_TRIVIAL(trace) << "Unable to create all of the shader modules.";
        return false;
    }

    VkPipelineShaderStageCreateInfo ss_ci[2];
    ss_ci[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    ss_ci[0].pNext = nullptr;
    ss_ci[0].flags = 0;
    ss_ci[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    ss_ci[0].module = m_vertex_shader_module;
    ss_ci[0].pName = "main";
    ss_ci[0].pSpecializationInfo = nullptr;

    ss_ci[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    ss_ci[1].pNext = nullptr;
    ss_ci[1].flags = 0;
    ss_ci[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    ss_ci[1].module = m_fragment_shader_module;
    ss_ci[1].pName = "main";
    ss_ci[1].pSpecializationInfo = nullptr;

    VkPipelineVertexInputStateCreateInfo vert_in_ci;
    vert_in_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vert_in_ci.pNext = nullptr;
    vert_in_ci.flags = 0;
    vert_in_ci.vertexBindingDescriptionCount = 0;
    vert_in_ci.pVertexBindingDescriptions = nullptr;
    vert_in_ci.vertexAttributeDescriptionCount = 0;
    vert_in_ci.pVertexAttributeDescriptions = nullptr;

    VkPipelineInputAssemblyStateCreateInfo input_asm_ci;
    input_asm_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_asm_ci.pNext = nullptr;
    input_asm_ci.flags = 0;
    input_asm_ci.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_asm_ci.primitiveRestartEnable = VK_FALSE;

    VkExtent2D &extent = presentation.swapchainExtent();
    VkViewport viewport;
    viewport.x = 0.0;
    viewport.y = 0.0;
    viewport.width = static_cast<float>(extent.width);
    viewport.height = static_cast<float>(extent.height);
    viewport.minDepth = 0.0;
    viewport.maxDepth = 1.0;

    VkRect2D scissor;
    scissor.offset = { 0, 0 };
    scissor.extent = extent;

    VkPipelineViewportStateCreateInfo vp_ci;
    vp_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vp_ci.pNext = nullptr;
    vp_ci.flags = 0;
    vp_ci.viewportCount = 1;
    vp_ci.pViewports = &viewport;
    vp_ci.scissorCount = 1;
    vp_ci.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo raster_ci;
    raster_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    raster_ci.pNext = nullptr;
    raster_ci.flags = 0;
    raster_ci.depthClampEnable = VK_FALSE;
    raster_ci.rasterizerDiscardEnable = VK_FALSE;
    raster_ci.polygonMode = VK_POLYGON_MODE_FILL;
    raster_ci.cullMode = VK_CULL_MODE_BACK_BIT;
    raster_ci.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    raster_ci.depthBiasEnable = VK_FALSE;
    raster_ci.depthBiasConstantFactor = 0.0;
    raster_ci.depthBiasClamp = 0.0;
    raster_ci.depthBiasSlopeFactor = 0.0;
    raster_ci.lineWidth = 1.0;

    VkPipelineMultisampleStateCreateInfo msamp_ci;
    msamp_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    msamp_ci.pNext = nullptr;
    msamp_ci.flags = 0;
    msamp_ci.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    msamp_ci.sampleShadingEnable = VK_FALSE;
    msamp_ci.minSampleShading = 1.0;
    msamp_ci.pSampleMask = nullptr;
    msamp_ci.alphaToCoverageEnable = VK_FALSE;
    msamp_ci.alphaToOneEnable = VK_FALSE;

    // VkPipelineDepthStencilStateCreateInfo depth_ci;
    // depth_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    // depth_ci.pNext = nullptr;
    // depth_ci.flags = 0;

    VkPipelineColorBlendAttachmentState blender;
    blender.blendEnable = VK_TRUE;
    blender.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    blender.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blender.colorBlendOp = VK_BLEND_OP_ADD;
    blender.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    blender.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    blender.alphaBlendOp = VK_BLEND_OP_ADD;
    blender.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo blend_ci;
    blend_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blend_ci.pNext = nullptr;
    blend_ci.flags = 0;
    blend_ci.logicOpEnable = VK_FALSE;
    blend_ci.logicOp = VK_LOGIC_OP_COPY;
    blend_ci.attachmentCount = 1;
    blend_ci.pAttachments = &blender;
    blend_ci.blendConstants[0] = 0.0;
    blend_ci.blendConstants[1] = 0.0;
    blend_ci.blendConstants[2] = 0.0;
    blend_ci.blendConstants[3] = 0.0;

    // VkPipelineDynamicStateCreateInfo dyn_state_ci;
    // dyn_state_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    // dyn_state_ci.pNext = nullptr;
    // dyn_state_ci.flags = 0;

    VkPipelineLayoutCreateInfo pl_layout_ci;
    pl_layout_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pl_layout_ci.pNext = nullptr;
    pl_layout_ci.flags = 0;
    pl_layout_ci.setLayoutCount = 0;
    pl_layout_ci.pSetLayouts = nullptr;
    pl_layout_ci.pushConstantRangeCount = 0;
    pl_layout_ci.pPushConstantRanges = nullptr;

    VkResult rslt = vkCreatePipelineLayout(dev, &pl_layout_ci, nullptr, &m_pipeline_layout);
    if (rslt == VK_SUCCESS) {
        BOOST_LOG_TRIVIAL(trace) << "Created pipeline layout: " << m_pipeline_layout;
    } else {
        BOOST_LOG_TRIVIAL(error) << "Error creating pipeline layout: " << rslt;
        return false;
    }

    VkAttachmentDescription color_att;
    color_att.flags = 0;
    color_att.format = presentation.swapchainFormat().format;
    color_att.samples = VK_SAMPLE_COUNT_1_BIT;
    color_att.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_att.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_att.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_att.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_att.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_att.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference color_ref;
    color_ref.attachment = 0;
    color_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass;
    subpass.flags = 0;
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.inputAttachmentCount = 0;
    subpass.pInputAttachments = nullptr;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_ref;
    subpass.pResolveAttachments = nullptr;
    subpass.pDepthStencilAttachment = nullptr;
    subpass.preserveAttachmentCount = 0;
    subpass.pPreserveAttachments = nullptr;

    VkRenderPassCreateInfo rp_ci;
    rp_ci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rp_ci.pNext = nullptr;
    rp_ci.flags = 0;
    rp_ci.attachmentCount = 1;
    rp_ci.pAttachments = &color_att;
    rp_ci.subpassCount = 1;
    rp_ci.pSubpasses = &subpass;

    VkSubpassDependency sd;
    sd.dependencyFlags = 0;
    sd.srcSubpass = VK_SUBPASS_EXTERNAL;
    sd.dstSubpass = 0;
    sd.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    sd.srcAccessMask = 0;
    sd.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    sd.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    rp_ci.dependencyCount = 1;
    rp_ci.pDependencies = &sd;

    rslt = vkCreateRenderPass(dev, &rp_ci, nullptr, &m_render_pass);
    if (rslt == VK_SUCCESS) {
        BOOST_LOG_TRIVIAL(trace) << "Created render pass: " << m_render_pass;
    } else {
        BOOST_LOG_TRIVIAL(error) << "Error creating render pass: " << rslt;
        return false;
    }

    VkGraphicsPipelineCreateInfo pipeline_ci;
    pipeline_ci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_ci.pNext = nullptr;
    pipeline_ci.flags = 0;
    pipeline_ci.stageCount = 2;
    pipeline_ci.pStages = ss_ci;
    pipeline_ci.pVertexInputState = &vert_in_ci;
    pipeline_ci.pInputAssemblyState = &input_asm_ci;
    pipeline_ci.pTessellationState = nullptr;
    pipeline_ci.pViewportState = &vp_ci;
    pipeline_ci.pRasterizationState = &raster_ci;
    pipeline_ci.pMultisampleState = &msamp_ci;
    pipeline_ci.pDepthStencilState = nullptr;
    pipeline_ci.pColorBlendState = &blend_ci;
    pipeline_ci.pDynamicState = nullptr;
    pipeline_ci.layout = m_pipeline_layout;
    pipeline_ci.renderPass = m_render_pass;
    pipeline_ci.subpass = 0;
    pipeline_ci.basePipelineHandle = VK_NULL_HANDLE;
    pipeline_ci.basePipelineIndex = -1;

    rslt = vkCreateGraphicsPipelines(dev, VK_NULL_HANDLE, 1, &pipeline_ci, nullptr, &m_pipeline);
    if (rslt == VK_SUCCESS) {
        BOOST_LOG_TRIVIAL(trace) << "Created graphics pipeline: " << m_pipeline;
    } else {
        BOOST_LOG_TRIVIAL(error) << "Error creating graphics pipeline: " << rslt;
        return false;
    }

    std::vector<VkImageView> &swapchain_image_views = presentation.swapchainImageViews();
    m_swapchain_framebuffers.resize(swapchain_image_views.size());
    for (unsigned int i = 0; i < swapchain_image_views.size(); ++i) {
        VkImageView attachments[] = { swapchain_image_views[i] };

        VkFramebufferCreateInfo fb_ci;
        fb_ci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fb_ci.pNext = nullptr;
        fb_ci.flags = 0;
        fb_ci.renderPass = m_render_pass;
        fb_ci.attachmentCount = 1;
        fb_ci.pAttachments = attachments;
        fb_ci.width = extent.width;
        fb_ci.height = extent.height;
        fb_ci.layers = 1;

        rslt = vkCreateFramebuffer(dev, &fb_ci, nullptr, &m_swapchain_framebuffers[i]);
        if (rslt == VK_SUCCESS) {
            BOOST_LOG_TRIVIAL(trace) << "Created swapchain framebuffer " << i << ": " << m_swapchain_framebuffers[i];
        } else {
            BOOST_LOG_TRIVIAL(error) << "Error creating swapchain framebuffer " << i << ": " << rslt;
            return false;
        }
    }

    return true;
}

void vgraphplay::gfx::Pipeline::dispose() {
    VkDevice &dev = m_parent->device();

    if (dev != VK_NULL_HANDLE && m_swapchain_framebuffers.size() > 0) {
        for (unsigned int i = 0; i < m_swapchain_framebuffers.size(); ++i) {
            BOOST_LOG_TRIVIAL(trace) << "Destroying swapchain framebuffer " << i << ": " << m_swapchain_framebuffers[i];
            vkDestroyFramebuffer(dev, m_swapchain_framebuffers[i], nullptr);
        }
    }

    if (dev != VK_NULL_HANDLE && m_pipeline != VK_NULL_HANDLE) {
        BOOST_LOG_TRIVIAL(trace) << "Destroying pipeline: " << m_pipeline;
        vkDestroyPipeline(dev, m_pipeline, nullptr);
    }

    if (dev != VK_NULL_HANDLE && m_render_pass != VK_NULL_HANDLE) {
        BOOST_LOG_TRIVIAL(trace) << "Destroying render pass: " << m_render_pass;
        vkDestroyRenderPass(dev, m_render_pass, nullptr);
        m_render_pass = VK_NULL_HANDLE;
    }

    if (dev != VK_NULL_HANDLE && m_pipeline_layout != VK_NULL_HANDLE) {
        BOOST_LOG_TRIVIAL(trace) << "Destroying pipeline layout: " << m_pipeline_layout;
        vkDestroyPipelineLayout(dev, m_pipeline_layout, nullptr);
        m_pipeline_layout = VK_NULL_HANDLE;
    }

    if (dev != VK_NULL_HANDLE && m_vertex_shader_module != VK_NULL_HANDLE) {
        BOOST_LOG_TRIVIAL(trace) << "Destroying vertex shader module: " << m_vertex_shader_module;
        vkDestroyShaderModule(dev, m_vertex_shader_module, nullptr);
        m_vertex_shader_module = VK_NULL_HANDLE;
    }

    if (dev != VK_NULL_HANDLE && m_fragment_shader_module != VK_NULL_HANDLE) {
        BOOST_LOG_TRIVIAL(trace) << "Destroying fragment shader module: " << m_fragment_shader_module;
        vkDestroyShaderModule(dev, m_fragment_shader_module, nullptr);
        m_fragment_shader_module = VK_NULL_HANDLE;
    }
}

VkShaderModule vgraphplay::gfx::Pipeline::createShaderModule(const char *filename) {
    System &system = m_parent->parent();
    AssetFinder &asset_finder = system.assetFinder();
    
    VkDevice &dev = m_parent->device();
    VkShaderModule rv = VK_NULL_HANDLE;

    VkShaderModuleCreateInfo sm_ci;
    sm_ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    sm_ci.pNext = nullptr;
    sm_ci.flags = 0;

    path bytecode_path = asset_finder.findShader(filename);
    if (!exists(bytecode_path)) {
        BOOST_LOG_TRIVIAL(error) << "Cannot find file " << bytecode_path;
        return rv;
    }

    std::ifstream bytecode_file(bytecode_path.string(), std::ios::ate | std::ios::binary);
    std::streampos bytecode_size = bytecode_file.tellg();
    std::vector<char> bytecode(static_cast<unsigned int>(bytecode_size));
    bytecode_file.seekg(0);
    bytecode_file.read(bytecode.data(), bytecode_size);
    bytecode_file.close();

    sm_ci.codeSize = static_cast<size_t>(bytecode_size);
    sm_ci.pCode = reinterpret_cast<uint32_t*>(bytecode.data());

    VkResult rslt = vkCreateShaderModule(dev, &sm_ci, nullptr, &rv);
    if (rslt == VK_SUCCESS) {
        BOOST_LOG_TRIVIAL(trace) << "Created shader module for " << filename << ": " << rv;
    } else {
        BOOST_LOG_TRIVIAL(trace) << "Failed to create shader module: " << rslt;
    }

    return rv;
}
