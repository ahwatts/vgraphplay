// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#include <boost/log/trivial.hpp>

#include "../vulkan.h"

#include "System.h"
#include "../VulkanOutput.h"

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugReportFlagsEXT flags,
    VkDebugReportObjectTypeEXT object_type,
    uint64_t object,
    size_t location,
    int32_t code,
    const char *layer_prefix,
    const char *message,
    void *user_data
) {
    if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
        BOOST_LOG_TRIVIAL(error) << "Vulkan error: " << layer_prefix << ": " << message;
    } else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT) {
        BOOST_LOG_TRIVIAL(warning) << "Vulkan warning: " << layer_prefix << ": " << message;
    } else if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT) {
        BOOST_LOG_TRIVIAL(warning) << "Vulkan performance warning: " << layer_prefix << ": " << message;
    } else if (flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT) {
        BOOST_LOG_TRIVIAL(info) << "Vulkan info: " << layer_prefix << ": " << message;
    } else if (flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT) {
        BOOST_LOG_TRIVIAL(debug) << "Vulkan debug: " << layer_prefix << ": " << message;
    } else {
        BOOST_LOG_TRIVIAL(warning) << "Vulkan unknown level: " << layer_prefix << ": " << message;
    }

    return VK_FALSE;
}

vgraphplay::gfx::System::System(GLFWwindow *window)
    : m_window{window},
      m_instance{VK_NULL_HANDLE},
      m_debug_callback{VK_NULL_HANDLE},
      m_device{VK_NULL_HANDLE},
      m_physical_device{VK_NULL_HANDLE},
      m_graphics_queue_family{0},
      m_present_queue_family{0},
      m_graphics_queue{VK_NULL_HANDLE},
      m_present_queue{VK_NULL_HANDLE},
      m_command_pool{VK_NULL_HANDLE},
      m_command_buffers{},
      m_surface{VK_NULL_HANDLE},
      m_swapchain{VK_NULL_HANDLE},
      m_swapchain_images{},
      m_swapchain_image_views{},
      m_swapchain_format{VK_FORMAT_UNDEFINED, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR},
      m_swapchain_extent{0, 0},
      m_vertex_shader_module{VK_NULL_HANDLE},
      m_fragment_shader_module{VK_NULL_HANDLE},
      m_pipeline_layout{VK_NULL_HANDLE},
      m_render_pass{VK_NULL_HANDLE},
      m_pipeline{VK_NULL_HANDLE},
      m_swapchain_framebuffers{},
      m_image_available_semaphore{VK_NULL_HANDLE},
      m_render_finished_semaphore{VK_NULL_HANDLE}
{}

vgraphplay::gfx::System::~System() {
    dispose();
}

bool vgraphplay::gfx::System::initialize(bool debug) {
    bool rv = initInstance(debug);

    if (rv && debug) {
        rv = rv && initDebugCallback();
    }

    return rv;
    // if (m_callback == VK_NULL_HANDLE && debug) {
    //     VkDebugReportCallbackCreateInfoEXT drc_ci;
    //     drc_ci.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
    //     drc_ci.pNext = nullptr;
    //     drc_ci.flags =
    //         VK_DEBUG_REPORT_ERROR_BIT_EXT |
    //         VK_DEBUG_REPORT_WARNING_BIT_EXT |
    //         VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT |
    //         VK_DEBUG_REPORT_INFORMATION_BIT_EXT |
    //         VK_DEBUG_REPORT_DEBUG_BIT_EXT;
    //     drc_ci.pfnCallback = debugCallback;
    //     drc_ci.pUserData = nullptr;

    //     VkResult rslt = vkCreateDebugReportCallbackEXT(instance(), &drc_ci, nullptr, &m_callback);
    //     if (rslt == VK_SUCCESS) {
    //         BOOST_LOG_TRIVIAL(trace) << "Debug report callback created: " << m_callback;
    //     } else {
    //         BOOST_LOG_TRIVIAL(error) << "Error creating debug report callback: " << rslt;
    //         return false;
    //     }
    // }

    // if (m_surface == VK_NULL_HANDLE) {
    //     VkResult rslt = glfwCreateWindowSurface(instance(), window(), nullptr, &m_surface);
    //     if (rslt == VK_SUCCESS) {
    //         BOOST_LOG_TRIVIAL(trace) << "Created surface: " << m_surface;
    //     } else {
    //         BOOST_LOG_TRIVIAL(error) << "Error creating surface: " << rslt;
    //         return false;
    //     }
    // }

    // return m_device.initialize();
}

void vgraphplay::gfx::System::dispose() {
    if (m_device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(m_device);
    }

    cleanupDebugCallback();
    cleanupInstance();

    // m_device.dispose();

    // if (m_instance != VK_NULL_HANDLE && m_surface != VK_NULL_HANDLE) {
    //     BOOST_LOG_TRIVIAL(trace) << "Destroying surface: " << m_surface;
    //     vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
    //     m_surface = VK_NULL_HANDLE;
    // }
}

bool vgraphplay::gfx::System::initInstance(bool debug) {
    if (m_instance == VK_NULL_HANDLE) {
        logGlobalExtensions();
        logGlobalLayers();

        // The list of extensions we need.
        std::vector<const char*> extension_names;

        // Add the extensions GLFW wants to the list.
        uint32_t glfw_extension_count;
        const char **glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
        for (unsigned int i = 0; i < glfw_extension_count; ++i) {
            BOOST_LOG_TRIVIAL(trace) << "GLFW requires extension: " << glfw_extensions[i];
            extension_names.emplace_back(glfw_extensions[i]);
        }

        // Add the debug report extension if we're running in debug mode.
        if (debug) {
            extension_names.emplace_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
        }

        // Make sure we have the extensions we need.
        uint32_t num_extensions;
        vkEnumerateInstanceExtensionProperties(nullptr, &num_extensions, nullptr);
        std::vector<VkExtensionProperties> instance_extensions{num_extensions};
        vkEnumerateInstanceExtensionProperties(nullptr, &num_extensions, instance_extensions.data());
        for (unsigned int i = 0; i < extension_names.size(); ++i) {
            bool found = false;
            std::string wanted_extension_name{extension_names[i]};
            for (unsigned int j = 0; j < instance_extensions.size(); ++j) {
                std::string extension_name{instance_extensions[j].extensionName};
                if (wanted_extension_name == extension_name) {
                    found = true;
                    break;
                }
            }

            if (!found) {
                BOOST_LOG_TRIVIAL(error) << "Unable to find extension " << extension_names[i] << ". Cannot continue.";
                return false;
            }
        }

        // The layers we need.
        std::vector<const char*> layer_names;

        // Add the standard validation layers if we're running in debug mode.
        if (debug) {
            layer_names.emplace_back("VK_LAYER_LUNARG_standard_validation");
        }

        // Make sure we have the layers we need.
        uint32_t num_layers;
        vkEnumerateInstanceLayerProperties(&num_layers, nullptr);
        std::vector<VkLayerProperties> instance_layers{num_layers};
        vkEnumerateInstanceLayerProperties(&num_layers, instance_layers.data());
        for (unsigned int i = 0; i < layer_names.size(); ++i) {
            bool found = false;
            std::string wanted_layer_name{layer_names[i]};
            for (unsigned int j = 0; j < instance_layers.size(); ++j) {
                std::string layer_name{instance_layers[j].layerName};
                if (wanted_layer_name == layer_name) {
                    found = true;
                    break;
                }
            }

            if (!found) {
                BOOST_LOG_TRIVIAL(error) << "Unable to find layer " << layer_names[i] << ". Cannot continue.";
                return false;
            }
        }

        VkInstanceCreateInfo inst_ci;
        inst_ci.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        inst_ci.pNext = nullptr;
        inst_ci.flags = 0;
        inst_ci.pApplicationInfo = nullptr;
        inst_ci.enabledLayerCount = (uint32_t)layer_names.size();
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

    return true;
}

void vgraphplay::gfx::System::cleanupInstance() {
    if (m_instance != VK_NULL_HANDLE) {
        BOOST_LOG_TRIVIAL(trace) << "Destroying Vulkan instance: " << m_instance;
        vkDestroyInstance(m_instance, nullptr);
        m_instance = VK_NULL_HANDLE;
    }
}

bool vgraphplay::gfx::System::initDebugCallback() {
    if (m_instance != VK_NULL_HANDLE && m_debug_callback == VK_NULL_HANDLE) {
        VkDebugReportCallbackCreateInfoEXT drc_ci;
        drc_ci.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
        drc_ci.pNext = nullptr;
        drc_ci.flags =
            VK_DEBUG_REPORT_ERROR_BIT_EXT |
            VK_DEBUG_REPORT_WARNING_BIT_EXT |
            VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT |
            VK_DEBUG_REPORT_INFORMATION_BIT_EXT |
            VK_DEBUG_REPORT_DEBUG_BIT_EXT;
        drc_ci.pfnCallback = debugCallback;
        drc_ci.pUserData = nullptr;

        VkResult rslt = vkCreateDebugReportCallbackEXT(m_instance, &drc_ci, nullptr, &m_debug_callback);
        if (rslt == VK_SUCCESS) {
            BOOST_LOG_TRIVIAL(trace) << "Debug report callback created: " << m_debug_callback;
        } else {
            BOOST_LOG_TRIVIAL(error) << "Error creating debug report callback: " << rslt;
            return false;
        }
    }

    return true;
}

void vgraphplay::gfx::System::cleanupDebugCallback() {
    if (m_instance != VK_NULL_HANDLE && m_debug_callback != VK_NULL_HANDLE) {
        BOOST_LOG_TRIVIAL(trace) << "Destroying debug report callback: " << m_debug_callback;
        vkDestroyDebugReportCallbackEXT(m_instance, m_debug_callback, nullptr);
        m_debug_callback = VK_NULL_HANDLE;
    }
}

// bool vgraphplay::gfx::System::recordCommands() {
//     CommandStore &commandStore = m_device.commandStore();
//     Pipeline &pipeline = m_device.pipeline();
//     Presentation &presentation = m_device.presentation();

//     std::vector<VkCommandBuffer> &commandBuffers = commandStore.commandBuffers();
//     std::vector<VkFramebuffer> &swapchainFramebuffers = pipeline.swapchainFramebuffers();

//     for (unsigned int i = 0; i < commandBuffers.size(); ++i) {
//         VkCommandBufferBeginInfo cb_bi;
//         cb_bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
//         cb_bi.pNext = nullptr;
//         cb_bi.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
//         cb_bi.pInheritanceInfo = nullptr;

//         VkResult rslt = vkBeginCommandBuffer(commandBuffers[i], &cb_bi);
//         if (rslt == VK_SUCCESS) {
//             BOOST_LOG_TRIVIAL(trace) << "Beginning recording to command buffer " << commandBuffers[i];
//         } else {
//             BOOST_LOG_TRIVIAL(error) << "Error beginning command buffer recording: " << rslt;
//             return false;
//         }

//         VkRenderPassBeginInfo rp_bi;
//         rp_bi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
//         rp_bi.pNext = nullptr;
//         rp_bi.renderPass = pipeline.renderPass();
//         rp_bi.framebuffer = swapchainFramebuffers[i];
//         rp_bi.renderArea.offset = { 0, 0 };
//         rp_bi.renderArea.extent = presentation.swapchainExtent();

//         VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
//         rp_bi.clearValueCount = 1;
//         rp_bi.pClearValues = &clearColor;

//         vkCmdBeginRenderPass(commandBuffers[i], &rp_bi, VK_SUBPASS_CONTENTS_INLINE);
//         vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline());
//         vkCmdDraw(commandBuffers[i], 3, 1, 0, 0);
//         vkCmdEndRenderPass(commandBuffers[i]);

//         rslt = vkEndCommandBuffer(commandBuffers[i]);
//         if (rslt == VK_SUCCESS) {
//             BOOST_LOG_TRIVIAL(trace) << "Finished recording to command buffer " << commandBuffers[i];
//         } else {
//             BOOST_LOG_TRIVIAL(trace) << "Error finishing command buffer recording: " << rslt;
//             return false;
//         }
//     }

//     return true;
// }

void vgraphplay::gfx::System::drawFrame() {
    // CommandQueues &queues = m_device.queues();
    // CommandStore &command_store = m_device.commandStore();
    // Presentation &presentation = m_device.presentation();

    // VkDevice &device = m_device.device();
    // VkSwapchainKHR &swapchain = presentation.swapchain();
    // std::vector<VkCommandBuffer> &command_buffers = command_store.commandBuffers();
    // VkQueue &graphics_queue = queues.graphicsQueue();
    // VkQueue &present_queue = queues.presentQueue();
    // VkSemaphore &image_available = m_device.imageAvailableSemaphore();
    // VkSemaphore &render_finished = m_device.renderFinishedSemaphore();

    // uint32_t image_index;
    // vkAcquireNextImageKHR(
    //     device, swapchain, std::numeric_limits<uint64_t>::max(),
    //     image_available, VK_NULL_HANDLE, &image_index);

    // VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    // VkSubmitInfo si;
    // si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    // si.pNext = nullptr;
    // si.waitSemaphoreCount = 1;
    // si.pWaitSemaphores = &image_available;
    // si.pWaitDstStageMask = wait_stages;
    // si.commandBufferCount = 1;
    // si.pCommandBuffers = &command_buffers[image_index];
    // si.signalSemaphoreCount = 1;
    // si.pSignalSemaphores = &render_finished;

    // VkResult rslt = vkQueueSubmit(graphics_queue, 1, &si, VK_NULL_HANDLE);
    // if (rslt != VK_SUCCESS) {
    //     BOOST_LOG_TRIVIAL(error) << "Error submitting draw command buffer: " << rslt;
    // }

    // VkPresentInfoKHR pi;
    // pi.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    // pi.pNext = nullptr;
    // pi.waitSemaphoreCount = 1;
    // pi.pWaitSemaphores = &render_finished;
    // pi.swapchainCount = 1;
    // pi.pSwapchains = &swapchain;
    // pi.pImageIndices = &image_index;
    // pi.pResults = nullptr;

    // rslt = vkQueuePresentKHR(present_queue, &pi);
    // if (rslt != VK_SUCCESS) {
    //     BOOST_LOG_TRIVIAL(error) << "Error submitting swapchain update: " << rslt;
    // }
}
