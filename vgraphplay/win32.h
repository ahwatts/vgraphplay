// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#ifndef _VGRAPHPLAY_VGRAPHPLAY_WIN32_H_
#define _VGRAPHPLAY_VGRAPHPLAY_WIN32_H_

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>

class Win32Context {
public:
    Win32Context();
    ~Win32Context();

    bool initWindow(LONG width, LONG height);
    void mainLoop();

    HINSTANCE hInstance;
    HWND hWnd;
};

LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

#endif
