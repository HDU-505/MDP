// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"
#include "ErrorHandler.h"
#include <winrt/base.h>

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        // 初始化WinRT (必需！用于BLE通信)
        try {
            winrt::init_apartment();
            std::cout << "WinRT initialized successfully" << std::endl;
        }
        catch (const std::exception& e) {
            std::cerr << "Failed to initialize WinRT: " << e.what() << std::endl;
            return FALSE;
        }
        
        // 初始化日志系统
        sdk::Logger::Initialize(
            sdk::LogLevel::DEBUG,    // 调试级别：显示所有日志
            false,                    // 启用控制台输出
            true,                    // 启用文件输出
            "sdk.log"                // 日志文件路径
        );
        
        sdk::Logger::Info("MDP SDK Loaded");
        break;
        
    case DLL_THREAD_ATTACH:
        break;
        
    case DLL_THREAD_DETACH:
        break;
        
    case DLL_PROCESS_DETACH:
        // 关闭日志
        sdk::Logger::Info("MDP SDK Unloading");
        sdk::Logger::Shutdown();
        
        // 反初始化WinRT
        winrt::uninit_apartment();
        break;
    }
    return TRUE;
}