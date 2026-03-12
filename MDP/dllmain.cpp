// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include "ErrorHandler.h"
#include "bt/BLEComm.h"
#include <winrt/base.h>

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        // Initialize the robust logging system
        sdk::Logger::Initialize(
            sdk::LogLevel::INFO,    // Debug level: output all logs
            false,                    // Enable console output
            true,                    // Enable file output
            "sdk.log"                // Log file path
        );

        // Initialize WinRT (MANDATORY for BLE communication)
        try {
            winrt::init_apartment();
            sdk::Logger::Log(sdk::LogLevel::INFO, sdk::ErrorCategory::GENERAL, "WinRT initialized successfully");
        }
        catch (const std::exception& e) {
            sdk::Logger::Error(sdk::ErrorCategory::GENERAL, std::string("Failed to initialize WinRT: ") + e.what());
            return FALSE;
        }
        
        sdk::Logger::Info("MDP SDK Loaded");
        break;
        
    case DLL_THREAD_ATTACH:
        break;
        
    case DLL_THREAD_DETACH:
        break;
        
    case DLL_PROCESS_DETACH:
        // Gracefully disconnect all connected BLE devices before shutting down WinRT
        CloseAllBLEDevices();

        // Shutdown logging subsystem
        sdk::Logger::Info("MDP SDK Unloading");
        sdk::Logger::Shutdown();
        
        // Uninitialize WinRT gracefully
        winrt::uninit_apartment();
        break;
    }
    return TRUE;
}
