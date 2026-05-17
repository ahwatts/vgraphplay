// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#ifndef _VGRAPHPLAY_VGRAPHPLAY_VULKAN_H_
#define _VGRAPHPLAY_VGRAPHPLAY_VULKAN_H_

// On Apple, enable this to gain access to
// VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME and its attendant C++ equivalents.
#ifdef __APPLE__
#define VK_ENABLE_BETA_EXTENSIONS
#endif

// We need this so that vk::Result::eErrorOutOfDateKHR gets returned instead of 
// throwing an exception.
#define VULKAN_HPP_HANDLE_ERROR_OUT_OF_DATE_AS_SUCCESS 1

// Include the Vulkan C++ RAII bindings. If C++20 modules are available, use
// those.
#if defined(__INTELLISENSE__) || !defined(USE_CPP_20_MODULES)
#include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif

// GLFW has to be included *after* Vulkan. The Vulkan RAII include and the C++20
// module will both ultimately include the main vulkan.h header, so this
// include will work.
#include <GLFW/glfw3.h>

#endif
