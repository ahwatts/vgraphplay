// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#include <iostream>
#include <memory>

#include "vulkan.h"

#include "Graphics.h"

using namespace vgraphplay;

void initGLFW(int width, int height, const char *title, GLFWwindow **window);
void handleGLFWError(int code, const char *desc);
void bailout(const std::string &msg);

static Graphics GFX;
const int WIDTH = 1024;
const int HEIGHT = 768;

int main(int argc, char **argv) {
    GLFWwindow *window;
    initGLFW(WIDTH, HEIGHT, "VGraphplay", &window);

    GFX.initialize(window);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }

    GFX.shutDown();
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
    std::cerr << "GLFW Error Code " << code << std::endl
              << desc << std::endl;
}

void bailout(const std::string &msg) {
    std::cerr << msg << std::endl;
    GFX.shutDown();
    glfwTerminate();
    std::exit(1);
}
