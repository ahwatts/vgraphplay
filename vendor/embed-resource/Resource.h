// -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil; -*-

#pragma once
#ifndef _PLANET_VENDOR_EMBED_RESOURCE_H_
#define _PLANET_VENDOR_EMBED_RESOURCE_H_

#include <string>

class Resource {
public:
    Resource(const unsigned char* start, const size_t len) : resource_data(start), data_len(len) {}

    const unsigned char * const &data() const { return resource_data; }
    const size_t &size() const { return data_len; }

    const unsigned char *begin() const { return resource_data; }
    const unsigned char *end() const { return resource_data + data_len; }

    std::string toString() { return std::string(reinterpret_cast<const char *>(data()), size()); }

private:
    const unsigned char* resource_data;
    const size_t data_len;
};

#define LOAD_RESOURCE(RESOURCE) ([]() {                                    \
        extern const unsigned char _resource_##RESOURCE[];               \
        extern const size_t _resource_##RESOURCE##_len;                  \
        return Resource(_resource_##RESOURCE, _resource_##RESOURCE##_len); \
    })()

#endif
