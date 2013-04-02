#pragma once

#ifndef GENERAL_H

#define WIN32_LEAN_AND_MEAN

#include "targetver.h"

#include <Windows.h>
#include <Ole2.h>
#include <Commctrl.h>
#include <Shlwapi.h>
#include <Shlobj.h>
#include <strsafe.h>
#include <Shellapi.h>
#include <Filter.h>
#include "DoubleList.h"

#define QWORD UINT64

#ifndef _DEBUG
#define _ASSERT(x) x
#define _ASSERTE(x) x
#endif

//DEBUG FUNCTIONS
#ifdef _DEBUG
#include <crtdbg.h>

#define new new(_NORMAL_BLOCK,  __FILE__, __LINE__)
#define malloc(size) _malloc_dbg(size, _NORMAL_BLOCK,  __FILE__, __LINE__)

#endif//_DEBUG

inline BOOL DisplayError(DWORD dwError = 0)
{
	if (dwError == 0) dwError = GetLastError();

	if (dwError)
	{
		WCHAR wsError[500];
		WCHAR* wsDestEnd;
		StringCchPrintfExW(wsError, 500, &wsDestEnd, NULL, 0, L"Error 0x%08X: ", dwError);

		FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, 0, dwError, 0, wsDestEnd, 500, 0);
		
		MessageBox(0, wsError, 0, MB_ICONERROR);
		return TRUE;
	}
	return FALSE;
}

#ifdef _UNICODE
#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
#endif

#endif//GENERAL_H