#pragma once

// 目标平台: Windows 10 (1809+) - 确保Win10兼容性
#ifndef WINVER
#define WINVER 0x0A00           // Windows 10
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00     // Windows 10
#endif

// WinRT C++/WinRT 兼容性设置
#ifndef WINRT_LEAN_AND_MEAN
#define WINRT_LEAN_AND_MEAN
#endif

#define WIN32_LEAN_AND_MEAN             // 从 Windows 头文件中排除极少使用的内容

// Windows 头文件
#include <windows.h>
