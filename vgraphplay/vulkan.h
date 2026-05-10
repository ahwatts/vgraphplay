// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#ifndef _VGRAPHPLAY_VGRAPHPLAY_VULKAN_H_
#define _VGRAPHPLAY_VGRAPHPLAY_VULKAN_H_

#ifdef __APPLE__
// On Apple, enable this to gain access to
// VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME and its attendant C++ equivalents.
#define VK_ENABLE_BETA_EXTENSIONS
#endif

#if defined(__INTELLISENSE__) || !defined(USE_CPP_20_MODULES)
#include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif

#endif
