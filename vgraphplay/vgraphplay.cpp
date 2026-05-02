// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#include <memory>
#include <print>

#include <boost/log/trivial.hpp>

#include "vulkan.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "Application.h"

using namespace vgraphplay;

void initGLFW(int width, int height, const char *title, GLFWwindow **window);
void handleGLFWError(int code, const char *desc);
void bailout(const std::string &msg);

const int WIDTH = 1024;
const int HEIGHT = 768;

int main(int argc, char **argv) {
    GLFWwindow *window;
    initGLFW(WIDTH, HEIGHT, "VGraphplay", &window);

    try {
        Application app{window, true};
        app.run();
    } catch (const std::exception &e) {
        std::println(stderr, "Error running application: {}", e.what());
    }
    
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

    if (!*window) {
        bailout("Could not create window");
    }
}

void handleGLFWError(int code, const char *desc) {
    std::println(stderr, "GLFW error code {}\n{}", code, desc);
}

void bailout(const std::string &msg) {
    std::println(stderr, "{}", msg);
    glfwTerminate();
    std::exit(1);
}
