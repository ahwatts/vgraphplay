// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#include <iostream>
#include <memory>

#include <boost/filesystem.hpp>
#define BOOST_LOG_DYN_LINK
#include <boost/log/trivial.hpp>

#include "vulkan.h"

#include "AssetFinder.h"
#include "Graphics.h"

using namespace vgraphplay;
using namespace boost::filesystem;

void initGLFW(int width, int height, const char *title, GLFWwindow **window);
void handleGLFWError(int code, const char *desc);
void bailout(const std::string &msg);

void keypress(GLFWwindow *wnd, int key, int scancode, int action, int mods);

static std::unique_ptr<gfx::System> GFX{nullptr};
const int WIDTH = 1024;
const int HEIGHT = 768;

int main(int argc, char **argv) {
    GLFWwindow *window;
    initGLFW(WIDTH, HEIGHT, "VGraphplay", &window);

    path bin_path(argv[0]);
    bin_path.remove_filename();
    AssetFinder finder(bin_path);

    GFX.reset(new gfx::System(window, finder));
    GFX->initialize();

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }

    delete GFX.release();
    glfwTerminate();
    return 0;
}

void initGLFW(int width, int height, const char *title, GLFWwindow **window) {
    glfwSetErrorCallback(handleGLFWError);
    if (!glfwInit()) {
        bailout("Could not initialize GLFW!");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    *window = glfwCreateWindow(width, height, title, nullptr, nullptr);

    glfwSetKeyCallback(*window, keypress);

    if (!*window) {
        bailout("Could not create window");
    }
}

void handleGLFWError(int code, const char *desc) {
    std::cerr << "GLFW Error Code " << code << std::endl
              << desc << std::endl;
}

void bailout(const std::string &msg) {
    std::cerr << msg << std::endl;
    glfwTerminate();
    std::exit(1);
}

void keypress(GLFWwindow *wnd, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        switch (key) {
        case GLFW_KEY_ESCAPE:
            glfwSetWindowShouldClose(wnd, true);
            break;
        default:
            std::cout << "key: " << key
                      << " scancode: " << scancode
                      << " action: " << action
                      << " mods: " << mods
                      << std::endl;
        }
    }
}
