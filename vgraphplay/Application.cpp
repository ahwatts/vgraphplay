// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#include <iostream>

#include "vulkan.h"

#include "Application.h"
#include "gfx/System.h"

vgraphplay::Application::Application(GLFWwindow *window)
  : m_window{window},
    m_gfx{window},
    m_window_width{0},
    m_window_height{0}
{
    glfwGetFramebufferSize(m_window, &m_window_width, &m_window_height);
    glfwSetWindowUserPointer(window, this);
    glfwSetKeyCallback(m_window, vgraphplay::Application::keyCallback);
    glfwSetFramebufferSizeCallback(m_window, vgraphplay::Application::resizeCallback);
}

vgraphplay::Application::~Application() {
    dispose();
}

bool vgraphplay::Application::initialize(bool debug) {
    bool rv = m_gfx.initialize(debug);
    return rv;
}

void vgraphplay::Application::dispose() {
    m_gfx.dispose();
}

void vgraphplay::Application::keyCallback(GLFWwindow *window, int key, int scancode, int action, int mode) {
    Application *app = (Application*)glfwGetWindowUserPointer(window);
    if (app != nullptr) {
        app->handleKey(key, scancode, action, mode);
    }
}

void vgraphplay::Application::handleKey(int key, int scancode, int action, int mode) {
    switch (key) {
    case GLFW_KEY_ESCAPE:
        glfwSetWindowShouldClose(m_window, GLFW_TRUE);
        break;
    default:
        std::cout << "key: " << key
                  << " scancode: " << scancode
                  << " action: " << action
                  << " mode: " << mode
                  << std::endl;
    }
}

void vgraphplay::Application::resizeCallback(GLFWwindow *window, int width, int height) {
    Application *app = (Application*)glfwGetWindowUserPointer(window);
    if (app != nullptr) {
        app->handleResize(width, height);
    }
}

void vgraphplay::Application::handleResize(int width, int height) {
    m_window_width = width;
    m_window_height = height;
    // reset the graphics for the new dimensions.
}

void vgraphplay::Application::run() {
    while (!glfwWindowShouldClose(m_window)) {
        glfwPollEvents();
        m_gfx.drawFrame();
    }
}
