// -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil; -*-

#pragma once
#ifndef _PLANET_VENDOR_EMBED_RESOURCE_H_
#define _PLANET_VENDOR_EMBED_RESOURCE_H_

#include <string>
#include <vector>

#define LOAD_RESOURCE(RESOURCE) ([]() -> const std::vector<unsigned char>& { \
    extern const std::vector<unsigned char> _resource_##RESOURCE;            \
    return _resource_##RESOURCE;                                             \
})()

#endif
