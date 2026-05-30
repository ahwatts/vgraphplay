// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#include <chrono>
#include <cassert>
#include <cstring>
#include <format>
#include <utility>
#include <vector>

#include <boost/log/trivial.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "../glm.h"
#include "../vulkan.h"

#include "System.h"

bool hasExtension(std::vector<vk::ExtensionProperties> &all_extensions, const char *extension_name);
bool hasLayer(std::vector<vk::LayerProperties> &all_layers, const char *layer_name);
std::vector<const char *> buildInstanceExtensionList(vk::raii::Context &context, bool debug);
std::vector<const char *> buildInstanceLayerList(vk::raii::Context &context, bool debug);

vk::raii::PhysicalDevice choosePhysicalDevice(const std::vector<vk::raii::PhysicalDevice> &devices /*, vk::SurfaceKHR &surface */);
vk::SurfaceFormatKHR chooseSwapchainSurfaceFormat(const std::vector<vk::SurfaceFormatKHR> &available_formats);
vk::PresentModeKHR chooseSwapchainPresentMode(const std::vector<vk::PresentModeKHR> &available_modes);
vk::Extent2D chooseSwapchainExtent(GLFWwindow *window, const vk::SurfaceCapabilitiesKHR &caps);
uint32_t chooseMemoryTypeIndex(vk::raii::PhysicalDevice &physical_device, uint32_t type_filter, vk::MemoryPropertyFlags properties);

const std::vector<unsigned char> &UNLIT_BYTECODE = LOAD_RESOURCE(unlit_slang_spv);
const std::vector<unsigned char> &WARREN_TEXTURE = LOAD_RESOURCE(warren_jpg);

// const uint16_t NUM_RECTANGLE_VERTICES = 8;
// const vgraphplay::gfx::Vertex RECTANGLE_VERTICES[NUM_RECTANGLE_VERTICES] = {
//     {{-0.5f, -0.5f,  0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
//     {{ 0.5f, -0.5f,  0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
//     {{ 0.5f,  0.5f,  0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
//     {{-0.5f,  0.5f,  0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},

//     {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
//     {{ 0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
//     {{ 0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
//     {{-0.5f,  0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
// };

// const uint16_t NUM_RECTANGLE_INDICES = 12;
// const uint16_t RECTANGLE_INDICES[NUM_RECTANGLE_INDICES] = {
//     0, 1, 2, 2, 3, 0,
//     4, 5, 6, 6, 7, 4,
// };

// const std::vector<vgraphplay::gfx::Vertex> VERTICES = {
//     {{ 0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
//     {{ 0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}},
//     {{-0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}}
// };

const std::vector<vgraphplay::gfx::Vertex> RECTANGLE_VERTICES = {
    {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    {{ 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
    {{ 0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}},
    {{-0.5f,  0.5f}, {1.0f, 1.0f, 1.0f}},
};

const std::vector<uint32_t> RECTANGLE_INDICES = {
    0, 1, 2, 2, 3, 0,
};

vk::VertexInputBindingDescription vgraphplay::gfx::Vertex::bindingDescription() {
    return vk::VertexInputBindingDescription{
        .binding = 0,
        .stride = sizeof(Vertex),
        .inputRate = vk::VertexInputRate::eVertex,
    };
}

std::array<vk::VertexInputAttributeDescription, 2> vgraphplay::gfx::Vertex::attributeDescription() {
    return {
        vk::VertexInputAttributeDescription{
            .location = 0,
            .binding = 0,
            .format = vk::Format::eR32G32Sfloat,
            .offset = offsetof(Vertex, pos),
        },
        vk::VertexInputAttributeDescription{
            .location = 1,
            .binding = 0,
            .format = vk::Format::eR32G32B32Sfloat,
            .offset = offsetof(Vertex, color),
        },
    };
}

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
      m_vertex_buffer{nullptr},
      m_index_buffer{nullptr},
      m_uniform_buffers{},
      m_vertex_buffer_memory{nullptr},
      m_index_buffer_memory{nullptr},
      m_uniform_buffers_memory{},
      m_uniform_buffers_mapped{},
      m_texture_image{nullptr},
      m_texture_image_memory{nullptr},
      m_texture_image_view{nullptr},
      m_texture_sampler{nullptr},
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
      m_descriptor_set_layout{nullptr},
      m_descriptor_pool{nullptr},
      m_descriptor_sets{},
      m_present_complete_semaphores{},
      m_render_finished_semaphores{},
      m_draw_fences{}
    //   m_texture_sampler{VK_NULL_HANDLE},
    //   m_depth_image{VK_NULL_HANDLE},
    //   m_depth_image_memory{VK_NULL_HANDLE},
    //   m_depth_image_view{VK_NULL_HANDLE},
{
    initInstance();
    initDebugMessenger();
    initSurface();
    initDevice();
    initSwapchain();
    initSynchronizationObjects();
    initDescriptorSetLayout();
    initPipeline();
    initCommandPool();
    initCommandBuffer();
    initDescriptorPool();
    initVertexBuffer();
    initIndexBuffer();
    initUniformBuffers();
    initTextureImage();
    initTextureImageView();
    initTextureSampler();
    initDescriptorSets();
}

vgraphplay::gfx::System::~System() {
    // Wait until things are finished before destructing the chirren.
    if (m_command_queue != nullptr) {
        m_command_queue.waitIdle();
    }
}

void vgraphplay::gfx::System::initInstance() {
    if (m_instance != nullptr) {
        return;
    }

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

    assert(m_instance != nullptr);

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
        bool supports_vulkan_13 = props.apiVersion >= vk::ApiVersion13;

        const std::vector<vk::QueueFamilyProperties> queue_families = dev.getQueueFamilyProperties();
        bool supports_graphics = std::ranges::any_of(
            queue_families,
            [](const auto &qfp) { return !!(qfp.queueFlags & vk::QueueFlagBits::eGraphics); }
        );

        const std::vector<vk::ExtensionProperties> all_extensions = dev.enumerateDeviceExtensionProperties();
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

        const auto features = dev.getFeatures2<
            vk::PhysicalDeviceFeatures2,
            vk::PhysicalDeviceVulkan11Features,
            vk::PhysicalDeviceVulkan13Features, 
            vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT
        >();
        bool supports_required_features =
            features.get<vk::PhysicalDeviceFeatures2>().features.samplerAnisotropy &&
            features.get<vk::PhysicalDeviceVulkan11Features>().shaderDrawParameters &&
            features.get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering &&
            features.get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState;

        if (supports_vulkan_13 && 
            supports_graphics && 
            supports_all_extensions && 
            supports_required_features)
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

    assert(m_instance != nullptr);
    assert(m_surface != nullptr);

    // logPhysicalDevices(m_instance);
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
        vk::PhysicalDeviceFeatures2{
            .features = vk::PhysicalDeviceFeatures{
                .samplerAnisotropy = true, // Enable ansiotropic filtering in samplers
            },
        },
        vk::PhysicalDeviceVulkan11Features{
            /*.shaderDrawParameters = true*/ // Enable shader draw parameters (we need this for SV_VertexID in the shader)
        },
        vk::PhysicalDeviceVulkan13Features{
            .synchronization2 = true, // Support new synchronization commands
            .dynamicRendering = true, // Enable dynamic rendering from Vulkan 1.3
        },     
        vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT{
            .extendedDynamicState = true // Enable extended dynamic state from the extension
        },
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

    assert(m_instance != nullptr);

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

    assert(m_physical_device != nullptr);
    assert(m_surface != nullptr);
    assert(m_device != nullptr);
    
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
    for (auto &image : m_swapchain_images) {
        m_swapchain_image_views.push_back(createImageView(
            image,
            m_swapchain_format.format,
            vk::ImageAspectFlagBits::eColor
        ));
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

std::pair<vk::raii::Buffer, vk::raii::DeviceMemory> vgraphplay::gfx::System::createBuffer(
    vk::DeviceSize size, 
    vk::BufferUsageFlags usage, 
    vk::MemoryPropertyFlags properties, 
    std::optional<const char *> name
) {
    assert(m_physical_device != nullptr);
    assert(m_device != nullptr);

    std::string buffer_name;
    if (name.has_value()) {
        buffer_name = std::string{name.value()} + " buffer";
    } else {
        buffer_name = "buffer";
    }

    vk::BufferCreateInfo buf_ci{
        .size = size,
        .usage = usage,
        .sharingMode = vk::SharingMode::eExclusive,
    };
    vk::raii::Buffer buffer = m_device.createBuffer(buf_ci);
    BOOST_LOG_TRIVIAL(trace) << "Created " << buffer_name <<": " << *buffer;

    vk::MemoryRequirements mem_reqs = buffer.getMemoryRequirements();
    uint32_t memory_type = chooseMemoryTypeIndex(
        m_physical_device,
        mem_reqs.memoryTypeBits,
        properties
    );

    vk::MemoryAllocateInfo mem_ai{
        .allocationSize = mem_reqs.size,
        .memoryTypeIndex = memory_type,
    };
    vk::raii::DeviceMemory memory = m_device.allocateMemory(mem_ai);
    BOOST_LOG_TRIVIAL(trace) << "Allocated memory for " << buffer_name << ": " << *memory;
    buffer.bindMemory(memory, 0);

    return {std::move(buffer), std::move(memory)};
}

void vgraphplay::gfx::System::copyBuffer(vk::raii::Buffer &src, vk::raii::Buffer &dst, vk::DeviceSize size) {
    vk::raii::CommandBuffer cmds = beginOneTimeCommands();
    cmds.copyBuffer(*src, *dst, vk::BufferCopy{0, 0, size});
    endOneTimeCommands(std::move(cmds));
}

void vgraphplay::gfx::System::initVertexBuffer() {
    vk::DeviceSize vertex_buffer_size = sizeof(decltype(RECTANGLE_VERTICES)::value_type) * RECTANGLE_VERTICES.size();

    auto [staging_buffer, staging_buffer_memory] = createBuffer(
        vertex_buffer_size,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
        "vertex staging"
    );

    void *data = staging_buffer_memory.mapMemory(0, vertex_buffer_size);
    std::memcpy(data, RECTANGLE_VERTICES.data(), vertex_buffer_size);
    staging_buffer_memory.unmapMemory();

    std::tie(m_vertex_buffer, m_vertex_buffer_memory) = createBuffer(
        vertex_buffer_size,
        vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        "vertex"
    );

    copyBuffer(staging_buffer, m_vertex_buffer, vertex_buffer_size);
}

void vgraphplay::gfx::System::initIndexBuffer() {
    vk::DeviceSize index_buffer_size = sizeof(decltype(RECTANGLE_INDICES)::value_type) * RECTANGLE_INDICES.size();

    auto [staging_buffer, staging_buffer_memory] = createBuffer(
        index_buffer_size,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
        "index staging"
    );

    void *data = staging_buffer_memory.mapMemory(0, index_buffer_size);
    std::memcpy(data, RECTANGLE_INDICES.data(), index_buffer_size);
    staging_buffer_memory.unmapMemory();

    std::tie(m_index_buffer, m_index_buffer_memory) = createBuffer(
        index_buffer_size,
        vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        "index"
    );

    copyBuffer(staging_buffer, m_index_buffer, index_buffer_size);
}

void vgraphplay::gfx::System::initUniformBuffers() {
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        vk::DeviceSize buffer_size = sizeof(Transformations);
        auto [buffer, buffer_memory] = createBuffer(
            buffer_size,
            vk::BufferUsageFlagBits::eUniformBuffer,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
            "uniform"
        );

        m_uniform_buffers.emplace_back(std::move(buffer));
        m_uniform_buffers_memory.emplace_back(std::move(buffer_memory));
        m_uniform_buffers_mapped.emplace_back(m_uniform_buffers_memory.back().mapMemory(0, buffer_size));
    }
}

void vgraphplay::gfx::System::initDescriptorSetLayout() {
    assert(m_device != nullptr);

    vk::DescriptorSetLayoutBinding dsl_lb{
        .binding = 0,
        .descriptorType = vk::DescriptorType::eUniformBuffer,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eVertex,
    };

    vk::DescriptorSetLayoutCreateInfo dsl_ci = vk::DescriptorSetLayoutCreateInfo{}.setBindings(dsl_lb);
    m_descriptor_set_layout = m_device.createDescriptorSetLayout(dsl_ci);
    BOOST_LOG_TRIVIAL(trace) << "Created descriptor set layout: " << *m_descriptor_set_layout;
}

void vgraphplay::gfx::System::initDescriptorPool() {
    vk::DescriptorPoolSize dp_sz{
        .type = vk::DescriptorType::eUniformBuffer,
        .descriptorCount = MAX_FRAMES_IN_FLIGHT,
    };
    vk::DescriptorPoolCreateInfo dp_ci = vk::DescriptorPoolCreateInfo{
        .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        .maxSets = MAX_FRAMES_IN_FLIGHT,
    }.setPoolSizes(dp_sz);
    m_descriptor_pool = m_device.createDescriptorPool(dp_ci);
    BOOST_LOG_TRIVIAL(trace) << "Created descriptor pool: " << *m_descriptor_pool;
}

void vgraphplay::gfx::System::initDescriptorSets() {
    std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, *m_descriptor_set_layout);
    vk::DescriptorSetAllocateInfo ds_ai = vk::DescriptorSetAllocateInfo{
        .descriptorPool = *m_descriptor_pool,
    }.setSetLayouts(layouts);
    m_descriptor_sets = m_device.allocateDescriptorSets(ds_ai);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        BOOST_LOG_TRIVIAL(trace) << "Allocated descriptor set " << i << ": " << *m_descriptor_sets[i];
        vk::DescriptorBufferInfo buffer_info{
            .buffer = *m_uniform_buffers[i],
            .offset = 0,
            .range = sizeof(Transformations),
        };
        vk::WriteDescriptorSet ds_write = vk::WriteDescriptorSet{
            .dstSet = *m_descriptor_sets[i],
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorType = vk::DescriptorType::eUniformBuffer,
        }.setBufferInfo(buffer_info);
        m_device.updateDescriptorSets(ds_write, {});
    }
}

void vgraphplay::gfx::System::initPipeline() {
    assert(m_device != nullptr);
    
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

    vk::VertexInputBindingDescription vertex_binding_info = Vertex::bindingDescription();
    std::array<vk::VertexInputAttributeDescription, 2> vertex_attribute_info = Vertex::attributeDescription();
    vk::PipelineVertexInputStateCreateInfo vertex_input_ci = vk::PipelineVertexInputStateCreateInfo{}
        .setVertexBindingDescriptions(vertex_binding_info)
        .setVertexAttributeDescriptions(vertex_attribute_info);

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
        .frontFace = vk::FrontFace::eCounterClockwise,
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

    vk::PipelineLayoutCreateInfo layout_ci = vk::PipelineLayoutCreateInfo{
        .setLayoutCount = 0,
        .pushConstantRangeCount = 0,
    }.setSetLayouts(*m_descriptor_set_layout);
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

void vgraphplay::gfx::System::updateUniformBuffer(uint32_t frame_index) {
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    Transformations xforms;
    xforms.model = glm::rotate(
        glm::mat4(1.0f), 
        time * glm::radians(90.0f), 
        glm::vec3(0.0f, 0.0f, 1.0f)
    );
    xforms.view = glm::lookAt(
        glm::vec3(2.0f, 2.0f, 2.0f), 
        glm::vec3(0.0f, 0.0f, 0.0f), 
        glm::vec3(0.0f, 0.0f, 1.0f)
    );
    xforms.projection = glm::perspective(
        glm::radians(45.0f),
        static_cast<float>(m_swapchain_extent.width) / static_cast<float>(m_swapchain_extent.height),
        0.01f,
        10.0f
    );
    xforms.projection[1][1] *= -1;
    std::memcpy(m_uniform_buffers_mapped[frame_index], &xforms, sizeof(xforms));
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
    command_buffer.bindVertexBuffers(0, *m_vertex_buffer, {0});
    command_buffer.bindIndexBuffer(*m_index_buffer, {0}, vk::IndexType::eUint32);
    command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pipeline_layout, 0, *m_descriptor_sets[m_frame_index], nullptr);
    command_buffer.setViewport(0, vk::Viewport{0.0f, 0.0f, static_cast<float>(m_swapchain_extent.width), static_cast<float>(m_swapchain_extent.height)});
    command_buffer.setScissor(0, vk::Rect2D{vk::Offset2D{0, 0}, m_swapchain_extent});
    command_buffer.drawIndexed(static_cast<uint32_t>(RECTANGLE_INDICES.size()), 1, 0, 0, 0);

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

vk::raii::CommandBuffer vgraphplay::gfx::System::beginOneTimeCommands() {
    vk::CommandBufferAllocateInfo cb_ai{
        .commandPool = *m_command_pool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = 1,
    };
    vk::raii::CommandBuffer cb = std::move(m_device.allocateCommandBuffers(cb_ai).front());

    vk::CommandBufferBeginInfo cb_bi{
        .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
    };
    cb.begin(cb_bi);

    return std::move(cb);    
}

void vgraphplay::gfx::System::endOneTimeCommands(vk::raii::CommandBuffer &&commands) {
    commands.end();
    vk::SubmitInfo si = vk::SubmitInfo{}.setCommandBuffers(*commands);
    m_command_queue.submit(si, nullptr);
    m_command_queue.waitIdle();
} 

void vgraphplay::gfx::System::initSynchronizationObjects() {
    assert(m_device != nullptr);
    
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

std::pair<vk::raii::Image, vk::raii::DeviceMemory> vgraphplay::gfx::System::createImage(
    uint32_t width, uint32_t height,
    vk::Format format,
    vk::ImageTiling tiling,
    vk::ImageUsageFlags usage,
    vk::MemoryPropertyFlags properties
) {
    vk::ImageCreateInfo img_ci{
        .imageType = vk::ImageType::e2D,
        .format = format,
        .extent = {.width = width, .height = height, .depth = 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = vk::SampleCountFlagBits::e1,
        .tiling = tiling,
        .usage = usage,
        .sharingMode = vk::SharingMode::eExclusive,
    };
    vk::raii::Image image = m_device.createImage(img_ci);
    BOOST_LOG_TRIVIAL(trace) << "Created image: " << *image;

    vk::MemoryRequirements mem_reqs = image.getMemoryRequirements();
    vk::MemoryAllocateInfo mem_ai{
        .allocationSize = mem_reqs.size,
        .memoryTypeIndex = chooseMemoryTypeIndex(m_physical_device, mem_reqs.memoryTypeBits, properties),
    };
    vk::raii::DeviceMemory image_memory = m_device.allocateMemory(mem_ai);
    BOOST_LOG_TRIVIAL(trace) << "Allocated memory " << *image_memory << " for image: " << *image;
    image.bindMemory(image_memory, 0);

    return {std::move(image), std::move(image_memory)};
}

vk::raii::ImageView vgraphplay::gfx::System::createImageView(
    const vk::Image image, 
    vk::Format format, 
    vk::ImageAspectFlags aspect_mask
) {
    vk::ImageViewCreateInfo imgv_ci{
        .image = image,
        .viewType = vk::ImageViewType::e2D,
        .format = format,
        .components = {
            vk::ComponentSwizzle::eIdentity,
            vk::ComponentSwizzle::eIdentity,
            vk::ComponentSwizzle::eIdentity,
            vk::ComponentSwizzle::eIdentity,
        },
        .subresourceRange = { aspect_mask, 0, 1, 0, 1 },
    };

    vk::raii::ImageView rv = m_device.createImageView(imgv_ci);
    BOOST_LOG_TRIVIAL(trace) << "Created image view: " << *rv << " image " << image;
    return rv;
} 

void vgraphplay::gfx::System::transitionImageLayout(
    vk::raii::CommandBuffer &command_buffer,
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

    command_buffer.pipelineBarrier2(dep);
}

void vgraphplay::gfx::System::copyBufferToImage(
    vk::raii::CommandBuffer &command_buffer,
    const vk::Buffer buffer,
    const vk::Image image,
    uint32_t width,
    uint32_t height
) {
    vk::BufferImageCopy bi_cp{
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource = {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
        .imageOffset = {0, 0, 0},
        .imageExtent = {width, height, 1},
    };
    command_buffer.copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, bi_cp);
}

void vgraphplay::gfx::System::initTextureImage() {
    assert(m_device != nullptr);

    int tex_width, tex_height, tex_channels;
    stbi_uc *pixels = stbi_load_from_memory(
        WARREN_TEXTURE.data(), WARREN_TEXTURE.size(),
        &tex_width, &tex_height, &tex_channels,
        STBI_rgb_alpha
    );
    vk::DeviceSize tex_size = tex_width * tex_height * 4;
    assert(pixels != nullptr);

    auto [staging_buffer, staging_buffer_memory] = createBuffer(
        tex_size,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
        "texture staging"
    );

    void *data = staging_buffer_memory.mapMemory(0, tex_size);
    std::memcpy(data, pixels, tex_size);
    staging_buffer_memory.unmapMemory();
    stbi_image_free(pixels);

    std::tie(m_texture_image, m_texture_image_memory) = createImage(
        tex_width, tex_height,
        vk::Format::eR8G8B8A8Srgb,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
        vk::MemoryPropertyFlagBits::eDeviceLocal
    );

    vk::raii::CommandBuffer cmds = beginOneTimeCommands();
    transitionImageLayout(
        cmds,
        *m_texture_image,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eTransferDstOptimal,
        {},
        vk::AccessFlagBits2::eTransferWrite,
        vk::PipelineStageFlagBits2::eTopOfPipe,
        vk::PipelineStageFlagBits2::eTransfer
    );
    copyBufferToImage(cmds, *staging_buffer, *m_texture_image, tex_width, tex_height);
    transitionImageLayout(
        cmds,
        *m_texture_image,
        vk::ImageLayout::eTransferDstOptimal,
        vk::ImageLayout::eShaderReadOnlyOptimal,
        vk::AccessFlagBits2::eTransferWrite,
        vk::AccessFlagBits2::eShaderRead,
        vk::PipelineStageFlagBits2::eTransfer,
        vk::PipelineStageFlagBits2::eFragmentShader
    );
    endOneTimeCommands(std::move(cmds));
}

void vgraphplay::gfx::System::initTextureImageView() {
    m_texture_image_view = createImageView(
        *m_texture_image,
        vk::Format::eR8G8B8A8Srgb,
        vk::ImageAspectFlagBits::eColor
    );
}

void vgraphplay::gfx::System::initTextureSampler() {
    assert(m_device != nullptr);

    vk::PhysicalDeviceProperties props = m_physical_device.getProperties();

    vk::SamplerCreateInfo smp_ci{
        .magFilter = vk::Filter::eLinear,
        .minFilter = vk::Filter::eLinear,
        .mipmapMode = vk::SamplerMipmapMode::eLinear,
        .addressModeU = vk::SamplerAddressMode::eRepeat,
        .addressModeV = vk::SamplerAddressMode::eRepeat,
        .addressModeW = vk::SamplerAddressMode::eRepeat,
        .anisotropyEnable = vk::True,
        .maxAnisotropy = props.limits.maxSamplerAnisotropy,
        .compareEnable = vk::False,
        .compareOp = vk::CompareOp::eAlways,
        .borderColor = vk::BorderColor::eIntOpaqueBlack,
        .unnormalizedCoordinates = vk::False,
    };

    m_texture_sampler = m_device.createSampler(smp_ci);
    BOOST_LOG_TRIVIAL(trace) << "Created texture sampler: " << *m_texture_sampler;
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
    updateUniformBuffer(m_frame_index);
    
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

/* bool vgraphplay::gfx::System::initDepthResources() {
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

uint32_t chooseMemoryTypeIndex(vk::raii::PhysicalDevice &physical_device, uint32_t type_filter, vk::MemoryPropertyFlags properties) {
    vk::PhysicalDeviceMemoryProperties mem_props = physical_device.getMemoryProperties();
    
    for (uint32_t i = 0; i < mem_props.memoryTypeCount; ++i) {
        if (type_filter & (1 << i) &&
            (mem_props.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;            
        }
    }

    throw std::runtime_error{"Unable to find suitable memory type"};
}
