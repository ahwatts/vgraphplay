// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#ifndef _VGRAPHPLAY_VGRAPHPLAY_APPLICATION_H_
#define _VGRAPHPLAY_VGRAPHPLAY_APPLICATION_H_

#include "vulkan.h"

#include "gfx/System.h"

namespace vgraphplay {
    class Application {
    public:
        Application(GLFWwindow *window);
        ~Application();

        bool initialize(bool debug);
        void dispose();

        static void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mode);
        void handleKey(int key, int scancode, int action, int mode);

        static void resizeCallback(GLFWwindow *window, int width, int height);
        void handleResize(int width, int height);

        void run();

    private:
        GLFWwindow *m_window;
        gfx::System m_gfx;
        int m_window_width, m_window_height;
    };
}

#endif
