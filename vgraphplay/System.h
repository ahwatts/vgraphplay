// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#ifndef _VGRAPHPLAY_VGRAPHPLAY_SYSTEM_H_
#define _VGRAPHPLAY_VGRAPHPLAY_SYSTEM_H_

#include <vector>

namespace vgraphplay {
    class Graphics;

    class System {
    public:
        System();
        virtual ~System();
        virtual bool initWindow(unsigned int width, unsigned int height) = 0;
        virtual void addExtensionNames(std::vector<const char *> &extensions) = 0;
        virtual bool initSurface(Graphics &gfx) = 0;
        virtual void mainLoop() = 0;
    };
}

#endif
