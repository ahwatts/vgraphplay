// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#include <boost/log/trivial.hpp>

#include "../vulkan.h"

#include "Device.h"
#include "Commands.h"

vgraphplay::gfx::CommandQueues::CommandQueues(Device *parent)
    : m_parent{parent},
      m_graphics_queue_family{UINT32_MAX},
      m_graphics_queue{VK_NULL_HANDLE},
      m_present_queue_family{UINT32_MAX},
      m_present_queue{VK_NULL_HANDLE}
{}

vgraphplay::gfx::CommandQueues::~CommandQueues() {
    dispose();
}

bool vgraphplay::gfx::CommandQueues::initialize(uint32_t gq_fam, uint32_t pq_fam) {
    VkDevice &dev = m_parent->device();

    m_graphics_queue_family = gq_fam;
    vkGetDeviceQueue(dev, m_graphics_queue_family, 0, &m_graphics_queue);
    BOOST_LOG_TRIVIAL(trace) << "Graphics queue: " << m_graphics_queue;

    m_present_queue_family = pq_fam;
    vkGetDeviceQueue(dev, m_present_queue_family, 0, &m_present_queue);
    BOOST_LOG_TRIVIAL(trace) << "Present queue: " << m_present_queue;

    return true;
}

void vgraphplay::gfx::CommandQueues::dispose() {
    // Nothing to do here, yet.
}

vgraphplay::gfx::CommandStore::CommandStore(Device *parent)
    : m_parent{parent},
      m_pool{VK_NULL_HANDLE},
      m_buffers{}
{}

vgraphplay::gfx::CommandStore::~CommandStore() {
    dispose();
}

bool vgraphplay::gfx::CommandStore::initialize(CommandQueues &queues) {
    VkDevice &dev = m_parent->device();

    VkCommandPoolCreateInfo cp_ci;
    cp_ci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cp_ci.pNext = nullptr;
    cp_ci.flags = 0;
    cp_ci.queueFamilyIndex = queues.graphicsQueueFamily();

    VkResult rslt = vkCreateCommandPool(dev, &cp_ci, nullptr, &m_pool);
    if (rslt == VK_SUCCESS) {
        BOOST_LOG_TRIVIAL(trace) << "Created command pool: " << m_pool;
    } else {
        BOOST_LOG_TRIVIAL(error) << "Error creating command pool " << rslt;
        return false;
    }

    size_t num_buffers = m_parent->pipeline().swapchainFramebuffers().size();
    m_buffers.resize(num_buffers);

    VkCommandBufferAllocateInfo cb_ai;
    cb_ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cb_ai.pNext = nullptr;
    cb_ai.commandPool = m_pool;
    cb_ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cb_ai.commandBufferCount = static_cast<uint32_t>(num_buffers);

    rslt = vkAllocateCommandBuffers(dev, &cb_ai, m_buffers.data());

    if (rslt == VK_SUCCESS) {
        BOOST_LOG_TRIVIAL(trace) << "Created " << m_buffers.size() << " command buffers";
    } else {
        BOOST_LOG_TRIVIAL(error) << "Error creating command buffers " << rslt;
        return false;
    }

    return true;
}

void vgraphplay::gfx::CommandStore::dispose() {
    VkDevice &dev = m_parent->device();

    if (dev != VK_NULL_HANDLE && m_pool != VK_NULL_HANDLE) {
        BOOST_LOG_TRIVIAL(trace) << "Destroying command pool: " << m_pool;
        vkDestroyCommandPool(dev, m_pool, nullptr);
    }
}
