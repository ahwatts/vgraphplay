// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#include <boost/log/trivial.hpp>

#include "../vulkan.h"

#include "AssetFinder.h"
#include "Commands.h"
#include "Device.h"
#include "Pipeline.h"
#include "Presentation.h"
#include "System.h"
#include "VulkanOutput.h"

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
    
    return m_queues.initialize(graphics_queue_family, present_queue_family) &&
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

VkPhysicalDevice vgraphplay::gfx::Device::choosePhysicalDevice(const std::vector<VkPhysicalDevice> &devices) {
    // We probably want to make a better decision than this...
    return devices[0];
}

uint32_t vgraphplay::gfx::Device::chooseGraphicsQueueFamily(VkPhysicalDevice &device, const std::vector<VkQueueFamilyProperties> &families) {
    for (uint32_t i = 0; i < families.size(); ++i) {
        if (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            return i;
        }
    }
    return UINT32_MAX;
}

uint32_t vgraphplay::gfx::Device::choosePresentQueueFamily(VkPhysicalDevice &device, const std::vector<VkQueueFamilyProperties> &families, VkSurfaceKHR &surf) {
    for (uint32_t i = 0; i < families.size(); ++i) {
        VkBool32 supports_present = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surf, &supports_present);
        if (supports_present) {
            return i;
        }
    }
    return UINT32_MAX;
}
