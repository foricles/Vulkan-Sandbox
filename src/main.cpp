#include <Windows.h>
#include <chrono>
#include "vulkan/vkengine.hpp"
#include "app/app.hpp"




LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow)
{
#ifdef _DEBUG
    FILE* stream;
    AllocConsole();
    freopen_s(&stream, "CONOUT$", "w", stdout);
#endif // _DEBUG


    const wchar_t CLASS_NAME[] = L"VulkanBlur";

    WNDCLASS wc = { };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        L"Blur",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (hwnd == NULL)
    {
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);


    MSG msg = { };
    bool isRunning = true;
    double deltaTime = 0;
    do
    {
        const auto start_clock = std::chrono::high_resolution_clock::now();

        while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            isRunning &= (msg.message != WM_QUIT);
        }

        if (isRunning)
        {
            App::Get().Update(float(deltaTime));
            App::Get().Render();
        }


        wchar_t title[256]{ 0 };
        swprintf_s(title, L"Vulkan blur. CPU: %.2lfms   GPU: %.2lfms   FPS: %.2lf", deltaTime * 1000.0, App::Get().GetGputTime(), 1.f / deltaTime);
        SetWindowText(hwnd, title);


        const auto end_clock = std::chrono::high_resolution_clock::now();
        deltaTime = std::chrono::duration<double, std::chrono::seconds::period>(end_clock - start_clock).count();

    } while (isRunning);

    DestroyWindow(hwnd);
    UnregisterClass(CLASS_NAME, hInstance);

    return 0;
}

#include <windowsx.h>
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_CREATE:
    {

#ifdef _DEBUG
        const VulkanEngine::list layers = { "VK_LAYER_KHRONOS_validation" };
        const VulkanEngine::list extensions = { VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME, VK_EXT_DEBUG_UTILS_EXTENSION_NAME };
#else
        const VulkanEngine::list layers = { };
        const VulkanEngine::list extensions = { VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME };
#endif // _DEBUG

        VulkanEngine::list devextensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
            VK_KHR_MAINTENANCE_4_EXTENSION_NAME,
            VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
            VK_EXT_SHADER_DEMOTE_TO_HELPER_INVOCATION_EXTENSION_NAME,
            VK_KHR_SEPARATE_DEPTH_STENCIL_LAYOUTS_EXTENSION_NAME
        };

#ifdef _DEBUG
        const VulkanEngine::list devlayers = { };
#else
        const VulkanEngine::list devlayers = { };
#endif // _DEBUG

        VulkanEngine::InitInstance(extensions, layers);
        VulkanEngine::InitSurface(hwnd, GetModuleHandle(NULL), devextensions, devlayers);

        App::Get().Init();
        break;
    }
    case WM_SIZE:
    {
        VulkanEngine::UpdateSwapchain(LOWORD(lParam), HIWORD(lParam));
        App::Get().OnWidowResize(LOWORD(lParam), HIWORD(lParam));
        break;
    }
    case WM_SYSKEYDOWN:
    case WM_KEYDOWN:
    {
        App::Get().Input.OnKeyDown(uint32_t(wParam));
        break;
    }
    case WM_SYSKEYUP:
    case WM_KEYUP:
    {
        App::Get().Input.OnKeyUp(uint32_t(wParam));
        break;
    }
    case WM_DESTROY:
    {
        App::Get().Shutdown();
            VulkanEngine::Shutdown();
            PostQuitMessage(0);
            break;
        }
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}