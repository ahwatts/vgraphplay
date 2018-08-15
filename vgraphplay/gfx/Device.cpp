// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#include <set>
#include <string>
#include <vector>

#include <boost/log/trivial.hpp>

#include "../vulkan.h"

#include "Commands.h"
#include "Device.h"
#include "Pipeline.h"
#include "Presentation.h"
#include "System.h"
#include "VulkanOutput.h"

struct ChosenDeviceInfo {
    VkPhysicalDevice dev;
    uint32_t graphics_queue_family;
    uint32_t present_queue_family;
};

ChosenDeviceInfo choosePhysicalDevice(std::vector<VkPhysicalDevice> &devices, VkSurfaceKHR &surface);

vgraphplay::gfx::Device::Device(System *parent)
    : m_device{VK_NULL_HANDLE},
      m_physical_device{VK_NULL_HANDLE},
      m_image_available_semaphore{VK_NULL_HANDLE},
      m_render_finished_semaphore{VK_NULL_HANDLE},
      m_parent{parent},
      m_queues{this},
      m_commands{this},
      m_present{this},
      m_pipeline{this}
{}

vgraphplay::gfx::Device::~Device() {
    dispose();
}

bool vgraphplay::gfx::Device::initialize() {
    if (m_device != VK_NULL_HANDLE) {
        return true;
    }

    VkInstance &inst = m_parent->instance();
    VkSurfaceKHR &surf = m_parent->surface();

    if (inst == VK_NULL_HANDLE || surf == VK_NULL_HANDLE) {
        BOOST_LOG_TRIVIAL(error) << "Things have been initialized out of order. Cannot create device.";
        return false;
    }

    logPhysicalDevices(inst);

    uint32_t num_devices;
    vkEnumeratePhysicalDevices(inst, &num_devices, nullptr);
    std::vector<VkPhysicalDevice> devices(num_devices);
    vkEnumeratePhysicalDevices(inst, &num_devices, devices.data());

    ChosenDeviceInfo chosen = choosePhysicalDevice(devices, surf);

    if (chosen.dev == VK_NULL_HANDLE) {
        return false;
    }

    m_physical_device = chosen.dev;

    BOOST_LOG_TRIVIAL(trace) << "physical device = " << m_physical_device
                             << " graphics queue family = " << chosen.graphics_queue_family
                             << " present queue family = " << chosen.present_queue_family;

    float queue_priority = 1.0;
    std::vector<VkDeviceQueueCreateInfo> queue_cis;
    VkDeviceQueueCreateInfo queue_ci;
    queue_ci.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_ci.pNext = nullptr;
    queue_ci.flags = 0;
    queue_ci.queueFamilyIndex = chosen.graphics_queue_family;
    queue_ci.queueCount = 1;
    queue_ci.pQueuePriorities = &queue_priority;
    queue_cis.push_back(queue_ci);

    if (chosen.present_queue_family != chosen.graphics_queue_family) {
        VkDeviceQueueCreateInfo queue_ci;
        queue_ci.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_ci.pNext = nullptr;
        queue_ci.flags = 0;
        queue_ci.queueFamilyIndex = chosen.present_queue_family;
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
    device_ci.queueCreateInfoCount = (uint32_t)queue_cis.size();
    device_ci.pQueueCreateInfos = queue_cis.data();
    device_ci.enabledLayerCount = 0;
    device_ci.ppEnabledLayerNames = nullptr;
    device_ci.pEnabledFeatures = nullptr;
    device_ci.enabledExtensionCount = (uint32_t)extension_names.size();
    device_ci.ppEnabledExtensionNames = extension_names.data();

    VkResult rslt = vkCreateDevice(m_physical_device, &device_ci, nullptr, &m_device);

    if (rslt == VK_SUCCESS) {
        BOOST_LOG_TRIVIAL(trace) << "Device created: " << m_device;
    } else {
        BOOST_LOG_TRIVIAL(error) << "Error creating device " << rslt;
        return false;
    }

    VkSemaphoreCreateInfo sem_ci;
    sem_ci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    sem_ci.pNext = nullptr;
    sem_ci.flags = 0;

    rslt = vkCreateSemaphore(m_device, &sem_ci, nullptr, &m_image_available_semaphore);
    if (rslt == VK_SUCCESS) {
        BOOST_LOG_TRIVIAL(trace) << "Created image available semaphore: " << m_image_available_semaphore;
    } else {
        BOOST_LOG_TRIVIAL(error) << "Error creating image available semaphore: " << rslt;
        return false;
    }

    rslt = vkCreateSemaphore(m_device, &sem_ci, nullptr, &m_render_finished_semaphore);
    if (rslt == VK_SUCCESS) {
        BOOST_LOG_TRIVIAL(trace) << "Created render finished semaphore: " << m_render_finished_semaphore;
    } else {
        BOOST_LOG_TRIVIAL(error) << "Error creating render finished semaphore: " << rslt;
        return false;
    }

    return m_queues.initialize(chosen.graphics_queue_family, chosen.present_queue_family)&&
        m_present.initialize() &&
        m_pipeline.initialize() &&
        m_commands.initialize(m_queues);
}

void vgraphplay::gfx::Device::dispose() {
    m_commands.dispose();
    m_pipeline.dispose();
    m_present.dispose();
    m_queues.dispose();

    if (m_device != VK_NULL_HANDLE && m_image_available_semaphore != VK_NULL_HANDLE) {
        BOOST_LOG_TRIVIAL(trace) << "Destroying image available semaphore: " << m_image_available_semaphore;
        vkDestroySemaphore(m_device, m_image_available_semaphore, nullptr);
    }

    if (m_device != VK_NULL_HANDLE && m_render_finished_semaphore != VK_NULL_HANDLE) {
        BOOST_LOG_TRIVIAL(trace) << "Destroying render finished semaphore: " << m_render_finished_semaphore;
        vkDestroySemaphore(m_device, m_render_finished_semaphore, nullptr);
    }

    if (m_device != VK_NULL_HANDLE) {
        BOOST_LOG_TRIVIAL(trace) << "Destroying device: " << m_device;
        vkDestroyDevice(m_device, nullptr);
        m_device = VK_NULL_HANDLE;
    }

    // No need to actually destroy the rest of these; they're
    // managed by Vulkan.
    m_physical_device = VK_NULL_HANDLE;
}

ChosenDeviceInfo choosePhysicalDevice(std::vector<VkPhysicalDevice> &devices, VkSurfaceKHR &surface) {
    const uint32_t MAX_INT = std::numeric_limits<uint32_t>::max();
    
    for (auto &dev : devices) {
        // Do we have queue families suitable for graphics / presentation?
        uint32_t graphics_queue = MAX_INT, present_queue = MAX_INT, num_queue_families = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(dev, &num_queue_families, nullptr);
        std::vector<VkQueueFamilyProperties> queue_families{num_queue_families};
        vkGetPhysicalDeviceQueueFamilyProperties(dev, &num_queue_families, queue_families.data());

        for (uint32_t id = 0; id < queue_families.size(); ++id) {
            if (graphics_queue == MAX_INT && queue_families[id].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                graphics_queue = id;
            }

            if (present_queue == MAX_INT) {
                VkBool32 supports_present = false;
                vkGetPhysicalDeviceSurfaceSupportKHR(dev, id, surface, &supports_present);
                if (supports_present) {
                    present_queue = id;
                }
            }
        }

        if (graphics_queue == MAX_INT && present_queue == MAX_INT) {
            continue;
        }

        // Are the extensions / layers we want supported?
        uint32_t num_extensions = 0;
        vkEnumerateDeviceExtensionProperties(dev, nullptr, &num_extensions, nullptr);
        std::vector<VkExtensionProperties> extensions{num_extensions};
        vkEnumerateDeviceExtensionProperties(dev, nullptr, &num_extensions, extensions.data());

        std::set<std::string> required_extensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };

        for (const auto &extension : extensions) {
            required_extensions.erase(extension.extensionName);
        }

        if (!required_extensions.empty()) {
            continue;
        }
        
        // Are swapchains supported, and are there surface formats / present modes we can use?
        uint32_t num_formats = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(dev, surface, &num_formats, nullptr);
        uint32_t num_present_modes = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(dev, surface, &num_present_modes, nullptr);

        if (num_formats > 0 && num_present_modes > 0) {
            return {
                dev,
                graphics_queue,
                present_queue,
            };
        }
    }

    return {
        VK_NULL_HANDLE,
        MAX_INT,
        MAX_INT,
    };
}
