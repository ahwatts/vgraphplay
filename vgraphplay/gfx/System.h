// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#ifndef _VGRAPHPLAY_VGRAPHPLAY_GFX_SYSTEM_H_
#define _VGRAPHPLAY_VGRAPHPLAY_GFX_SYSTEM_H_

#include "../vulkan.h"

#include "AssetFinder.h"
#include "Device.h"

namespace vgraphplay {
    namespace gfx {
        class CommandQueues;
        class CommandStore;
        class Pipeline;
        class Presentation;
        
        class System {
        public:
            System(GLFWwindow *window, const AssetFinder &asset_finder);
            ~System();

            bool initialize(bool debug);
            void dispose();

            bool recordCommands();
            void drawFrame();

            inline GLFWwindow* window() { return m_window; }
            inline VkInstance& instance() { return m_instance; }
            inline VkSurfaceKHR& surface() { return m_surface; }

            inline AssetFinder& assetFinder() { return m_asset_finder; }
            inline Device& device() { return m_device; }

        protected:
            GLFWwindow *m_window;
            VkInstance m_instance;
            VkDebugReportCallbackEXT m_callback;
            VkSurfaceKHR m_surface;
            
            AssetFinder m_asset_finder;
            Device m_device;
        };
    }
}

#endif
