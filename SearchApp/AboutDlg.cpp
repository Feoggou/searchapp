#include "General.h"
#include "AboutDlg.h"
#include "resource.h"

extern HINSTANCE hAppInstance;

CAboutDlg::CAboutDlg(void)
{
}

CAboutDlg::~CAboutDlg(void)
{
}

INT_PTR CALLBACK CAboutDlg::DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static CAboutDlg* pThis = NULL;
	
	if (uMsg == WM_INITDIALOG)
	{
		pThis = (CAboutDlg*)lParam;
		SetWindowLongW(hDlg, GWL_USERDATA, lParam);
		pThis->m_hWnd = hDlg;
		return pThis->OnInitDialog();
	}

	switch (uMsg)
	{
	case WM_CLOSE:
		{
			EndDialog(hDlg, IDOK);
		}
		break;

	case WM_COMMAND:
		{
			if (LOWORD(wParam) == IDOK)
			{
				EndDialog(hDlg, IDOK);
			}
		}
		break;

	case WM_INITDIALOG:
		{
			return pThis->OnInitDialog();
		}
		break;

	case WM_NOTIFY:
		{
			NMHDR* pNMHDR = (NMHDR*)lParam;
			switch (pNMHDR->code)
			{
			case NM_CLICK:
				{
					SHELLEXECUTEINFO sei;
					ZeroMemory(&sei,sizeof(SHELLEXECUTEINFO));
					sei.cbSize = sizeof(SHELLEXECUTEINFO);
					sei.lpVerb = TEXT( "open" );
					sei.lpFile = L"http://compose.mail.yahoo.com/?To=fio_244@yahoo.com";	
					sei.nShow = SW_SHOWNORMAL;

					ShellExecuteEx(&sei);
				}
				break;
			}
		}
		break;
	}

	return 0;
}

INT_PTR CAboutDlg::OnInitDialog()
{
	return TRUE;
}

void CAboutDlg::DoModal(HWND hParent)
{
	INT_PTR result = DialogBoxParamW(hAppInstance, MAKEINTRESOURCE(IDD_ABOUTBOX), hParent, DialogProc, (LPARAM)this);
	if (-1 == result)
	{
		DisplayError();
	}
}