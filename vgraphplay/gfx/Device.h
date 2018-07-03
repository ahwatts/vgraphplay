// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#ifndef _VGRAPHPLAY_VGRAPHPLAY_GFX_DEVICE_H_
#define _VGRAPHPLAY_VGRAPHPLAY_GFX_DEVICE_H_

#include "../vulkan.h"

#include "Commands.h"
#include "Pipeline.h"
#include "Presentation.h"

namespace vgraphplay {
    namespace gfx {
        class System;
        
        class Device {
        public:
            Device(System *parent);
            ~Device();

            bool initialize();
            void dispose();

            VkPhysicalDevice choosePhysicalDevice(const std::vector<VkPhysicalDevice> &devices);
            uint32_t chooseGraphicsQueueFamily(VkPhysicalDevice &device, const std::vector<VkQueueFamilyProperties> &families);
            uint32_t choosePresentQueueFamily(VkPhysicalDevice &device, const std::vector<VkQueueFamilyProperties> &families, VkSurfaceKHR &surf);

            inline VkDevice& device() { return m_device; }
            inline VkPhysicalDevice& physicalDevice() { return m_physical_device; }
            inline VkSemaphore& imageAvailableSemaphore() { return m_image_available_semaphore; }
            inline VkSemaphore& renderFinishedSemaphore() { return m_render_finished_semaphore; }

            inline System& parent() { return *m_parent; }
            inline CommandQueues& queues() { return m_queues; }
            inline CommandStore& commandStore() { return m_commands; }
            inline Presentation& presentation() { return m_present; }
            inline Pipeline& pipeline() { return m_pipeline; }

        protected:
            VkDevice m_device;
            VkPhysicalDevice m_physical_device;
            VkSemaphore m_image_available_semaphore;
            VkSemaphore m_render_finished_semaphore;

            System *m_parent;
            CommandQueues m_queues;
            CommandStore m_commands;
            Presentation m_present;
            Pipeline m_pipeline;
        };
    }
}

#endif
