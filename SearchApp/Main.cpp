#include "General.h"
#include "SearchAppDlg.h"
#include "resource.h"

CSearchAppDlg mainDlg;

HINSTANCE hAppInstance;

typedef BOOL (__stdcall *MYPROC)(BOOL);
inline void InitFileSystemIconList();

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
#ifdef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	OleInitialize(0);

	hAppInstance = hInstance;
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	UNREFERENCED_PARAMETER(nCmdShow);

	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	InitFileSystemIconList();

	mainDlg.DoModal();
	OleUninitialize();
	return 0;
}

void InitFileSystemIconList()
{
	HINSTANCE hinstLib; 
    MYPROC FileIconInit = 0; 
 
    // Get a handle to the DLL module.
 
    hinstLib = LoadLibrary(TEXT("Shell32.dll")); 
 
    // If the handle is valid, try to get the function address.
 
    if (hinstLib != NULL) 
    { 
        FileIconInit = (MYPROC) GetProcAddress(hinstLib, (LPCSTR)MAKELPARAM(660, 0)); 
 
        // If the function address is valid, call the function.
 
        if (NULL == FileIconInit) 
        {
            MessageBox(0, L"Error loading a function from Shell32.DLL", 0, 0);
			PostQuitMessage(0);
        }
    } 
	else
	{
		MessageBox(0, L"Error loading Shell32.DLL", 0, 0);
		PostQuitMessage(0);
	}

	_ASSERTE(FileIconInit(FALSE));

	FreeLibrary(hinstLib);
}