// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

// #include <chrono>
// #include <set>
#include <format>
#include <vector>

#include <boost/log/trivial.hpp>

// #define GLM_FORCE_DEPTH_ZERO_TO_ONE
// #include <glm/glm.hpp>
// #include <glm/gtc/matrix_transform.hpp>

// #define STB_IMAGE_IMPLEMENTATION
// #include <stb_image.h>

#include "../vulkan.h"

#include "System.h"
#include "../VulkanOutput.h"

bool hasExtension(std::vector<vk::ExtensionProperties> &all_extensions, const char *extension_name);
bool hasLayer(std::vector<vk::LayerProperties> &all_layers, const char *layer_name);
std::vector<const char *> buildInstanceExtensionList(vk::raii::Context &context, bool debug);
std::vector<const char *> buildInstanceLayerList(vk::raii::Context &context, bool debug);

vk::raii::PhysicalDevice choosePhysicalDevice(const std::vector<vk::raii::PhysicalDevice> &devices /*, vk::SurfaceKHR &surface */);
vk::SurfaceFormatKHR chooseSwapchainSurfaceFormat(const std::vector<vk::SurfaceFormatKHR> &available_formats);
vk::PresentModeKHR chooseSwapchainPresentMode(const std::vector<vk::PresentModeKHR> &available_modes);
vk::Extent2D chooseSwapchainExtent(GLFWwindow *window, const vk::SurfaceCapabilitiesKHR &caps);

void transitionImageLayout(
    const vk::raii::CommandBuffer &commands,
    const vk::Image &image,
    vk::ImageLayout old_layout,
    vk::ImageLayout new_layout,
    vk::AccessFlags2 src_access_mask,
    vk::AccessFlags2 dst_access_mask,
    vk::PipelineStageFlags2 src_stage_mask,
    vk::PipelineStageFlags2 dst_stage_mask
);

const std::vector<unsigned char> &UNLIT_BYTECODE = LOAD_RESOURCE(unlit_slang_spv);
const std::vector<unsigned char> &WARREN_TEXTURE = LOAD_RESOURCE(warren_jpg);

const uint16_t NUM_RECTANGLE_VERTICES = 8;
const vgraphplay::gfx::Vertex RECTANGLE_VERTICES[NUM_RECTANGLE_VERTICES] = {
    {{-0.5f, -0.5f,  0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
    {{ 0.5f, -0.5f,  0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
    {{ 0.5f,  0.5f,  0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
    {{-0.5f,  0.5f,  0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},

    {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
    {{ 0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
    {{ 0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
    {{-0.5f,  0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
};

const uint16_t NUM_RECTANGLE_INDICES = 12;
const uint16_t RECTANGLE_INDICES[NUM_RECTANGLE_INDICES] = {
    0, 1, 2, 2, 3, 0,
    4, 5, 6, 6, 7, 4,
};

static VKAPI_ATTR vk::Bool32 VKAPI_CALL handleDebugMessage(
    vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
    vk::DebugUtilsMessageTypeFlagsEXT type,
    const vk::DebugUtilsMessengerCallbackDataEXT *pCallbackData,
    void *pUserData
) {
    std::string msg = std::format("Vulkan debug message: type: {} msg: {}", to_string(type), pCallbackData->pMessage);
    if (severity & vk::DebugUtilsMessageSeverityFlagBitsEXT::eError) {
        BOOST_LOG_TRIVIAL(error) << msg;
    } else if (severity & vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning) {
        BOOST_LOG_TRIVIAL(warning) << msg;
    } else if (severity & vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo) {
        BOOST_LOG_TRIVIAL(info) << msg;
    } else if (severity & vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose) {
        BOOST_LOG_TRIVIAL(debug) << msg;
    } else {
        BOOST_LOG_TRIVIAL(warning) << msg << " (unknown severity: " << to_string(severity) << ")";
    }

    return vk::False;
}

vgraphplay::gfx::System::System(GLFWwindow *window, bool debug)
    : m_debug{debug},
      m_window{window},
      m_frame_index{0},
      m_context{},
      m_instance{nullptr},
      m_debug_messenger{nullptr},
      m_physical_device{nullptr},
      m_device{nullptr},
      m_command_queue_family_index{~static_cast<uint32_t>(0)},
      m_command_queue{nullptr},
      m_command_pool{nullptr},
      m_command_buffers{},
      m_surface{nullptr},
      m_swapchain_format{},
      m_swapchain_extent{0, 0},
      m_swapchain_image_count{0},
      m_swapchain{nullptr},
      m_swapchain_images{},
      m_swapchain_image_views{},
      m_framebuffer_resized{false},
      m_pipeline_layout{nullptr},
      m_pipeline{nullptr},
      m_present_complete_semaphores{},
      m_render_finished_semaphores{},
      m_draw_fences{}
    //   m_present_queue{VK_NULL_HANDLE},
    //   m_vertex_buffer{VK_NULL_HANDLE},
    //   m_index_buffer{VK_NULL_HANDLE},
    //   m_uniform_buffers{},
    //   m_vertex_buffer_memory{VK_NULL_HANDLE},
    //   m_index_buffer_memory{VK_NULL_HANDLE},
    //   m_uniform_buffers_memory{},
    //   m_texture_image{VK_NULL_HANDLE},
    //   m_texture_image_memory{VK_NULL_HANDLE},
    //   m_texture_image_view{VK_NULL_HANDLE},
    //   m_texture_sampler{VK_NULL_HANDLE},
    //   m_framebuffer_resized{false},
    //   m_depth_image{VK_NULL_HANDLE},
    //   m_depth_image_memory{VK_NULL_HANDLE},
    //   m_depth_image_view{VK_NULL_HANDLE},
    //   m_descriptor_set_layout{VK_NULL_HANDLE},
    //   m_descriptor_pool{VK_NULL_HANDLE},
    //   m_descriptor_sets{},
{
    initInstance();
    initDebugMessenger();
    initSurface();
    initDevice();
    initSwapchain();
    initPipeline();
    initCommandPool();
    initCommandBuffer();
    initSynchronizationObjects();
}

vgraphplay::gfx::System::~System() {
    // Wait until things are finished before destructing the chirren.
    if (m_command_queue != nullptr) {
        m_command_queue.waitIdle();
    }
}

/* bool vgraphplay::gfx::System::initialize(bool debug) {
    rv = rv && initRenderPass();
    rv = rv && initDescriptorSetLayout();
    rv = rv && initSemaphores();
    rv = rv && initDepthResources();
    rv = rv && initSwapchainFramebuffers();
    rv = rv && initTextureImage();
    rv = rv && initTextureImageView();
    rv = rv && initTextureSampler();
    rv = rv && initVertexBuffer();
    rv = rv && initIndexBuffer();
    rv = rv && initUniformBuffers();
    rv = rv && initDescriptorPool();
    rv = rv && initDescriptorSets();
    rv = rv && recordCommandBuffers();

    return rv;
} */

void vgraphplay::gfx::System::initInstance() {
    if (m_instance != nullptr) {
        return;
    }

    // logInstanceExtensions(m_context);
    // logInstanceLayers(m_context);

    vk::InstanceCreateFlags flags;
    std::vector<const char *> extension_names = buildInstanceExtensionList(m_context, m_debug);
    std::vector<const char *> layer_names = buildInstanceLayerList(m_context, m_debug);

    // In addition to the extension that was checked for and added above, we
    // also need to pass this flag in.
    #ifdef __APPLE__
    flags |= vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;
    #endif

    constexpr vk::ApplicationInfo app_info{
        .pApplicationName = "vgraphplay",
        .applicationVersion = VK_MAKE_VERSION(0, 1, 0),
        .pEngineName = "No Engine",
        .engineVersion = VK_MAKE_VERSION(0, 1, 0),
        .apiVersion = vk::ApiVersion14
    };

    vk::InstanceCreateInfo inst_ci{
        .flags = flags,
        .pApplicationInfo = &app_info,
        .enabledLayerCount = static_cast<uint32_t>(layer_names.size()),
        .ppEnabledLayerNames = layer_names.data(),
        .enabledExtensionCount = static_cast<uint32_t>(extension_names.size()),
        .ppEnabledExtensionNames = extension_names.data(),
    };

    m_instance = m_context.createInstance(inst_ci);
    BOOST_LOG_TRIVIAL(trace) << "Vulkan instance created: " << *m_instance;
}

void vgraphplay::gfx::System::initDebugMessenger() {
    if (!m_debug || m_debug_messenger != nullptr) {
        return;
    }

    if (m_instance == nullptr) {
        throw std::runtime_error("Cannot create debug messenger; Vulkan instance is null");
    }

    vk::DebugUtilsMessengerCreateInfoEXT dm_ci{
        .messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eError |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose,
        .messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
            vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
            vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation,
        .pfnUserCallback = handleDebugMessage,
    };

    m_debug_messenger = m_instance.createDebugUtilsMessengerEXT(dm_ci);
    BOOST_LOG_TRIVIAL(trace) << "Created debug messenger: " << *m_debug_messenger;
}

vk::raii::PhysicalDevice choosePhysicalDevice(const std::vector<vk::raii::PhysicalDevice> &devices /*, vk::SurfaceKHR &surface */) {
    std::vector<const char *> required_extensions{
        vk::KHRSwapchainExtensionName,
    };

    #ifdef __APPLE__
    required_extensions.push_back(vk::KHRPortabilitySubsetExtensionName);
    #endif

    for (auto &dev : devices) {
        const vk::PhysicalDeviceProperties props = dev.getProperties();
        const auto features = dev.template getFeatures2<
            vk::PhysicalDeviceFeatures2,
            vk::PhysicalDeviceVulkan11Features,
            vk::PhysicalDeviceVulkan13Features, 
            vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT
        >();
        const std::vector<vk::ExtensionProperties> all_extensions = dev.enumerateDeviceExtensionProperties();
        const std::vector<vk::QueueFamilyProperties> queue_families = dev.getQueueFamilyProperties();

        bool supports_vulkan_13 = props.apiVersion >= vk::ApiVersion13;
        bool supports_graphics = std::ranges::any_of(
            queue_families,
            [](const auto &qfp) { return !!(qfp.queueFlags & vk::QueueFlagBits::eGraphics); }
        );
        bool supports_all_extensions = std::ranges::all_of(
            required_extensions,
            [&all_extensions](const auto &this_req_ext) {
                return std::ranges::any_of(
                    all_extensions,
                    [this_req_ext](const auto &ext) {
                        return strcmp(ext.extensionName, this_req_ext) == 0;
                    }
                );
            }
        );
        bool supports_dynamic_rendering = features.template get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering;
        bool supports_dynamic_state = features.template get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState;
        bool supports_draw_parameters = features.template get<vk::PhysicalDeviceVulkan11Features>().shaderDrawParameters;

        if (supports_vulkan_13 && 
            supports_graphics && 
            supports_all_extensions && 
            supports_dynamic_rendering && 
            supports_dynamic_state && 
            supports_draw_parameters)
        {
            return dev;
        }
    }

    throw std::runtime_error{"Could not find a suitable GPU"};
}

void vgraphplay::gfx::System::initDevice() {
    if (m_device != nullptr) {
        return;
    }

    if (m_instance == nullptr || m_surface == nullptr) {
        throw std::runtime_error("Cannot create device; Vulkan instance or surface is null");
    }

    logPhysicalDevices(m_instance);
    const std::vector<vk::raii::PhysicalDevice> physical_devices = m_instance.enumeratePhysicalDevices();
    m_physical_device = choosePhysicalDevice(physical_devices);
    BOOST_LOG_TRIVIAL(trace) << "Chose physical device " << m_physical_device.getProperties().deviceName;

    m_command_queue_family_index = ~0;
    const std::vector<vk::QueueFamilyProperties> qfps = m_physical_device.getQueueFamilyProperties();
    for (uint32_t i = 0; i < qfps.size(); ++i) {
        if (qfps[i].queueFlags & vk::QueueFlagBits::eGraphics &&
            m_physical_device.getSurfaceSupportKHR(i, *m_surface))
        {
            m_command_queue_family_index = i;
            break;
        }
    }

    if (m_command_queue_family_index == ~0) {
        throw std::runtime_error("Unable to find a queue family that supports graphics and presentation");
    }

    float command_queue_priority = 0.5f;
    vk::DeviceQueueCreateInfo command_queue_ci{
        .queueFamilyIndex = m_command_queue_family_index,
        .queueCount = 1,
        .pQueuePriorities = &command_queue_priority,
    };

    vk::StructureChain<
        vk::PhysicalDeviceFeatures2, 
        vk::PhysicalDeviceVulkan11Features,
        vk::PhysicalDeviceVulkan13Features, 
        vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT
    > feature_chain = {
        {},                             // vk::PhysicalDeviceFeatures2, empty (for now)
        {.shaderDrawParameters = true}, // Enable shader draw parameters (we need this for SV_VertexID in the shader)
        {
            .synchronization2 = true,   // Support new synchronization commands
            .dynamicRendering = true,   // Enable dynamic rendering from Vulkan 1.3
        },     
        {.extendedDynamicState = true}, // Enable extended dynamic state from the extension
    };

    std::vector<const char *> required_device_extensions = {
        vk::KHRSwapchainExtensionName,
    };

    #ifdef __APPLE__
    required_device_extensions.push_back(vk::KHRPortabilitySubsetExtensionName);
    #endif

    vk::DeviceCreateInfo device_ci = vk::DeviceCreateInfo{.pNext = &feature_chain.get<vk::PhysicalDeviceFeatures2>()}
        .setQueueCreateInfos(command_queue_ci)
        .setPEnabledExtensionNames(required_device_extensions);

    m_device = m_physical_device.createDevice(device_ci);
    BOOST_LOG_TRIVIAL(trace) << "Created device: " << *m_device;
    m_command_queue = vk::raii::Queue(m_device, m_command_queue_family_index, 0);
    BOOST_LOG_TRIVIAL(trace) << "Created command (graphics & presentation) queue: " << *m_command_queue;
}

void vgraphplay::gfx::System::initSurface() {
    if (m_surface != nullptr) {
        return;
    }

    if (m_instance == nullptr) {
        throw std::runtime_error("Cannot create surface; Vulkan instance is null");
    }

    VkSurfaceKHR surface_ = VK_NULL_HANDLE;
    VkResult rslt = glfwCreateWindowSurface(*m_instance, m_window, nullptr, &surface_);
    if (rslt != VK_SUCCESS) {
        throw std::runtime_error("Could not create surface");
    }

    m_surface = vk::raii::SurfaceKHR(m_instance, surface_);
    BOOST_LOG_TRIVIAL(trace) << "Created surface: " << *m_surface;
}

vk::SurfaceFormatKHR chooseSwapchainSurfaceFormat(const std::vector<vk::SurfaceFormatKHR> &available_formats) {
    assert(!available_formats.empty());

    const auto format = std::ranges::find_if(available_formats, [](const auto &f) {
        return f.format == vk::Format::eB8G8R8A8Srgb && f.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
    });
    return format != available_formats.end() ? *format : available_formats[0];
}

vk::PresentModeKHR chooseSwapchainPresentMode(const std::vector<vk::PresentModeKHR> &available_modes) {
    bool has_mailbox = std::ranges::any_of(
        available_modes,
        [](const auto mode) { return vk::PresentModeKHR::eMailbox == mode; }
    );

    return has_mailbox ? vk::PresentModeKHR::eMailbox : vk::PresentModeKHR::eFifo;
}

vk::Extent2D chooseSwapchainExtent(GLFWwindow *window, const vk::SurfaceCapabilitiesKHR &caps) {
    if (caps.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return caps.currentExtent;
    }
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    return {
        std::clamp<uint32_t>(width, caps.minImageExtent.width, caps.maxImageExtent.width),
        std::clamp<uint32_t>(height, caps.minImageExtent.height, caps.maxImageExtent.height),
    };
}

void vgraphplay::gfx::System::initSwapchain() {
    if (m_swapchain != nullptr) {
        return;
    }

    if (m_physical_device == nullptr || m_surface == nullptr || m_device == nullptr) {
        throw std::runtime_error("Cannot create swapchain; Vulkan instance, surface, or device is null");
    }

    vk::SurfaceCapabilitiesKHR surf_caps = m_physical_device.getSurfaceCapabilitiesKHR(*m_surface);
    std::vector<vk::SurfaceFormatKHR> formats = m_physical_device.getSurfaceFormatsKHR(*m_surface);
    std::vector<vk::PresentModeKHR> modes = m_physical_device.getSurfacePresentModesKHR(*m_surface);

    m_swapchain_format = chooseSwapchainSurfaceFormat(formats);
    m_swapchain_extent = chooseSwapchainExtent(m_window, surf_caps);
    vk::PresentModeKHR present_mode = chooseSwapchainPresentMode(modes);

    m_swapchain_image_count = std::max(3u, surf_caps.minImageCount);
    if ((surf_caps.maxImageCount > 0) && (surf_caps.maxImageCount < m_swapchain_image_count)) {
        m_swapchain_image_count = surf_caps.maxImageCount;
    }

    vk::SwapchainCreateInfoKHR swapchain_ci{
        .surface = m_surface,
        .minImageCount = m_swapchain_image_count,
        .imageFormat = m_swapchain_format.format,
        .imageColorSpace = m_swapchain_format.colorSpace,
        .imageExtent = m_swapchain_extent,
        .imageArrayLayers = 1,
        .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
        .imageSharingMode = vk::SharingMode::eExclusive,
        .preTransform = surf_caps.currentTransform,
        .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
        .presentMode = present_mode,
        .clipped = true,
    };
    m_swapchain = m_device.createSwapchainKHR(swapchain_ci);
    BOOST_LOG_TRIVIAL(trace) << "Created swapchain: " << *m_swapchain;
    m_swapchain_images = m_swapchain.getImages();
    BOOST_LOG_TRIVIAL(trace) << "Retrieved " << m_swapchain_images.size() << " swapchain images";
    m_swapchain_image_count = m_swapchain_images.size();

    m_swapchain_image_views.clear();
    vk::ImageViewCreateInfo imgv_ci{
        .viewType = vk::ImageViewType::e2D,
        .format = m_swapchain_format.format,
        .components = {
            vk::ComponentSwizzle::eIdentity,
            vk::ComponentSwizzle::eIdentity,
            vk::ComponentSwizzle::eIdentity,
            vk::ComponentSwizzle::eIdentity,
        },
        .subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 },
    };

    for (auto &image : m_swapchain_images) {
        imgv_ci.image = image;
        m_swapchain_image_views.emplace_back(m_device, imgv_ci);
        BOOST_LOG_TRIVIAL(trace) << "Created swapchain image view: " << *m_swapchain_image_views.back() << " for swapchain image " << image;
    }
}

void vgraphplay::gfx::System::recreateSwapchain() {
    int width{0}, height{0};
    glfwGetFramebufferSize(m_window, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(m_window, &width, &height);
        glfwWaitEvents();
    }

    m_device.waitIdle();
    m_swapchain_image_views.clear();
    m_swapchain_images.clear();
    m_swapchain = nullptr;
    initSwapchain();
}

void vgraphplay::gfx::System::initPipeline() {
    if (m_pipeline != nullptr) {
        return;
    }

    if (m_device == nullptr) {
        throw std::runtime_error("Unable to create pipeline; device is null");
    }
    
    vk::ShaderModuleCreateInfo sm_ci = vk::ShaderModuleCreateInfo{
        // This sizeof() is a very verbose way to say 1...
        .codeSize = UNLIT_BYTECODE.size() * sizeof(std::remove_reference<decltype(UNLIT_BYTECODE)>::type::value_type),
        .pCode = reinterpret_cast<const uint32_t *>(UNLIT_BYTECODE.data()),
    };
    vk::raii::ShaderModule shader_module = m_device.createShaderModule(sm_ci);
    BOOST_LOG_TRIVIAL(trace) << "Created shader module: " << *shader_module;

    std::array<vk::PipelineShaderStageCreateInfo, 2> shader_stages{
        vk::PipelineShaderStageCreateInfo{
            .stage = vk::ShaderStageFlagBits::eVertex,
            .module = shader_module,
            .pName = "vs_main",
        },
        vk::PipelineShaderStageCreateInfo{
            .stage = vk::ShaderStageFlagBits::eFragment,
            .module = shader_module,
            .pName = "fs_main",
        },
    };
    vk::PipelineVertexInputStateCreateInfo vertex_input_ci{};

    vk::PipelineInputAssemblyStateCreateInfo input_assembly_ci{
        .topology = vk::PrimitiveTopology::eTriangleList,
    };

    vk::PipelineViewportStateCreateInfo viewport_ci{
        .viewportCount = 1,
        .scissorCount = 1,
    };

    vk::PipelineRasterizationStateCreateInfo raster_ci{
        .depthClampEnable = vk::False,
        .rasterizerDiscardEnable = vk::False,
        .polygonMode = vk::PolygonMode::eFill,
        .cullMode = vk::CullModeFlagBits::eBack,
        .frontFace = vk::FrontFace::eClockwise,
        .depthBiasEnable = vk::False,
        .lineWidth = 1.0f,
    };

    vk::PipelineMultisampleStateCreateInfo multisample_state_ci{
        .rasterizationSamples = vk::SampleCountFlagBits::e1,
        .sampleShadingEnable = vk::False,
    };

    vk::PipelineColorBlendAttachmentState blend_attachment{
        .blendEnable = vk::False,
        .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
    };
    vk::PipelineColorBlendStateCreateInfo color_blend_ci = vk::PipelineColorBlendStateCreateInfo{
        .logicOpEnable = vk::False,
        .logicOp = vk::LogicOp::eCopy,        
    }.setAttachments(blend_attachment);

    std::array<vk::DynamicState, 2> dynamic_states{
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor,
    };
    vk::PipelineDynamicStateCreateInfo dynamic_state_ci = vk::PipelineDynamicStateCreateInfo{}.setDynamicStates(dynamic_states);

    vk::PipelineLayoutCreateInfo layout_ci{
        .setLayoutCount = 0,
        .pushConstantRangeCount = 0,
    };
    m_pipeline_layout = m_device.createPipelineLayout(layout_ci);
    BOOST_LOG_TRIVIAL(trace) << "Created pipeline layout: " << *m_pipeline_layout;

    vk::StructureChain<vk::GraphicsPipelineCreateInfo, vk::PipelineRenderingCreateInfo> pipeline_ci{
        {
            .pVertexInputState = &vertex_input_ci,
            .pInputAssemblyState = &input_assembly_ci,
            .pViewportState = &viewport_ci,
            .pRasterizationState = &raster_ci,
            .pMultisampleState = &multisample_state_ci,
            .pColorBlendState = &color_blend_ci,
            .pDynamicState = &dynamic_state_ci,
            .layout = m_pipeline_layout,
            .renderPass = nullptr,
        },
        {},
    };
    pipeline_ci.get<vk::GraphicsPipelineCreateInfo>().setStages(shader_stages);
    pipeline_ci.get<vk::PipelineRenderingCreateInfo>().setColorAttachmentFormats(m_swapchain_format.format);

    m_pipeline = m_device.createGraphicsPipeline(nullptr, pipeline_ci.get<vk::GraphicsPipelineCreateInfo>());
    BOOST_LOG_TRIVIAL(trace) << "Created graphics pipeline: " << *m_pipeline;
}

void vgraphplay::gfx::System::initCommandPool() {
    if (m_command_pool != nullptr) {
        return;
    }

    if (m_device == nullptr) {
        throw std::runtime_error{"Unable to create command pool; device is null"};
    }

    vk::CommandPoolCreateInfo cp_ci{
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = m_command_queue_family_index,
    };

    m_command_pool = m_device.createCommandPool(cp_ci);
    BOOST_LOG_TRIVIAL(trace) << "Created command pool: " << *m_command_pool;
}

void vgraphplay::gfx::System::initCommandBuffer() {
    if (!m_command_buffers.empty()) {
        return;
    }

    if (m_device == nullptr || m_command_pool == nullptr) {
        throw std::runtime_error{"Unable to create command buffer; device or command pool is null"};
    }

    vk::CommandBufferAllocateInfo cb_ai{
        .commandPool = m_command_pool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = MAX_FRAMES_IN_FLIGHT,
    };

    m_command_buffers = vk::raii::CommandBuffers(m_device, cb_ai);
    for (auto &b : m_command_buffers) {
        BOOST_LOG_TRIVIAL(trace) << "Created command buffer: " << *b;
    }
}

void transitionImageLayout(
    const vk::raii::CommandBuffer &commands,
    const vk::Image &image,
    vk::ImageLayout old_layout,
    vk::ImageLayout new_layout,
    vk::AccessFlags2 src_access_mask,
    vk::AccessFlags2 dst_access_mask,
    vk::PipelineStageFlags2 src_stage_mask,
    vk::PipelineStageFlags2 dst_stage_mask
) {
    vk::ImageMemoryBarrier2 barrier{
        .srcStageMask = src_stage_mask,
        .srcAccessMask = src_access_mask,
        .dstStageMask = dst_stage_mask,
        .dstAccessMask = dst_access_mask,
        .oldLayout = old_layout,
        .newLayout = new_layout,
        .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
        .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
        .image = image,
        .subresourceRange = {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
    };

    vk::DependencyInfo dep = vk::DependencyInfo{
        .dependencyFlags = {},
    }.setImageMemoryBarriers(barrier);

    commands.pipelineBarrier2(dep);
}

void vgraphplay::gfx::System::recordRenderInCommandBuffer(uint32_t image_index) {
    vk::raii::CommandBuffer &command_buffer = m_command_buffers[m_frame_index];

    command_buffer.begin({});
    
    // Transition the image to the color attachment layout.
    transitionImageLayout(
        command_buffer, 
        m_swapchain_images[image_index],
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eColorAttachmentOptimal,
        {},
        vk::AccessFlagBits2::eColorAttachmentWrite,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput
    );

    // Start the rendering to the color attachment.
    vk::ClearValue clear_color = vk::ClearColorValue{0.0f, 0.0f, 0.0f, 1.0f};
    vk::RenderingAttachmentInfo rai{
        .imageView = m_swapchain_image_views[image_index],
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .clearValue = clear_color,
    };
    vk::RenderingInfo ri = vk::RenderingInfo{
        .renderArea = {.offset = {0, 0}, .extent = m_swapchain_extent},
        .layerCount = 1,
    }.setColorAttachments(rai);
    command_buffer.beginRendering(ri);

    // Render.
    command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *m_pipeline);
    command_buffer.setViewport(0, vk::Viewport{0.0f, 0.0f, static_cast<float>(m_swapchain_extent.width), static_cast<float>(m_swapchain_extent.height)});
    command_buffer.setScissor(0, vk::Rect2D{vk::Offset2D{0, 0}, m_swapchain_extent});
    command_buffer.draw(3, 1, 0, 0);

    // Done rendering.
    command_buffer.endRendering();

    // Transition the image to be presentable.
    transitionImageLayout(
        command_buffer,
        m_swapchain_images[image_index],
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::ImageLayout::ePresentSrcKHR,
        vk::AccessFlagBits2::eColorAttachmentWrite,
        {},
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::PipelineStageFlagBits2::eBottomOfPipe
    );

    command_buffer.end();
}

void vgraphplay::gfx::System::initSynchronizationObjects() {
    if (m_device == nullptr) {
        throw std::runtime_error{"Unable to create synchronization objects; device is null"};
    }

    for (size_t i = 0; i < m_swapchain_image_count; ++i) {
        m_render_finished_semaphores.emplace_back(m_device, vk::SemaphoreCreateInfo{});
        BOOST_LOG_TRIVIAL(trace) << "Created semaphore (render finished) for image " << i << ": " << *m_render_finished_semaphores.back();
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)  {
        m_present_complete_semaphores.emplace_back(m_device, vk::SemaphoreCreateInfo{});
        BOOST_LOG_TRIVIAL(trace) << "Created semaphore (present complete) for frame " << i << ": " << *m_present_complete_semaphores.back();
        m_draw_fences.emplace_back(m_device, vk::FenceCreateInfo{.flags = vk::FenceCreateFlagBits::eSignaled});
        BOOST_LOG_TRIVIAL(trace) << "Created fence (draw) for frame " << i << ": " << *m_draw_fences.back();
    }
}

void vgraphplay::gfx::System::drawFrame() {
    vk::raii::CommandBuffer &command_buffer = m_command_buffers[m_frame_index];
    vk::raii::Semaphore &present_complete = m_present_complete_semaphores[m_frame_index];
    vk::raii::Fence &draw_fence = m_draw_fences[m_frame_index];

    vk::Result result = m_device.waitForFences(*draw_fence, vk::True, UINT64_MAX);
    if (result != vk::Result::eSuccess) {
        throw std::runtime_error{"Error waiting for draw fence"};
    }

    auto [result2, image_index] = m_swapchain.acquireNextImage(
        UINT64_MAX, 
        *present_complete, 
        nullptr
    );
    if (result2 == vk::Result::eErrorOutOfDateKHR) {
        recreateSwapchain();
        return;
    } else if (result2 != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR) {
        throw std::runtime_error{"Error acquiring next swapchain image"};
    }

    m_device.resetFences(*draw_fence);

    vk::raii::Semaphore &render_finished = m_render_finished_semaphores[image_index];
    recordRenderInCommandBuffer(image_index);

    vk::PipelineStageFlags wait_dst_stage_mask{vk::PipelineStageFlagBits::eColorAttachmentOutput};
    vk::SubmitInfo submit_info = vk::SubmitInfo{.pWaitDstStageMask = &wait_dst_stage_mask}
        .setWaitSemaphores(*present_complete)
        .setCommandBuffers(*command_buffer)
        .setSignalSemaphores(*render_finished);

    m_command_queue.submit(submit_info, *draw_fence);

    vk::PresentInfoKHR present_info = vk::PresentInfoKHR{}
        .setWaitSemaphores(*render_finished)
        .setSwapchains(*m_swapchain)
        .setImageIndices(image_index);

    result = m_command_queue.presentKHR(present_info);
    if (result == vk::Result::eSuboptimalKHR || result == vk::Result::eErrorOutOfDateKHR || m_framebuffer_resized) {
        m_framebuffer_resized = false;
        recreateSwapchain();
    } else if (result != vk::Result::eSuccess) {
        throw std::runtime_error{"Error presenting new image"};
    }

    m_frame_index = (m_frame_index + 1) % MAX_FRAMES_IN_FLIGHT;
}

void vgraphplay::gfx::System::setFramebufferResized() {
    m_framebuffer_resized = true;
}

/* bool vgraphplay::gfx::System::initDescriptorSetLayout() {
    if (m_descriptor_set_layout != VK_NULL_HANDLE) {
        return true;
    }

    if (m_device == VK_NULL_HANDLE) {
        BOOST_LOG_TRIVIAL(error) << "Things have been initialized out of order. Unable to create descriptor set layout.";
        return false;
    }

    std::array<VkDescriptorSetLayoutBinding, 2> bindings{};

    // UBO binding.
    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    bindings[0].pImmutableSamplers = nullptr;

    // Sampler binding.
    bindings[1].binding = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[1].descriptorCount = 1;
    bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    bindings[1].pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo dsl_ci;
    dsl_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    dsl_ci.pNext = nullptr;
    dsl_ci.flags = 0;
    dsl_ci.bindingCount = static_cast<uint32_t>(bindings.size());
    dsl_ci.pBindings = bindings.data();

    VkResult rslt = vkCreateDescriptorSetLayout(m_device, &dsl_ci, nullptr, &m_descriptor_set_layout);
    if (rslt == VK_SUCCESS) {
        BOOST_LOG_TRIVIAL(trace) << "Created descriptor set layout: " << m_descriptor_set_layout;
        return true;
    } else {
        BOOST_LOG_TRIVIAL(error) << "Error creating descriptor set layout " << rslt;
        return false;
    }
}

void vgraphplay::gfx::System::cleanupDescriptorSetLayout() {
    if (m_device != VK_NULL_HANDLE && m_descriptor_set_layout != VK_NULL_HANDLE) {
        BOOST_LOG_TRIVIAL(trace) << "Destroying descriptor set layout: " << m_descriptor_set_layout;
        vkDestroyDescriptorSetLayout(m_device, m_descriptor_set_layout, nullptr);
        m_descriptor_set_layout = VK_NULL_HANDLE;
    }
}

bool vgraphplay::gfx::System::initSemaphores() {
    if (m_image_available_semaphore != VK_NULL_HANDLE &&
        m_render_finished_semaphore != VK_NULL_HANDLE) {
        return true;
    }

    if (m_device == VK_NULL_HANDLE) {
        BOOST_LOG_TRIVIAL(error) << "Things have been initialized out of order. Cannot create semaphores.";
        return false;
    }

    VkSemaphoreCreateInfo sem_ci;
    sem_ci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    sem_ci.pNext = nullptr;
    sem_ci.flags = 0;

    if (m_image_available_semaphore == VK_NULL_HANDLE) {
        VkResult rslt = vkCreateSemaphore(m_device, &sem_ci, nullptr, &m_image_available_semaphore);
        if (rslt == VK_SUCCESS) {
            BOOST_LOG_TRIVIAL(trace) << "Created image available semaphore: " << m_image_available_semaphore;
        } else {
            BOOST_LOG_TRIVIAL(error) << "Error creating image available semaphore: " << rslt;
            return false;
        }
    }

    if (m_render_finished_semaphore == VK_NULL_HANDLE) {
        VkResult rslt = vkCreateSemaphore(m_device, &sem_ci, nullptr, &m_render_finished_semaphore);
        if (rslt == VK_SUCCESS) {
            BOOST_LOG_TRIVIAL(trace) << "Created render finished semaphore: " << m_render_finished_semaphore;
        } else {
            BOOST_LOG_TRIVIAL(error) << "Error creating render finished semaphore: " << rslt;
            return false;
        }
    }

    return true;
}

void vgraphplay::gfx::System::cleanupSemaphores() {
    if (m_device != VK_NULL_HANDLE && m_image_available_semaphore != VK_NULL_HANDLE) {
        BOOST_LOG_TRIVIAL(trace) << "Destroying image available semaphore: " << m_image_available_semaphore;
        vkDestroySemaphore(m_device, m_image_available_semaphore, nullptr);
        m_image_available_semaphore = VK_NULL_HANDLE;
    }

    if (m_device != VK_NULL_HANDLE && m_render_finished_semaphore != VK_NULL_HANDLE) {
        BOOST_LOG_TRIVIAL(trace) << "Destroying render finished semaphore: " << m_render_finished_semaphore;
        vkDestroySemaphore(m_device, m_render_finished_semaphore, nullptr);
        m_render_finished_semaphore = VK_NULL_HANDLE;
    }
}

bool vgraphplay::gfx::System::initDepthResources() {
    VkFormat depth_format = chooseDepthFormat();
    bool brslt = createImage(m_swapchain_extent.width, m_swapchain_extent.height,
                depth_format,
                VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                m_depth_image, m_depth_image_memory);
    if (brslt) {
        BOOST_LOG_TRIVIAL(trace) << "Created depth image: " << m_depth_image << " and memory: " << m_depth_image_memory;
    } else {
        BOOST_LOG_TRIVIAL(error) << "Error creating depth image";
        return false;
    }

    m_depth_image_view = createImageView(m_depth_image, depth_format, VK_IMAGE_ASPECT_DEPTH_BIT);
    if (m_depth_image_view == VK_NULL_HANDLE) {
        BOOST_LOG_TRIVIAL(error) << "Error creating depth image view";
        return false;
    } else {
        BOOST_LOG_TRIVIAL(trace) << "Created depth image view: " << m_depth_image_view;
    }

    transitionImageLayout(m_depth_image, depth_format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

    return true;
}

void vgraphplay::gfx::System::cleanupDepthResources() {
    if (m_device != VK_NULL_HANDLE) {
        if (m_depth_image_view != VK_NULL_HANDLE) {
            BOOST_LOG_TRIVIAL(trace) << "Destroying depth image view: " << m_depth_image_view;
            vkDestroyImageView(m_device, m_depth_image_view, nullptr);
            m_depth_image_view = VK_NULL_HANDLE;
        }

        if (m_depth_image != VK_NULL_HANDLE) {
            BOOST_LOG_TRIVIAL(trace) << "Destroying depth image: " << m_depth_image;
            vkDestroyImage(m_device, m_depth_image, nullptr);
            m_depth_image = VK_NULL_HANDLE;
        }

        if (m_depth_image_memory != VK_NULL_HANDLE) {
            BOOST_LOG_TRIVIAL(trace) << "Freeing depth image memory: " << m_depth_image_memory;
            vkFreeMemory(m_device, m_depth_image_memory, nullptr);
            m_depth_image_memory = VK_NULL_HANDLE;
        }
    }
}

VkFormat vgraphplay::gfx::System::chooseDepthFormat() {
    std::array<VkFormat, 3> candidates{
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT,
    };
    return chooseFormat(candidates.data(), candidates.size(),
                        VK_IMAGE_TILING_OPTIMAL,
                        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

bool vgraphplay::gfx::System::initTextureImage() {
    if (m_texture_image != VK_NULL_HANDLE && m_texture_image_memory != VK_NULL_HANDLE) {
        return true;
    }

    if (m_device == VK_NULL_HANDLE) {
        BOOST_LOG_TRIVIAL(error) << "Things have been initialized out of order. Cannot create texture image";
        return false;
    }

    int tex_width, tex_height, tex_channels;
    stbi_uc *pixels = stbi_load_from_memory(WARREN_TEXTURE.begin(), WARREN_TEXTURE.size(),
                                            &tex_width, &tex_height, &tex_channels,
                                            STBI_rgb_alpha);
    VkDeviceSize tex_size = tex_width * tex_height * 4;

    VkBuffer staging_buffer;
    VkDeviceMemory staging_buffer_memory;
    bool brslt = createBuffer(tex_size,
                              VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                              staging_buffer,
                              staging_buffer_memory);

    if (!brslt) {
        BOOST_LOG_TRIVIAL(error) << "Unable to create staging buffer for texture image";
        return false;
    }

    void *data;
    VkResult rslt = vkMapMemory(m_device, staging_buffer_memory, 0, tex_size, 0, &data);
    if (rslt == VK_SUCCESS) {
        BOOST_LOG_TRIVIAL(trace) << "Mapped memory for staging buffer for texture image: " << staging_buffer_memory;
    } else {
        BOOST_LOG_TRIVIAL(error) << "Unable to map memory for staging buffer for texture image "<< rslt;
        return false;
    }

    std::memcpy(data, pixels, static_cast<size_t>(tex_size));
    vkUnmapMemory(m_device, staging_buffer_memory);
    BOOST_LOG_TRIVIAL(trace) << "Unmapped memory for staging buffer for texture image";
    stbi_image_free(pixels);

    brslt = createImage(static_cast<uint32_t>(tex_width), static_cast<uint32_t>(tex_height),
                        VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
                        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                        m_texture_image, m_texture_image_memory);
    if (brslt) {
        BOOST_LOG_TRIVIAL(trace) << "Created texture image " << m_texture_image << " with memory " << m_texture_image_memory;
    } else {
        BOOST_LOG_TRIVIAL(error) << "Unable to create texture image";
        return false;
    }

    brslt = transitionImageLayout(m_texture_image, VK_FORMAT_R8G8B8A8_UNORM,
                                  VK_IMAGE_LAYOUT_UNDEFINED,
                                  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    if (brslt) {
        BOOST_LOG_TRIVIAL(trace) << "Transitioned texture image " << m_texture_image << " layout for transfer";
    } else {
        BOOST_LOG_TRIVIAL(error) << "Unable to transition image layout";
        return false;
    }

    brslt = copyBufferToImage(staging_buffer, m_texture_image, tex_width, tex_height);
    if (brslt) {
        BOOST_LOG_TRIVIAL(trace) << "Copied staging buffer " << staging_buffer << " to texture image " << m_texture_image;
    } else {
        BOOST_LOG_TRIVIAL(error) << "Unable to copy staging buffer to texture image";
        return false;
    }

    brslt = transitionImageLayout(m_texture_image, VK_FORMAT_R8G8B8A8_UNORM,
                                  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    if (brslt) {
        BOOST_LOG_TRIVIAL(trace) << "Transitioned texture image " << m_texture_image << " layout for sampling";
    } else {
        BOOST_LOG_TRIVIAL(error) << "Unable to transition image layout for sampling";
        return false;
    }

    vkDestroyBuffer(m_device, staging_buffer, nullptr);
    BOOST_LOG_TRIVIAL(trace) << "Destroyed staging buffer for texture image";
    vkFreeMemory(m_device, staging_buffer_memory, nullptr);
    BOOST_LOG_TRIVIAL(trace) << "Freed staging buffer memory for texture image";

    return true;
}

void vgraphplay::gfx::System::cleanupTextureImage() {
    if (m_device != VK_NULL_HANDLE) {
        if (m_texture_image != VK_NULL_HANDLE) {
            BOOST_LOG_TRIVIAL(trace) << "Destroying texture image: " << m_texture_image;
            vkDestroyImage(m_device, m_texture_image, nullptr);
            m_texture_image = VK_NULL_HANDLE;
        }

        if (m_texture_image_memory != VK_NULL_HANDLE) {
            BOOST_LOG_TRIVIAL(trace) << "Freeing texture image memory: " << m_texture_image_memory;
            vkFreeMemory(m_device, m_texture_image_memory, nullptr);
            m_texture_image_memory = VK_NULL_HANDLE;
        }
    }
}

bool vgraphplay::gfx::System::initTextureImageView() {
    if (m_texture_image_view != VK_NULL_HANDLE) {
        return true;
    }

    if (m_device == VK_NULL_HANDLE || m_texture_image == VK_NULL_HANDLE) {
        BOOST_LOG_TRIVIAL(error) << "Things have been initialized out of order. Could not create texture image view.";
        return false;
    }

    m_texture_image_view = createImageView(m_texture_image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);
    if (m_texture_image_view == VK_NULL_HANDLE) {
        BOOST_LOG_TRIVIAL(error) << "Unable to create texture image view";
        return false;
    } else {
        BOOST_LOG_TRIVIAL(trace) << "Created texture image view: " << m_texture_image_view;
    }

    return true;
}

void vgraphplay::gfx::System::cleanupTextureImageView() {
    if (m_device != VK_NULL_HANDLE && m_texture_image_view != VK_NULL_HANDLE) {
        BOOST_LOG_TRIVIAL(trace) << "Destroying texture image view: " << m_texture_image_view;
        vkDestroyImageView(m_device, m_texture_image_view, nullptr);
        m_texture_image_view = VK_NULL_HANDLE;
    }
}

bool vgraphplay::gfx::System::initTextureSampler() {
    if (m_texture_sampler != VK_NULL_HANDLE) {
        return true;
    }

    if (m_device == VK_NULL_HANDLE) {
        BOOST_LOG_TRIVIAL(error) << "Things have been initialized out of order. Cannot create texture sampler.";
        return false;
    }

    VkSamplerCreateInfo smp_ci;
    smp_ci.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    smp_ci.pNext = nullptr;
    smp_ci.flags = 0;
    smp_ci.magFilter = VK_FILTER_LINEAR;
    smp_ci.minFilter = VK_FILTER_LINEAR;
    smp_ci.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    smp_ci.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    smp_ci.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    smp_ci.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    smp_ci.mipLodBias = 0.0;
    smp_ci.anisotropyEnable = VK_TRUE;
    smp_ci.maxAnisotropy = 16.0;
    smp_ci.compareEnable = VK_FALSE;
    smp_ci.compareOp = VK_COMPARE_OP_ALWAYS;
    smp_ci.minLod = 0.0;
    smp_ci.maxLod = 0.0;
    smp_ci.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    smp_ci.unnormalizedCoordinates = VK_FALSE;

    VkResult rslt = vkCreateSampler(m_device, &smp_ci, nullptr, &m_texture_sampler);
    if (rslt == VK_SUCCESS) {
        BOOST_LOG_TRIVIAL(trace) << "Created texture sampler: " << m_texture_sampler;
    } else {
        BOOST_LOG_TRIVIAL(error) << "Unable to create texture sampler " << rslt;
        return false;
    }

    return true;
}

void vgraphplay::gfx::System::cleanupTextureSampler() {
    if (m_device != VK_NULL_HANDLE && m_texture_sampler != VK_NULL_HANDLE) {
        BOOST_LOG_TRIVIAL(trace) << "Destroying texture sampler: " << m_texture_sampler;
        vkDestroySampler(m_device, m_texture_sampler, nullptr);
        m_texture_sampler = VK_NULL_HANDLE;
    }
}

bool vgraphplay::gfx::System::initVertexBuffer() {
    if (m_vertex_buffer != VK_NULL_HANDLE &&
        m_vertex_buffer_memory != VK_NULL_HANDLE)
    {
        return true;
    }

    VkDeviceSize buffer_size = sizeof(RECTANGLE_VERTICES);
    VkBuffer staging_buffer;
    VkDeviceMemory staging_buffer_memory;
    bool rslt_b = createBuffer(buffer_size,
                               VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                               staging_buffer,
                               staging_buffer_memory);

    if (!rslt_b) {
        BOOST_LOG_TRIVIAL(error) << "Unable to create staging buffer for vertex buffer";
        return false;
    }

    void *buffer_data;
    VkResult rslt = vkMapMemory(m_device, staging_buffer_memory, 0, buffer_size, 0, &buffer_data);
    if (rslt == VK_SUCCESS) {
        BOOST_LOG_TRIVIAL(trace) << "Mapped staging buffer memory " << staging_buffer_memory;
    } else {
        BOOST_LOG_TRIVIAL(error) << "Unable to map staging buffer memory " << rslt;
        return false;
    }

    Vertex *vertices = static_cast<Vertex*>(buffer_data);
    std::copy(RECTANGLE_VERTICES, RECTANGLE_VERTICES + NUM_RECTANGLE_VERTICES, vertices);

    vkUnmapMemory(m_device, staging_buffer_memory);
    BOOST_LOG_TRIVIAL(trace) << "Unmapped staging buffer memory " << staging_buffer_memory;

    rslt_b = createBuffer(buffer_size,
                          VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                          m_vertex_buffer,
                          m_vertex_buffer_memory);

    if (!rslt_b) {
        BOOST_LOG_TRIVIAL(error) << "Unable to create vertex buffer";
        return false;
    }

    rslt_b = copyBuffer(staging_buffer, m_vertex_buffer, buffer_size);
    if (!rslt_b) {
        BOOST_LOG_TRIVIAL(error) << "Unable to copy data to vertex buffer";
        return false;
    }

    BOOST_LOG_TRIVIAL(trace) << "Destroying staging buffer: " << staging_buffer;
    vkDestroyBuffer(m_device, staging_buffer, nullptr);
    staging_buffer = VK_NULL_HANDLE;

    BOOST_LOG_TRIVIAL(trace) << "Freeing staging buffer memory: " << staging_buffer_memory;
    vkFreeMemory(m_device, staging_buffer_memory, nullptr);
    staging_buffer_memory = VK_NULL_HANDLE;

    return true;
}

void vgraphplay::gfx::System::cleanupVertexBuffer() {
    if (m_device != VK_NULL_HANDLE) {
        if (m_vertex_buffer != VK_NULL_HANDLE) {
            BOOST_LOG_TRIVIAL(trace) << "Destroying vertex buffer: " << m_vertex_buffer;
            vkDestroyBuffer(m_device, m_vertex_buffer, nullptr);
            m_vertex_buffer = VK_NULL_HANDLE;
        }

        if (m_vertex_buffer_memory != VK_NULL_HANDLE) {
            BOOST_LOG_TRIVIAL(trace) << "Freeing vertex buffer memory: " << m_vertex_buffer_memory;
            vkFreeMemory(m_device, m_vertex_buffer_memory, nullptr);
            m_vertex_buffer_memory = VK_NULL_HANDLE;
        }
    }
}

bool vgraphplay::gfx::System::initIndexBuffer() {
    if (m_index_buffer != VK_NULL_HANDLE && m_index_buffer_memory != VK_NULL_HANDLE) {
        return true;
    }

    if (m_device == VK_NULL_HANDLE) {
        BOOST_LOG_TRIVIAL(error) << "Things have been initialized out of order. Cannot create index buffer.";
        return false;
    }

    VkDeviceSize buffer_size = sizeof(RECTANGLE_INDICES);
    VkBuffer staging_buffer;
    VkDeviceMemory staging_buffer_memory;

    bool brslt = createBuffer(buffer_size,
                              VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                              staging_buffer,
                              staging_buffer_memory);
    if (!brslt) {
        BOOST_LOG_TRIVIAL(error) << "Unable to create staging buffer for the index buffer";
        return false;
    }

    void *buffer_data = nullptr;
    VkResult rslt = vkMapMemory(m_device, staging_buffer_memory, 0, buffer_size, 0, &buffer_data);
    if (rslt == VK_SUCCESS) {
        BOOST_LOG_TRIVIAL(trace) << "Mapped memory for staging buffer for index buffer";
    } else {
        BOOST_LOG_TRIVIAL(error) << "Unable to map memory for staging buffer for index buffer " << rslt;
        return false;
    }

    uint16_t *indices = static_cast<uint16_t*>(buffer_data);
    std::copy(RECTANGLE_INDICES, RECTANGLE_INDICES + NUM_RECTANGLE_INDICES, indices);

    vkUnmapMemory(m_device, staging_buffer_memory);
    BOOST_LOG_TRIVIAL(trace) << "Unmapped staging buffer memory for index buffer";

    brslt = createBuffer(buffer_size,
                         VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                         m_index_buffer,
                         m_index_buffer_memory);
    if (!brslt) {
        BOOST_LOG_TRIVIAL(error) << "Unable to create index buffer";
        return false;
    }

    brslt = copyBuffer(staging_buffer, m_index_buffer, buffer_size);

    if (!brslt) {
        BOOST_LOG_TRIVIAL(error) << "Unable to copy index data to index buffer";
        return false;
    }

    BOOST_LOG_TRIVIAL(trace) << "Destroying staging buffer for index buffer";
    vkDestroyBuffer(m_device, staging_buffer, nullptr);
    staging_buffer = VK_NULL_HANDLE;

    BOOST_LOG_TRIVIAL(trace) << "Freeing staging buffer memory for index buffer";
    vkFreeMemory(m_device, staging_buffer_memory, nullptr);
    staging_buffer_memory = VK_NULL_HANDLE;

    return true;
}

void vgraphplay::gfx::System::cleanupIndexBuffer() {
    if (m_device != VK_NULL_HANDLE) {
        if (m_index_buffer != VK_NULL_HANDLE) {
            BOOST_LOG_TRIVIAL(trace) << "Destroying index buffer: " << m_index_buffer;
            vkDestroyBuffer(m_device, m_index_buffer, nullptr);
            m_index_buffer = VK_NULL_HANDLE;
        }

        if (m_index_buffer_memory != VK_NULL_HANDLE) {
            BOOST_LOG_TRIVIAL(trace) << "Freeing index buffer memory: " << m_index_buffer_memory;
            vkFreeMemory(m_device, m_index_buffer_memory, nullptr);
            m_index_buffer_memory = VK_NULL_HANDLE;
        }
    }
}

bool vgraphplay::gfx::System::initUniformBuffers() {
    if (m_uniform_buffers.size() > 0 && m_uniform_buffers_memory.size() > 0) {
        return true;
    }

    if (m_device == VK_NULL_HANDLE) {
        BOOST_LOG_TRIVIAL(error) << "Things have been initialized out of order. Unable to create uniform buffers.";
        return false;
    }

    int num_buffers = m_swapchain_framebuffers.size();
    VkDeviceSize buffer_size = sizeof(Transormations);
    m_uniform_buffers.resize(num_buffers, VK_NULL_HANDLE);
    m_uniform_buffers_memory.resize(num_buffers, VK_NULL_HANDLE);
    bool brslt = false;
    for (int i = 0; i < num_buffers; ++i) {
        brslt = createBuffer(buffer_size,
                             VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                             m_uniform_buffers[i],
                             m_uniform_buffers_memory[i]);
        if (!brslt) {
            BOOST_LOG_TRIVIAL(error) << "Unable to create uniform buffer";
            return false;
        }
    }

    return true;
}

void vgraphplay::gfx::System::cleanupUniformBuffers() {
    if (m_device != VK_NULL_HANDLE) {
        for (const auto &buffer : m_uniform_buffers) {
            if (buffer != VK_NULL_HANDLE) {
                BOOST_LOG_TRIVIAL(trace) << "Destroying uniform buffer: " << buffer;
                vkDestroyBuffer(m_device, buffer, nullptr);
            }
        }
        m_uniform_buffers.clear();

        for (const auto &memory : m_uniform_buffers_memory) {
            if (memory != VK_NULL_HANDLE) {
                BOOST_LOG_TRIVIAL(trace) << "Freeing uniform buffer memory: " << memory;
                vkFreeMemory(m_device, memory, nullptr);
            }
        }
        m_uniform_buffers_memory.clear();
    }
}

void vgraphplay::gfx::System::updateUniformBuffer(uint32_t current_image) {
    static auto start_time = std::chrono::high_resolution_clock::now();
    auto current_time = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(current_time - start_time).count();
    Transormations xform{};

    xform.model = glm::rotate(glm::mat4x4{1.0f}, time * glm::radians(90.0f), glm::vec3{0.0f, 0.0f, 1.0f});
    xform.view = glm::lookAt(glm::vec3{2.0f, 2.0f, 2.0f}, glm::vec3{0.0f, 0.0f, 0.0f}, glm::vec3{0.0f, 0.0f, 1.0f});
    xform.projection = glm::perspective(glm::radians(45.0f), m_swapchain_extent.width / static_cast<float>(m_swapchain_extent.height), 0.1f, 10.0f);
    xform.projection[1][1] *= -1;

    void *data{nullptr};
    vkMapMemory(m_device, m_uniform_buffers_memory[current_image], 0, sizeof(Transormations), 0, &data);
    Transormations *buf_xform = static_cast<Transormations*>(data);
    std::copy(&xform, &xform + 1, buf_xform);
    vkUnmapMemory(m_device, m_uniform_buffers_memory[current_image]);
}

bool vgraphplay::gfx::System::initDescriptorPool() {
    if (m_descriptor_pool != VK_NULL_HANDLE) {
        return true;
    }

    if (m_device == VK_NULL_HANDLE) {
        BOOST_LOG_TRIVIAL(error) << "Things have been initialized out of order. Could not create descriptor pool.";
        return false;
    }

    std::array<VkDescriptorPoolSize, 2> pool_sizes{};
    pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    pool_sizes[0].descriptorCount = static_cast<uint32_t>(m_swapchain_images.size());
    pool_sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    pool_sizes[1].descriptorCount = static_cast<uint32_t>(m_swapchain_images.size());

    VkDescriptorPoolCreateInfo dp_ci;
    dp_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    dp_ci.pNext = nullptr;
    dp_ci.flags = 0;
    dp_ci.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
    dp_ci.pPoolSizes = pool_sizes.data();
    dp_ci.maxSets = m_swapchain_images.size();

    VkResult rslt = vkCreateDescriptorPool(m_device, &dp_ci, nullptr, &m_descriptor_pool);
    if (rslt == VK_SUCCESS) {
        BOOST_LOG_TRIVIAL(trace) << "Created descriptor pool: " << m_descriptor_pool;
        return true;
    } else {
        BOOST_LOG_TRIVIAL(error) << "Could not create descriptor pool " << rslt;
        return false;
    }
}

void vgraphplay::gfx::System::cleanupDescriptorPool() {
    if (m_device != VK_NULL_HANDLE && m_descriptor_pool != VK_NULL_HANDLE) {
        BOOST_LOG_TRIVIAL(trace) << "Destroying descriptor pool: " << m_descriptor_pool;
        vkDestroyDescriptorPool(m_device, m_descriptor_pool, nullptr);
        m_descriptor_pool = VK_NULL_HANDLE;
    }
}

bool vgraphplay::gfx::System::initDescriptorSets() {
    if (m_descriptor_sets.size() > 0) {
        return true;
    }

    if (m_device == VK_NULL_HANDLE || m_descriptor_pool == VK_NULL_HANDLE) {
        BOOST_LOG_TRIVIAL(error) << "Things have been initialized out of order. Cannot allocate descriptor sets.";
        return false;
    }

    int num_images = m_swapchain_images.size();
    std::vector<VkDescriptorSetLayout> layouts{m_swapchain_images.size(), m_descriptor_set_layout};

    VkDescriptorSetAllocateInfo ds_ai;
    ds_ai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    ds_ai.pNext = nullptr;
    ds_ai.descriptorPool = m_descriptor_pool;
    ds_ai.descriptorSetCount = num_images;
    ds_ai.pSetLayouts = layouts.data();

    m_descriptor_sets.resize(num_images, VK_NULL_HANDLE);
    VkResult rslt = vkAllocateDescriptorSets(m_device, &ds_ai, m_descriptor_sets.data());
    if (rslt == VK_SUCCESS) {
        BOOST_LOG_TRIVIAL(trace) << "Allocated " << num_images << " descriptor sets";
    } else {
        BOOST_LOG_TRIVIAL(error) << "Unable to allocate descriptor sets;";
        return false;
    }

    for (int i = 0; i < num_images; ++i) {
        VkDescriptorBufferInfo dbi;
        dbi.buffer = m_uniform_buffers[i];
        dbi.offset = 0;
        dbi.range = sizeof(Transormations);

        VkDescriptorImageInfo dii;
        dii.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        dii.imageView = m_texture_image_view;
        dii.sampler = m_texture_sampler;

        std::array<VkWriteDescriptorSet, 2> dsc_writes{};

        dsc_writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        dsc_writes[0].pNext = nullptr;
        dsc_writes[0].dstSet = m_descriptor_sets[i];
        dsc_writes[0].dstBinding = 0;
        dsc_writes[0].dstArrayElement = 0;
        dsc_writes[0].descriptorCount = 1;
        dsc_writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        dsc_writes[0].pBufferInfo = &dbi;
        dsc_writes[0].pImageInfo = nullptr;
        dsc_writes[0].pTexelBufferView = nullptr;

        dsc_writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        dsc_writes[1].pNext = nullptr;
        dsc_writes[1].dstSet = m_descriptor_sets[i];
        dsc_writes[1].dstBinding = 1;
        dsc_writes[1].dstArrayElement = 0;
        dsc_writes[1].descriptorCount = 1;
        dsc_writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        dsc_writes[1].pBufferInfo = nullptr;
        dsc_writes[1].pImageInfo = &dii;
        dsc_writes[1].pTexelBufferView = nullptr;

        vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(dsc_writes.size()), dsc_writes.data(), 0, nullptr);
        BOOST_LOG_TRIVIAL(trace) << "Updated descriptor set " << m_descriptor_sets[i];
    }

    return true;
}

void vgraphplay::gfx::System::cleanupDescriptorSets() {
}

uint32_t vgraphplay::gfx::System::chooseMemoryTypeIndex(uint32_t type_filter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties mem_props;
    vkGetPhysicalDeviceMemoryProperties(m_physical_device, &mem_props);

    for (uint32_t i = 0; i < mem_props.memoryTypeCount; ++i) {
        if (type_filter & (1 << i) &&
            (mem_props.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }

    return std::numeric_limits<uint32_t>::max();
}

VkFormat vgraphplay::gfx::System::chooseFormat(const VkFormat *candidates, int num_candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
    for (int i = 0; i < num_candidates; ++i) {
        const VkFormat &format = candidates[i];
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(m_physical_device, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return format;
        } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    return VK_FORMAT_UNDEFINED;
}

bool vgraphplay::gfx::System::hasStencilComponent(VkFormat format) {
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

bool vgraphplay::gfx::System::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags mem_props, VkBuffer &buffer, VkDeviceMemory &memory) {
    if (m_device == VK_NULL_HANDLE) {
        BOOST_LOG_TRIVIAL(error) << "Device has not been initialized. Cannot create buffer";
        return false;
    }

    VkBufferCreateInfo buf_ci;
    buf_ci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buf_ci.pNext = nullptr;
    buf_ci.flags = 0;
    buf_ci.size = size;
    buf_ci.usage = usage;
    buf_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    buf_ci.queueFamilyIndexCount = 0;
    buf_ci.pQueueFamilyIndices = nullptr;

    VkResult rslt = vkCreateBuffer(m_device, &buf_ci, nullptr, &buffer);
    if (rslt == VK_SUCCESS) {
        BOOST_LOG_TRIVIAL(trace) << "Created buffer: " << buffer;
    } else {
        BOOST_LOG_TRIVIAL(error) << "Error creating buffer " << rslt;
        return false;
    }

    VkMemoryRequirements mem_reqs;
    vkGetBufferMemoryRequirements(m_device, buffer, &mem_reqs);
    uint32_t memory_type = chooseMemoryTypeIndex(mem_reqs.memoryTypeBits, mem_props);
    if (memory_type == std::numeric_limits<uint32_t>::max()) {
        BOOST_LOG_TRIVIAL(error) << "No suitable memory type for buffer";
        return false;
    }

    VkMemoryAllocateInfo mem_ai;
    mem_ai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    mem_ai.pNext = nullptr;
    mem_ai.allocationSize = mem_reqs.size;
    mem_ai.memoryTypeIndex = memory_type;

    rslt = vkAllocateMemory(m_device, &mem_ai, nullptr, &memory);
    if (rslt == VK_SUCCESS) {
        BOOST_LOG_TRIVIAL(trace) << "Allocated buffer memory: " << memory;
    } else {
        BOOST_LOG_TRIVIAL(error) << "Error allocating buffer memory " << rslt;
        return false;
    }

    rslt = vkBindBufferMemory(m_device, buffer, memory, 0);
    if (rslt == VK_SUCCESS) {
        BOOST_LOG_TRIVIAL(trace) << "Bound memory allocation " << memory << " to buffer " << buffer;
        return true;
    } else {
        BOOST_LOG_TRIVIAL(error) << "Error binding memory allocation to buffer " << rslt;
        return false;
    }
}

bool vgraphplay::gfx::System::createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage &image, VkDeviceMemory &memory) {
    VkImageCreateInfo img_ci;
    img_ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    img_ci.pNext = nullptr;
    img_ci.flags = 0;
    img_ci.imageType = VK_IMAGE_TYPE_2D;
    img_ci.format = format;
    img_ci.extent.width = width;
    img_ci.extent.height = height;
    img_ci.extent.depth = 1;
    img_ci.mipLevels = 1;
    img_ci.arrayLayers = 1;
    img_ci.samples = VK_SAMPLE_COUNT_1_BIT;
    img_ci.tiling = tiling;
    img_ci.usage = usage;
    img_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    img_ci.queueFamilyIndexCount = 0;
    img_ci.pQueueFamilyIndices = nullptr;
    img_ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VkResult rslt = vkCreateImage(m_device, &img_ci, nullptr, &image);
    if (rslt == VK_SUCCESS) {
        BOOST_LOG_TRIVIAL(trace) << "Created image: " << image;
    } else {
        BOOST_LOG_TRIVIAL(error) << "Unable to create image " << rslt;
        return false;
    }

    VkMemoryRequirements mem_reqs;
    vkGetImageMemoryRequirements(m_device, image, &mem_reqs);

    VkMemoryAllocateInfo mem_ai;
    mem_ai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    mem_ai.pNext = nullptr;
    mem_ai.memoryTypeIndex = chooseMemoryTypeIndex(mem_reqs.memoryTypeBits, properties);
    mem_ai.allocationSize = mem_reqs.size;

    rslt = vkAllocateMemory(m_device, &mem_ai, nullptr, &memory);
    if (rslt == VK_SUCCESS) {
        BOOST_LOG_TRIVIAL(trace) << "Allocated memory for image: " << memory;
    } else {
        BOOST_LOG_TRIVIAL(error) << "Unable to allocate memory for image " << rslt;
        return false;
    }

    rslt = vkBindImageMemory(m_device, image, memory, 0);
    if (rslt == VK_SUCCESS) {
        BOOST_LOG_TRIVIAL(trace) << "Bound memory " << memory << " to image " << image;
    } else {
        BOOST_LOG_TRIVIAL(error) << "Unable to bind memory to image " << rslt;
        return false;
    }

    return true;
}

VkImageView vgraphplay::gfx::System::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspect_mask) {
    VkImageViewCreateInfo iv_ci;
    iv_ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    iv_ci.pNext = nullptr;
    iv_ci.flags = 0;
    iv_ci.image = image;
    iv_ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
    iv_ci.format = format;
    iv_ci.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    iv_ci.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    iv_ci.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    iv_ci.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    iv_ci.subresourceRange.aspectMask = aspect_mask;
    iv_ci.subresourceRange.baseMipLevel = 0;
    iv_ci.subresourceRange.levelCount = 1;
    iv_ci.subresourceRange.baseArrayLayer = 0;
    iv_ci.subresourceRange.layerCount = 1;

    VkImageView iv_rv;
    VkResult rslt = vkCreateImageView(m_device, &iv_ci, nullptr, &iv_rv);
    if (rslt == VK_SUCCESS) {
        BOOST_LOG_TRIVIAL(trace) << "Created image view: " << iv_rv;
    } else {
        BOOST_LOG_TRIVIAL(error) << "Error creating image view " << rslt;
        return VK_NULL_HANDLE;
    }

    return iv_rv;
}

bool vgraphplay::gfx::System::copyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size) {
    VkBufferCopy region;
    region.srcOffset = 0;
    region.dstOffset = 0;
    region.size = size;

    VkCommandBuffer xfer_cb = beginOneTimeCommands();
    if (xfer_cb == VK_NULL_HANDLE) {
        return false;
    }

    vkCmdCopyBuffer(xfer_cb, src, dst, 1, &region);

    return endOneTimeCommands(xfer_cb);
}

bool vgraphplay::gfx::System::copyBufferToImage(VkBuffer src, VkImage dst, uint32_t width, uint32_t height) {
    VkCommandBuffer cb = beginOneTimeCommands();
    if (cb == VK_NULL_HANDLE) {
        return false;
    }

    VkBufferImageCopy bi_cp;
    bi_cp.bufferOffset = 0;
    bi_cp.bufferRowLength = 0;
    bi_cp.bufferImageHeight = 0;
    bi_cp.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    bi_cp.imageSubresource.mipLevel = 0;
    bi_cp.imageSubresource.layerCount = 1;
    bi_cp.imageSubresource.baseArrayLayer = 0;
    bi_cp.imageOffset = { 0, 0, 0 };
    bi_cp.imageExtent = { width, height, 1 };

    vkCmdCopyBufferToImage(cb, src, dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &bi_cp);

    return endOneTimeCommands(cb);
}

bool vgraphplay::gfx::System::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout) {
    VkCommandBuffer cb = beginOneTimeCommands();
    if (cb == VK_NULL_HANDLE) {
        return false;
    }

    VkImageMemoryBarrier barrier;
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.pNext = nullptr;
    barrier.oldLayout = old_layout;
    barrier.newLayout = new_layout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags src_stage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    VkPipelineStageFlags dst_stage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

    if (new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

        if (hasStencilComponent(format)) {
            barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    }

    if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dst_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dst_stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }

    vkCmdPipelineBarrier(cb, src_stage, dst_stage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    return endOneTimeCommands(cb);
}

VkCommandBuffer vgraphplay::gfx::System::beginOneTimeCommands() {
    VkCommandBufferAllocateInfo cb_ai;
    cb_ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cb_ai.pNext = nullptr;
    cb_ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cb_ai.commandPool = m_command_pool;
    cb_ai.commandBufferCount = 1;

    VkCommandBuffer cb_rv{VK_NULL_HANDLE};
    VkResult rslt = vkAllocateCommandBuffers(m_device, &cb_ai, &cb_rv);
    if (rslt == VK_SUCCESS) {
        BOOST_LOG_TRIVIAL(trace) << "Allocated command buffer: " << cb_rv;
    } else {
        BOOST_LOG_TRIVIAL(error) << "Unable to allocate command buffer " << rslt;
        return VK_NULL_HANDLE;
    }

    VkCommandBufferBeginInfo cb_bi;
    cb_bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cb_bi.pNext = nullptr;
    cb_bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    cb_bi.pInheritanceInfo = nullptr;

    vkBeginCommandBuffer(cb_rv, &cb_bi);

    return cb_rv;
}

bool vgraphplay::gfx::System::endOneTimeCommands(VkCommandBuffer commands) {
    VkResult rslt = vkEndCommandBuffer(commands);
    if (rslt != VK_SUCCESS) {
        BOOST_LOG_TRIVIAL(error) << "Error ending recording to command buffer " << rslt;
        return false;
    }

    VkSubmitInfo si;
    si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    si.pNext = nullptr;
    si.waitSemaphoreCount = 0;
    si.pWaitSemaphores = nullptr;
    si.pWaitDstStageMask = nullptr;
    si.commandBufferCount = 1;
    si.pCommandBuffers = &commands;
    si.signalSemaphoreCount = 0;
    si.pSignalSemaphores = nullptr;

    rslt = vkQueueSubmit(m_graphics_queue, 1, &si, VK_NULL_HANDLE);
    if (rslt == VK_SUCCESS) {
        BOOST_LOG_TRIVIAL(trace) << "Submitted command buffer " << commands;
    } else {
        BOOST_LOG_TRIVIAL(error) << "Failed to submit command buffer " << rslt;
        return false;
    }

    vkQueueWaitIdle(m_graphics_queue);
    if (rslt == VK_SUCCESS) {
        BOOST_LOG_TRIVIAL(trace) << "Finished executing command buffer " << commands;
    } else {
        BOOST_LOG_TRIVIAL(error) << "Error waiting for command buffer to complete " << rslt;
        return false;
    }

    vkFreeCommandBuffers(m_device, m_command_pool, 1, &commands);
    return true;
}

VkVertexInputBindingDescription vgraphplay::gfx::Vertex::bindingDescription() {
    VkVertexInputBindingDescription desc;
    desc.binding = 0;
    desc.stride = sizeof(Vertex);
    desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return desc;
}

std::array<VkVertexInputAttributeDescription, 3> vgraphplay::gfx::Vertex::attributeDescription() {
    std::array<VkVertexInputAttributeDescription, 3> descs{};

    descs[0].binding = 0;
    descs[0].location = 0;
    descs[0].offset = offsetof(Vertex, pos);
    descs[0].format = VK_FORMAT_R32G32B32_SFLOAT;

    descs[1].binding = 0;
    descs[1].location = 1;
    descs[1].offset = offsetof(Vertex, color);
    descs[1].format = VK_FORMAT_R32G32B32_SFLOAT;

    descs[2].binding = 0;
    descs[2].location = 2;
    descs[2].offset = offsetof(Vertex, tex);
    descs[2].format = VK_FORMAT_R32G32_SFLOAT;

    return descs;
} */

bool hasExtension(std::vector<vk::ExtensionProperties> &all_extensions, const char *extension_name) {
    return std::ranges::any_of(
        all_extensions,
        [extension_name](auto const &extension) {
            return strcmp(extension.extensionName, extension_name) == 0;
        }
    );
}

bool hasLayer(std::vector<vk::LayerProperties> &all_layers, const char *layer_name) {
    return std::ranges::any_of(
        all_layers,
        [layer_name](auto const &layer) {
            return strcmp(layer.layerName, layer_name) == 0;
        }
    );
}

std::vector<const char *> buildInstanceExtensionList(vk::raii::Context &context, bool debug) {
    std::vector<const char *> rv;
    auto all_extensions = context.enumerateInstanceExtensionProperties();

    uint32_t glfw_extension_count = 0;
    auto glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
    for (uint32_t i = 0; i < glfw_extension_count; ++i) {
        if (hasExtension(all_extensions, glfw_extensions[i])) {
            rv.push_back(glfw_extensions[i]);
        } else {
            throw std::runtime_error("Required instance extension not found: " + std::string{glfw_extensions[i]});
        }
    }

    if (debug) {
        if (hasExtension(all_extensions, vk::EXTDebugUtilsExtensionName)) {
            rv.push_back(vk::EXTDebugUtilsExtensionName);
        } else {
            BOOST_LOG_TRIVIAL(info) << "Vulkan debug utils extension is unavailable";
        }
    }

    #ifdef __APPLE__
    if (hasExtension(all_extensions, vk::KHRPortabilityEnumerationExtensionName)) {
        rv.push_back(vk::KHRPortabilityEnumerationExtensionName);
    } else {
        throw std::runtime_error("Required instance extension not found: " + std::string{vk::KHRPortabilityEnumerationExtensionName});
    }
    #endif

    return rv;
}

std::vector<const char *> buildInstanceLayerList(vk::raii::Context &context, bool debug) {
    std::vector<const char *> rv;

    std::vector<vk::LayerProperties> all_layers = context.enumerateInstanceLayerProperties();
    std::vector<const char *> required_layers{
        "VK_LAYER_KHRONOS_validation"
    };

    for (auto &layer_name : required_layers) {
        if (hasLayer(all_layers, layer_name)) {
            rv.push_back(layer_name);
        } else {
            throw std::runtime_error("Required instance layer not found: " + std::string{layer_name});
        }
    }

    return rv;
}
