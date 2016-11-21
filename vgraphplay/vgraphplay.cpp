// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#include <memory>

#include "Graphics.h"
#include "System.h"

#ifdef _WIN32
#include "Win32.h"
#endif

using namespace vgraphplay;

int main(int argc, char **argv) {
    Graphics gfx;
    std::unique_ptr<System> sys;

#ifdef _WIN32
    sys.reset(dynamic_cast<System*>(new Win32System()));
#endif

    sys->initWindow(1024, 768);
    gfx.initialize(sys.get());
    sys->mainLoop();

    return 0;
}
