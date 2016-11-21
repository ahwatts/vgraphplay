// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#include "Win32.h"

#include <iostream>
#include <string>

#include "Graphics.h"

namespace vgraphplay {
    Win32System::Win32System()
        : System(),
          hInstance{GetModuleHandle(NULL)},
          hWnd{NULL}
    {}

    Win32System::~Win32System() {}

    bool Win32System::initWindow(unsigned int width, unsigned int height) {
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

        RECT wr = {0, 0, static_cast<LONG>(width), static_cast<LONG>(height)};
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

    void Win32System::addExtensionNames(std::vector<const char *> &extensions) {
        extensions.emplace_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
    }

    bool Win32System::initSurface(Graphics &graphics) {
        VkWin32SurfaceCreateInfoKHR create_info;
        create_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        create_info.pNext = nullptr;
        create_info.flags = 0;
        create_info.hinstance = hInstance;
        create_info.hwnd = hWnd;

        VkResult rslt = vkCreateWin32SurfaceKHR(graphics.instance, &create_info, nullptr, &graphics.surface);
        if (rslt == VK_SUCCESS) {
            std::cout << "Surface created: " << graphics.surface << std::endl;
            return true;
        } else {
            std::cerr << "Error: " << rslt << " surface = " << graphics.surface << std::endl;
            return false;
        }
    }

    void Win32System::mainLoop() {
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
            std::cout << "Destroying window in response to WM_CLOSE: " << hWnd << std::endl;
            DestroyWindow(hWnd);
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        case WM_KEYDOWN:
            switch (wParam) {
            case VK_ESCAPE:
                std::cout << "Destroying window in response to WM_KEYDOWN with VK_ESCAPE: " << hWnd << std::endl;
                DestroyWindow(hWnd);
                return 0;
            }
        default:
            return DefWindowProc(hWnd, uMsg, wParam, lParam);
        }
    }
}
