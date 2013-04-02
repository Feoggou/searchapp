#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <strsafe.h>

#define CLASSNAME L"SimpleWndClass"

WCHAR wsFilePath[MAX_PATH];

LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_HOTKEY:
		{
			ShellExecuteW(hWnd, L"open", wsFilePath, 0, 0, 1);
		}
		break;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	//create a window:
	WNDCLASS wndclass = {0};
	wndclass.hInstance = hInstance;
	wndclass.lpfnWndProc = WindowProc;
	wndclass.lpszClassName = CLASSNAME;
	
	if (0 == RegisterClassW(&wndclass))
	{
		DWORD dwError = GetLastError();
		WCHAR wsError[256];
		StringCchPrintfW(wsError, 256, L"Could not register SearchApp Invoker window. Error 0x%08X", dwError);
		MessageBox(0, wsError, 0, 0);
		return 0;
	}

	HWND hWnd = CreateWindowW(CLASSNAME, 0, 0, 0,0 , 0, 0, 0, 0, hInstance, 0);
	if (NULL == hWnd)
	{
		DWORD dwError = GetLastError();
		WCHAR wsError[256];
		StringCchPrintfW(wsError, 256, L"Could not create SearchApp Invoker window. Error 0x%08X", dwError);
		MessageBox(0, wsError, 0, 0);
		return 0;
	}

	MSG msg = {0};

	if (false == RegisterHotKey(hWnd, 0, MOD_WIN, 'S'))
	{
		DWORD dwError = GetLastError();
		WCHAR wsError[256];
		StringCchPrintfW(wsError, 256, L"Could not create SearchApp Invoker window. Error 0x%08X", dwError);
		MessageBox(0, wsError, 0, 0);
		return 0;
	}

	HKEY hKey;
	DWORD dwError = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Software", 0, KEY_READ, &hKey);
	if (dwError != 0)
	{
		WCHAR wsError[256];
		StringCchPrintfW(wsError, 256, L"Could not create SearchApp Invoker window. Error 0x%08X", dwError);
		MessageBox(0, wsError, 0, 0);
		return 0;
	}

	HKEY hKey2 = hKey;

	dwError = RegOpenKeyExW(hKey2, L"Microsoft", 0, KEY_READ, &hKey);
	if (dwError != 0)
	{
		WCHAR wsError[256];
		StringCchPrintfW(wsError, 256, L"Could not create SearchApp Invoker window. Error 0x%08X", dwError);
		MessageBox(0, wsError, 0, 0);
		return 0;
	}
	RegCloseKey(hKey2);

	hKey2 = hKey;
	dwError = RegOpenKeyExW(hKey2, L"Windows", 0, KEY_READ, &hKey);
	if (dwError != 0)
	{
		WCHAR wsError[256];
		StringCchPrintfW(wsError, 256, L"Could not create SearchApp Invoker window. Error 0x%08X", dwError);
		MessageBox(0, wsError, 0, 0);
		return 0;
	}
	RegCloseKey(hKey2);

	hKey2 = hKey;
	dwError = RegOpenKeyExW(hKey2, L"CurrentVersion", 0, KEY_READ, &hKey);
	if (dwError != 0)
	{
		WCHAR wsError[256];
		StringCchPrintfW(wsError, 256, L"Could not create SearchApp Invoker window. Error 0x%08X", dwError);
		MessageBox(0, wsError, 0, 0);
		return 0;
	}
	RegCloseKey(hKey2);

	hKey2 = hKey;
	dwError = RegOpenKeyExW(hKey2, L"App Paths", 0, KEY_READ, &hKey);
	if (dwError != 0)
	{
		WCHAR wsError[256];
		StringCchPrintfW(wsError, 256, L"Could not create SearchApp Invoker window. Error 0x%08X", dwError);
		MessageBox(0, wsError, 0, 0);
		return 0;
	}
	RegCloseKey(hKey2);

	DWORD cbMaxPath = MAX_PATH;
	dwError = RegGetValueW(hKey, L"SearchApp.exe", 0, RRF_RT_REG_SZ, 0, (void*)wsFilePath, &cbMaxPath);
	if (dwError != 0)
	{
		WCHAR wsError[256];
		StringCchPrintfW(wsError, 256, L"Could not create SearchApp Invoker window. Error 0x%08X", dwError);
		MessageBox(0, wsError, 0, 0);
		return 0;
	}
	RegCloseKey(hKey);

	int nResult;//0x5b = VK_LWIN
	while (msg.message != WM_QUIT)
	{
		nResult = PeekMessage(&msg, 0, 0, 0, 1);
		if (nResult)
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			Sleep(50);
		}
	}

	return 0;
}