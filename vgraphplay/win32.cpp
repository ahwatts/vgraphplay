// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#include "win32.h"

#include <iostream>
#include <string>

Win32Context::Win32Context()
    : hInstance{GetModuleHandle(NULL)},
      hWnd{NULL}
{}

Win32Context::~Win32Context() {}

bool Win32Context::initWindow(LONG width, LONG height) {
    std::string class_name = "vgraphplay";
    WNDCLASSEX win_class;
    win_class.cbSize = sizeof(WNDCLASSEX);
    win_class.style = CS_HREDRAW | CS_VREDRAW;
    win_class.lpfnWndProc = WindowProc;
    win_class.cbClsExtra = 0;
    win_class.cbWndExtra = 0;
    win_class.hInstance = GetModuleHandle(NULL);
    win_class.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    win_class.hCursor = LoadCursor(NULL, IDC_ARROW);
    win_class.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    win_class.lpszMenuName = NULL;
    win_class.lpszClassName = class_name.data();
    win_class.hIconSm = LoadIcon(NULL, IDI_WINLOGO);

    if (!RegisterClassEx(&win_class)) {
        std::cerr << "Could not create window class" << std::endl;
        return false;
    }

    RECT wr = {0, 0, width, height};
    AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);
    hWnd = CreateWindowEx(0,
                          class_name.data(),  // class name
                          class_name.data(),  // app name
                          WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_SYSMENU, // window style
                          100, 100,           // x/y coords
                          wr.right - wr.left, // width
                          wr.bottom - wr.top, // height
                          NULL,               // handle to parent
                          NULL,               // handle to menu
                          hInstance,          // hInstance
                          NULL);              // no extra parameters

    if (hWnd != NULL) {
        std::cout << "Created window: " << hWnd << std::endl;
        return true;
    } else {
        std::cerr << "Could not create a window" << std::endl;
        return false;
    }
}

void Win32Context::mainLoop() {
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));
            EndPaint(hWnd, &ps);
        }
        return 0;
    case WM_CLOSE:
        std::cout << "Destroying window " << hWnd << std::endl;
        DestroyWindow(hWnd);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
}
