#include "stdafx.h"

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
	default:
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}

int __stdcall wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
	WNDCLASSEX wc = { sizeof(WNDCLASSEX) };
	wc.style = CS_VREDRAW | CS_HREDRAW;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.hbrBackground = CreateSolidBrush(RGB(255, 255, 255));
	wc.lpszMenuName = nullptr;
	wc.lpszClassName = L"HelloDirect3D";
	wc.hIconSm = nullptr;

	RegisterClassEx(&wc);

	LONG clientWidth = 640; LONG clientHeight = 480;
	DWORD style = WS_OVERLAPPEDWINDOW; DWORD styleEx = 0;
	RECT rc = { 0,0,clientWidth,clientHeight };
	AdjustWindowRectEx(&rc, style, false, styleEx);
	auto cx = GetSystemMetrics(SM_CXSCREEN);
	auto cy = GetSystemMetrics(SM_CYFULLSCREEN);
	auto w = rc.right - rc.left;
	auto h = rc.bottom - rc.top;
	auto x = (cx - w) / 2;
	auto y = (cy - h) / 2;

	HWND hWnd = CreateWindowEx(WS_EX_NOREDIRECTIONBITMAP, wc.lpszClassName, L"Direct3D 12≥ı ºªØ", WS_OVERLAPPEDWINDOW,
		x, y, w, h, nullptr, nullptr, hInstance, nullptr);

	ShowWindow(hWnd, nCmdShow);
	MSG msg = {};
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return static_cast<int>(msg.wParam);
}
