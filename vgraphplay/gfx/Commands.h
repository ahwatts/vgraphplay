// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#ifndef _VGRAPHPLAY_VGRAPHPLAY_GFX_COMMANDS_H_
#define _VGRAPHPLAY_VGRAPHPLAY_GFX_COMMANDS_H_

#include <vector>

#include "../vulkan.h"

namespace vgraphplay {
    namespace gfx {
        class Device;
        
        class CommandQueues {
        public:
            CommandQueues(Device *parent);
            ~CommandQueues();

            bool initialize(uint32_t gq_fam, uint32_t pq_fam);
            void dispose();

            inline uint32_t graphicsQueueFamily() const { return m_graphics_queue_family; }
            inline uint32_t presentQueueFamily() const { return m_present_queue_family; }
            inline VkQueue& graphicsQueue() { return m_graphics_queue; }
            inline VkQueue& presentQueue() { return m_present_queue; }

        protected:
            Device *m_parent;
            uint32_t m_graphics_queue_family;
            VkQueue m_graphics_queue;
            uint32_t m_present_queue_family;
            VkQueue m_present_queue;
        };

        class CommandStore {
        public:
            CommandStore(Device *parent);
            ~CommandStore();

            bool initialize(CommandQueues &queues);
            void dispose();

            inline VkCommandPool& commandPool() { return m_pool; }
            inline std::vector<VkCommandBuffer>& commandBuffers() { return m_buffers; }

        protected:
            Device *m_parent;
            VkCommandPool m_pool;
            std::vector<VkCommandBuffer> m_buffers;
        };

    }
}

#endif
