// stdafx.h: 标准系统包含文件的包含文件，
// 或是经常使用但不常更改的
// 项目特定的包含文件
//

#pragma once

#define WIN32_LEAN_AND_MEAN             // 从 Windows 头文件中排除极少使用的内容
// Windows 头文件
#include <d3d12.h>
#include "d3dx12.h"
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <wrl.h>

#pragma comment(lib, "d3d12")
#pragma comment(lib, "dxgi")
#pragma comment(lib, "d3dcompiler")

using namespace DirectX;
using namespace Microsoft::WRL;

// C++ 运行时头文件
#include <string>
#include <array>
#include <vector>

using namespace std;

struct Utility
{
	static wstring GetModulePath(HMODULE hModule = nullptr)
	{
		wchar_t filename[MAX_PATH];
		GetModuleFileName(hModule, filename, _countof(filename));
		wchar_t *lastSlash = wcsrchr(filename, L'\\');
		if (lastSlash)
		{
			*(lastSlash + 1) = L'\0';
		}
		return { filename };
	}
};
