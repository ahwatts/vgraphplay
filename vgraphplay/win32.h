// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#ifndef _VGRAPHPLAY_VGRAPHPLAY_WIN32_H_
#define _VGRAPHPLAY_VGRAPHPLAY_WIN32_H_

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>

#include "System.h"

namespace vgraphplay {
    class Graphics;

    class Win32System : public System {
    public:
        Win32System();
        virtual ~Win32System();

        virtual bool initWindow(unsigned int width, unsigned int height) override;
        virtual void addExtensionNames(std::vector<const char *> &extensions) override;
        virtual bool initSurface(Graphics &graphics) override;
        virtual void mainLoop() override;

        HINSTANCE hInstance;
        HWND hWnd;
    };

    LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
}

#endif
