// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#include <iostream>
#include <memory>

#include <boost/log/trivial.hpp>

#include "vulkan.h"

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

    Application app{window};
    if (app.initialize(true)) {
        app.run();
    }
    app.dispose();

    glfwTerminate();
    return 0;
}

void initGLFW(int width, int height, const char *title, GLFWwindow **window) {
    glfwSetErrorCallback(handleGLFWError);
    if (!glfwInit()) {
        bailout("Could not initialize GLFW!");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    // glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
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
    glfwTerminate();
    std::exit(1);
}
