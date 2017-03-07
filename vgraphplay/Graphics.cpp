// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#include "Graphics.h"

#include <fstream>
#include <iostream>

#define BOOST_LOG_DYN_LINK
#include <boost/log/trivial.hpp>

#include "VulkanOutput.h"

namespace vgraphplay {
    namespace gfx {
        CommandQueues::CommandQueues(Device *parent)
            : m_parent{parent},
              m_graphics_queue_family{UINT32_MAX},
              m_graphics_queue{VK_NULL_HANDLE},
              m_present_queue_family{UINT32_MAX},
              m_present_queue{VK_NULL_HANDLE}
        {}

        CommandQueues::~CommandQueues() {
            dispose();
        }

        bool CommandQueues::initialize(uint32_t gq_fam, uint32_t pq_fam) {
            VkDevice &dev = device();

            m_graphics_queue_family = gq_fam;
            vkGetDeviceQueue(dev, m_graphics_queue_family, 0, &m_graphics_queue);
            BOOST_LOG_TRIVIAL(trace) << "Graphics queue: " << m_graphics_queue;

            m_present_queue_family = pq_fam;
            vkGetDeviceQueue(dev, m_present_queue_family, 0, &m_present_queue);
            BOOST_LOG_TRIVIAL(trace) << "Present queue: " << m_present_queue;

            return true;
        }

        void CommandQueues::dispose() {
            // Nothing to do here, yet.
        }

        VkDevice& CommandQueues::device() {
            return m_parent->device();
        }

        Device::Device(System *parent)
            : m_parent{parent},
              m_device{VK_NULL_HANDLE},
              m_physical_device{VK_NULL_HANDLE},
              m_queues{this},
              m_present{this},
              m_pipeline{this}
        {}

        Device::~Device() {
            dispose();
        }

        bool Device::initialize() {
            if (m_device != VK_NULL_HANDLE) {
                return true;
            }

            VkInstance &inst = instance();
            VkSurfaceKHR &surf = surface();

            if (inst == VK_NULL_HANDLE || surf == VK_NULL_HANDLE) {
                BOOST_LOG_TRIVIAL(error) << "Things have been initialized out of order. Cannot create device.";
                return false;
            }

            logPhysicalDevices(inst);

            uint32_t num_devices;
            vkEnumeratePhysicalDevices(inst, &num_devices, nullptr);
            std::vector<VkPhysicalDevice> devices(num_devices);
            vkEnumeratePhysicalDevices(inst, &num_devices, devices.data());
            m_physical_device = choosePhysicalDevice(devices);

            uint32_t num_queue_families = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(m_physical_device, &num_queue_families, nullptr);
            std::vector<VkQueueFamilyProperties> queue_families(num_queue_families);
            vkGetPhysicalDeviceQueueFamilyProperties(m_physical_device, &num_queue_families, queue_families.data());
            uint32_t graphics_queue_family = chooseGraphicsQueueFamily(m_physical_device, queue_families);
            uint32_t present_queue_family = choosePresentQueueFamily(m_physical_device, queue_families, surf);

            BOOST_LOG_TRIVIAL(trace) << "physical device = " << m_physical_device
                                     << " graphics queue family = " << graphics_queue_family
                                     << " present queue family = " << present_queue_family;

            float queue_priority = 1.0;
            std::vector<VkDeviceQueueCreateInfo> queue_cis;
            VkDeviceQueueCreateInfo queue_ci;
            queue_ci.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queue_ci.pNext = nullptr;
            queue_ci.flags = 0;
            queue_ci.queueFamilyIndex = graphics_queue_family;
            queue_ci.queueCount = 1;
            queue_ci.pQueuePriorities = &queue_priority;
            queue_cis.push_back(queue_ci);

            if (present_queue_family != graphics_queue_family) {
                VkDeviceQueueCreateInfo queue_ci;
                queue_ci.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                queue_ci.pNext = nullptr;
                queue_ci.flags = 0;
                queue_ci.queueFamilyIndex = present_queue_family;
                queue_ci.queueCount = 1;
                queue_ci.pQueuePriorities = &queue_priority;
                queue_cis.push_back(queue_ci);
            }

            std::vector<const char*> extension_names;
            extension_names.emplace_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

            VkDeviceCreateInfo device_ci;
            device_ci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
            device_ci.pNext = nullptr;
            device_ci.flags = 0;
            device_ci.queueCreateInfoCount = queue_cis.size();
            device_ci.pQueueCreateInfos = queue_cis.data();
            device_ci.enabledLayerCount = 0;
            device_ci.ppEnabledLayerNames = nullptr;
            device_ci.pEnabledFeatures = nullptr;
            device_ci.enabledExtensionCount = extension_names.size();
            device_ci.ppEnabledExtensionNames = extension_names.data();

            VkResult rslt = vkCreateDevice(m_physical_device, &device_ci, nullptr, &m_device);

            if (rslt == VK_SUCCESS) {
                BOOST_LOG_TRIVIAL(trace) << "Device created: " << m_device;
            } else {
                BOOST_LOG_TRIVIAL(error) << "Error creating device " << rslt;
                return false;
            }

            return m_queues.initialize(graphics_queue_family, present_queue_family) &&
                m_present.initialize() &&
                m_pipeline.initialize();
        }

        void Device::dispose() {
            m_pipeline.dispose();
            m_present.dispose();
            m_queues.dispose();

            if (m_device != VK_NULL_HANDLE) {
                BOOST_LOG_TRIVIAL(trace) << "Destroying device: " << m_device;
                vkDestroyDevice(m_device, nullptr);
                m_device = VK_NULL_HANDLE;
            }

            // No need to actually destroy the rest of these; they're
            // managed by Vulkan.
            m_physical_device = VK_NULL_HANDLE;
        }

        VkPhysicalDevice Device::choosePhysicalDevice(const std::vector<VkPhysicalDevice> &devices) {
            // We probably want to make a better decision than this...
            return devices[0];
        }

        uint32_t Device::chooseGraphicsQueueFamily(VkPhysicalDevice &device, const std::vector<VkQueueFamilyProperties> &families) {
            for (uint32_t i = 0; i < families.size(); ++i) {
                if (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                    return i;
                }
            }
            return UINT32_MAX;
        }

        uint32_t Device::choosePresentQueueFamily(VkPhysicalDevice &device, const std::vector<VkQueueFamilyProperties> &families, VkSurfaceKHR &surf) {
            for (uint32_t i = 0; i < families.size(); ++i) {
                VkBool32 supports_present = false;
                vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surf, &supports_present);
                if (supports_present) {
                    return i;
                }
            }
            return UINT32_MAX;
        }

        GLFWwindow* Device::window() {
            return m_parent->window();
        }

        VkInstance& Device::instance() {
            return m_parent->instance();
        }

        VkPhysicalDevice& Device::physicalDevice() {
            return m_physical_device;
        }

        VkDevice& Device::device() {
            return m_device;
        }

        VkSurfaceKHR& Device::surface() {
            return m_parent->surface();
        }

        CommandQueues& Device::queues() {
            return m_queues;
        }

        AssetFinder& Device::assetFinder() {
            return m_parent->assetFinder();
        }

        Presentation& Device::presentation() {
            return m_present;
        }

        Pipeline::Pipeline(Device *parent)
            : m_parent{parent},
              m_vertex_shader_module{VK_NULL_HANDLE},
              m_fragment_shader_module{VK_NULL_HANDLE},
              m_pipeline_layout{VK_NULL_HANDLE},
              m_render_pass{VK_NULL_HANDLE},
              m_pipeline{VK_NULL_HANDLE}
        {}

        Pipeline::~Pipeline() {
            dispose();
        }

        bool Pipeline::initialize() {
            VkDevice &dev = device();

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

            VkExtent2D &extent = presentation().swapchainExtent();
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
            color_att.format = presentation().swapchainFormat().format;
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
            rp_ci.dependencyCount = 0;
            rp_ci.pDependencies = nullptr;

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

            return true;
        }

        void Pipeline::dispose() {
            VkDevice &dev = device();

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

        VkShaderModule Pipeline::createShaderModule(const char *filename) {
            VkShaderModule rv = VK_NULL_HANDLE;

            VkShaderModuleCreateInfo sm_ci;
            sm_ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            sm_ci.pNext = nullptr;
            sm_ci.flags = 0;

            path bytecode_path = assetFinder().findShader(filename);
            if (!exists(bytecode_path)) {
                BOOST_LOG_TRIVIAL(error) << "Cannot find file " << bytecode_path;
                return rv;
            }

            std::ifstream bytecode_file(bytecode_path.string(), std::ios::ate | std::ios::binary);
            std::ifstream::streamoff bytecode_size = bytecode_file.tellg();
            std::vector<char> bytecode(static_cast<unsigned int>(bytecode_size));
            bytecode_file.seekg(0);
            bytecode_file.read(bytecode.data(), bytecode_size);
            bytecode_file.close();

            sm_ci.codeSize = static_cast<size_t>(bytecode_size);
            sm_ci.pCode = reinterpret_cast<uint32_t*>(bytecode.data());

            VkResult rslt = vkCreateShaderModule(device(), &sm_ci, nullptr, &rv);
            if (rslt == VK_SUCCESS) {
                BOOST_LOG_TRIVIAL(trace) << "Created shader module for " << filename << ": " << rv;
            } else {
                BOOST_LOG_TRIVIAL(trace) << "Failed to create shader module: " << rslt;
            }

            return rv;
        }

        VkDevice& Pipeline::device() {
            return m_parent->device();
        }

        AssetFinder& Pipeline::assetFinder() {
            return m_parent->assetFinder();
        }

        Presentation& Pipeline::presentation() {
            return m_parent->presentation();
        }

        Presentation::Presentation(Device *parent)
            : m_parent{parent},
              m_swapchain{VK_NULL_HANDLE},
              m_swapchain_images{},
              m_swapchain_image_views{},
              m_format{VK_FORMAT_UNDEFINED, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR},
              m_extent{0, 0}
        {}

        Presentation::~Presentation() {
            dispose();
        }

        bool Presentation::initialize() {
            if (m_swapchain == VK_NULL_HANDLE) {
                VkPhysicalDevice pdev = physicalDevice();
                VkDevice dev = device();
                VkSurfaceKHR surf = surface();
                CommandQueues &qs = queues();

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
                swapchain_ci.queueFamilyIndexCount = queue_families.size();
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

        void Presentation::dispose() {
            VkDevice &dev = device();
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

        VkSurfaceFormatKHR Presentation::chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &formats) {
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

        VkPresentModeKHR Presentation::choosePresentMode(const std::vector<VkPresentModeKHR> &modes) {
            // Prefer mailbox over fifo, if it's available.
            for (const auto &mode: modes) {
                if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
                    return mode;
                }
            }

            return VK_PRESENT_MODE_FIFO_KHR;
        }

        VkExtent2D Presentation::chooseSwapExtent(const VkSurfaceCapabilitiesKHR &surf_caps) {
            if (surf_caps.currentExtent.width != UINT32_MAX) {
                return surf_caps.currentExtent;
            }

            GLFWwindow *wnd = window();
            int width, height;
            glfwGetWindowSize(wnd, &width, &height);

            VkExtent2D extent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
            extent.width  = std::max(surf_caps.minImageExtent.width,  std::min(surf_caps.maxImageExtent.width,  extent.width));
            extent.height = std::max(surf_caps.minImageExtent.height, std::min(surf_caps.maxImageExtent.height, extent.height));
            return extent;
        }

        GLFWwindow* Presentation::window() {
            return m_parent->window();
        }

        VkPhysicalDevice& Presentation::physicalDevice() {
            return m_parent->physicalDevice();
        }

        VkDevice& Presentation::device() {
            return m_parent->device();
        }

        VkSurfaceKHR& Presentation::surface() {
            return m_parent->surface();
        }

        VkExtent2D& Presentation::swapchainExtent() {
            return m_extent;
        }

        VkSurfaceFormatKHR& Presentation::swapchainFormat() {
            return m_format;
        }

        CommandQueues& Presentation::queues() {
            return m_parent->queues();
        }

        System::System(GLFWwindow *window, const AssetFinder &asset_finder)
            : m_window{window},
              m_asset_finder{asset_finder},
              m_instance{VK_NULL_HANDLE},
              m_surface{VK_NULL_HANDLE},
              m_device{this}
        {}

        System::~System() {
            dispose();
        }

        bool System::initialize() {
            if (m_instance == VK_NULL_HANDLE) {
                logGlobalExtensions();
                logGlobalLayers();

                std::vector<const char*> extension_names;
                uint32_t glfw_extension_count;
                const char **glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
                for (unsigned int i = 0; i < glfw_extension_count; ++i) {
                    BOOST_LOG_TRIVIAL(trace) << "GLFW requires extension: " << glfw_extensions[i];
                    extension_names.emplace_back(glfw_extensions[i]);
                }

                std::vector<const char*> layer_names;
                layer_names.emplace_back("VK_LAYER_LUNARG_standard_validation");

                VkInstanceCreateInfo inst_ci;
                inst_ci.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
                inst_ci.pNext = nullptr;
                inst_ci.flags = 0;
                inst_ci.pApplicationInfo = nullptr;
                inst_ci.enabledLayerCount = layer_names.size();
                inst_ci.ppEnabledLayerNames = layer_names.data();
                inst_ci.enabledExtensionCount = static_cast<uint32_t>(extension_names.size());
                inst_ci.ppEnabledExtensionNames = extension_names.data();

                VkResult rslt = vkCreateInstance(&inst_ci, nullptr, &m_instance);
                if (rslt == VK_SUCCESS) {
                    BOOST_LOG_TRIVIAL(trace) << "Vulkan instance created: " << m_instance;
                } else {
                    BOOST_LOG_TRIVIAL(error) << "Error creating Vulkan instance: " << rslt;
                    return false;
                }
            }

            if (m_surface == VK_NULL_HANDLE) {
                VkResult rslt = glfwCreateWindowSurface(instance(), window(), nullptr, &m_surface);
                if (rslt == VK_SUCCESS) {
                    BOOST_LOG_TRIVIAL(trace) << "Created surface: " << m_surface;
                } else {
                    BOOST_LOG_TRIVIAL(error) << "Error creating surface: " << rslt;
                    return false;
                }
            }

            return m_device.initialize();
        }

        void System::dispose() {
            m_device.dispose();

            if (m_instance != VK_NULL_HANDLE && m_surface != VK_NULL_HANDLE) {
                BOOST_LOG_TRIVIAL(trace) << "Destroying surface: " << m_surface;
                vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
                m_surface = VK_NULL_HANDLE;
            }

            if (m_instance != VK_NULL_HANDLE) {
                BOOST_LOG_TRIVIAL(trace) << "Destroying Vulkan instance: " << m_instance;
                vkDestroyInstance(m_instance, nullptr);
                m_instance = VK_NULL_HANDLE;
            }
        }
    }
}
