// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"
#include "ErrorHandler.h"


BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        // 初始化日志系统
        sdk::Logger::Initialize(
            sdk::LogLevel::INFO,    // 最小级别：INFO（生产环境）
            false,                    // 启用控制台输出
            true,                   // 禁用文件输出
            "sdk.log"               // 日志文件路径
        );
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        // 关闭日志
        sdk::Logger::Shutdown();
        break;
    }
    return TRUE;
}