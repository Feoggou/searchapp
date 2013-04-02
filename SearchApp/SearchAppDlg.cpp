//#define _WIN32_WINNT	0x0400

#include "SearchAppDlg.h"
#include "resource.h"
#include "DataObject.h"
#include "FileOperationProgressSink.h"
#include "OptionsDlg.h"
#include "AboutDlg.h"

#include <filterr.h>

#define SEARCH_TASK_ICON	0x00FF
#define WM_TRAYICONMSG		WM_USER + 20
#define WM_TARYICON_DESTROY WM_USER + 21

//#define TESTING

#ifdef TESTING
#define TESTING_SEARCHIN L"D:\\poze\\test"
#endif

#define INFIDEL
#define IDM_ABOUT 0xFFFE
#define WM_SHELLNOTIFY	WM_USER + 200

extern HINSTANCE hAppInstance;
IContextMenu3* pContextMenu = NULL;
HWND hMainDlg;

WNDPROC CSearchAppDlg::OldEditSizeProc = NULL;
HWND CSearchAppDlg::m_hTaskWnd = NULL;
HANDLE CSearchAppDlg::m_hSecondThread = NULL;
BOOL CSearchAppDlg::m_bStopSearch = FALSE;

void __stdcall OnDestroyItem(FILEITEM& Item)
{
	CoTaskMemFree(Item.wsFullName);
}

void __stdcall OnDestroyFilter(FILTER& filter)
{
	filter.pClassFactory->Release();
	delete[] filter.wsExt;
}

void __stdcall OnDestroyFilterList(FILTERLIST& filterlist)
{
	filterlist.pFilterList->erase_all();
	delete filterlist.pFilterList;
}

LRESULT CALLBACK CSearchAppDlg::NewEditSizeProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_KILLFOCUS:
		{
			CSearchAppDlg* pDlg = (CSearchAppDlg*)GetWindowLongW(GetParent(hWnd), GWL_USERDATA);
			int len = GetWindowTextLengthW(hWnd);
			if (len == 0)
			{
				if (hWnd == pDlg->m_hSizeMax)
					SetWindowTextW(hWnd, L"Any");
				else SetWindowTextW(hWnd, L"0 B");
			}
		}
		break;
	}

	return CallWindowProcW(OldEditSizeProc, hWnd, uMsg, wParam, lParam);
}

CSearchAppDlg::CSearchAppDlg(void):
m_FileItems(OnDestroyItem),
m_FilterList(OnDestroyFilterList)
{
	m_hWnd = NULL;

	m_Condition.dwAttribON = FILE_ATTRIBUTE_COMPRESSED | FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_ENCRYPTED |
FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_TEMPORARY;
	m_Condition.dwAttribOFF = m_Condition.dwAttribON;

	m_Condition.m_bCSFileName =  m_Condition.m_bCSContents = false;
	m_hImageList = m_hImageListSmall = NULL;
	m_nCurrentItem = 0;
	m_bSearchFast = FALSE;
	m_dSorted = SortedUnknown;
	*m_wsDesktopPath = 0;

	m_wsSearchIn = m_wsSearchFileName = m_wsSearchFileExt = m_wsSearchContents = NULL;
	m_uRegisteredValue = 0;

	WNDCLASS wndclass = {0};
	wndclass.hInstance = hAppInstance;
	wndclass.lpfnWndProc = SecondWndProc;
	wndclass.lpszClassName = L"SecondWndClass";

	RegisterClassW(&wndclass);
}


CSearchAppDlg::~CSearchAppDlg(void)
{
	if (m_wsSearchIn) {delete[] m_wsSearchIn; m_wsSearchIn = NULL;}
	if (m_wsSearchFileName) {delete[] m_wsSearchFileName; m_wsSearchFileName = NULL;}
	if (m_wsSearchContents) {delete[] m_wsSearchContents; m_wsSearchContents = NULL;}
	m_FilterList.erase_all();
}

INT_PTR CALLBACK CSearchAppDlg::DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static CSearchAppDlg* pThis = NULL;
	
	if (uMsg == WM_INITDIALOG)
	{
		hMainDlg = hDlg;
		pThis = (CSearchAppDlg*)lParam;
		pThis->m_hWnd = hDlg;
		SetWindowLongPtrW(hDlg, GWL_USERDATA, lParam);
		return pThis->OnInitDialog();
	}

	if (pThis)
	{
		switch (uMsg)
		{
		case WM_SHELLNOTIFY:
			{
				pThis->OnShellNotify(wParam, lParam);
			}
			break;

		case WM_HELP:
			{
				WCHAR wsHelp[MAX_PATH];
				GetWindowsDirectory(wsHelp, MAX_PATH);
				StringCchCatW(wsHelp, MAX_PATH, L"\\hh.exe");

				DWORD len = MAX_PATH + 28;
				WCHAR* wsDir = new WCHAR[len];
				GetModuleFileNameW(0, wsDir, len);
				WCHAR* wpos= wcsrchr(wsDir, '\\');
				if (wpos)
				{
					*wpos = 0;
				}

				StringCchCatW(wsDir, len, L"\\SearchApp Documentation.chm");

				ShellExecute(hDlg, L"open", wsHelp, wsDir, 0, 1);
				delete[] wsDir;
			}
			break;

			//context menu:
		case WM_INITMENUPOPUP:
			{
				if (pContextMenu)
				{
					LRESULT lResult;
					pContextMenu->HandleMenuMsg2(uMsg, wParam, lParam, &lResult);
					return lResult;
				}
			}
			break;

		case WM_INITMENU:
			{
				if (pContextMenu)
				{
					LRESULT lResult;
					pContextMenu->HandleMenuMsg2(uMsg, wParam, lParam, &lResult);
					return lResult;
				}
			}
			break;

		case WM_DRAWITEM:
			{
				if (pContextMenu)
				{
					LRESULT lResult;
					pContextMenu->HandleMenuMsg2(uMsg, wParam, lParam, &lResult);
					return lResult;
				}
			}
			break;

		case WM_MEASUREITEM:
			{
				if (pContextMenu)
				{
					LRESULT lResult;
					pContextMenu->HandleMenuMsg2(uMsg, wParam, lParam, &lResult);
					return lResult;
				}
			}
			break;

		case WM_MENUCHAR:
			{
				if (pContextMenu)
				{
					LRESULT lResult;
					pContextMenu->HandleMenuMsg2(uMsg, wParam, lParam, &lResult);
					return lResult;
				}
			}

			//other messages:
		case WM_NOTIFY:
			{
				NMHDR* pNMHDR = (NMHDR*)lParam;
				switch (pNMHDR->code)
				{
					//the user clicked a header item
				case HDN_ITEMCLICK:
					{
						NMHEADER* pHeader = (NMHEADER*)lParam;
						if (pHeader->iButton == 0)//if left mouse button was pressed
							pThis->OnHeaderItemClicked(pHeader->iItem);
					}
					break;

					//the user double-clicked an item in the list
				case NM_DBLCLK:
					{
						NMITEMACTIVATE* pItemAct = (NMITEMACTIVATE*)lParam;

						if (pItemAct->iItem > -1)
						{
							FILEITEM* pItem = &pThis->m_FileItems[pItemAct->iItem];
							*(--pItem->wsDisplayName) = '\\';
							if (!pThis->PathFileExistsEx(pItem->wsFullName))
							{
								MessageBox(hDlg, L"This item does not exist - it has been deleted or moved.", 0, 0);
							}
							else
								ShellExecute(hDlg, NULL, pItem->wsFullName, NULL, NULL, SW_SHOWDEFAULT);
							*pItem->wsDisplayName = 0;
							++pItem->wsDisplayName;
						}
					}
					break;

					//the user right-clisk an item in the list
				case NM_RCLICK:
					{
						NMITEMACTIVATE* pItemAct = (NMITEMACTIVATE*)lParam;

						if (pItemAct->iItem > -1 && pItemAct->iItem < pThis->m_FileItems.size())
							pThis->OnListItemContextMenu(pItemAct->iItem, pItemAct->ptAction);
					}
					break;

				case NM_CLICK:
					{
						if (pNMHDR->idFrom == IDCL_ABOUT)
						{
							CAboutDlg dlg;
							dlg.DoModal(hDlg);
						}
					}
					break;
				}
			}
			break;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				//"Search"
			case IDC_SEARCH: pThis->OnSearch();
				break;

			case IDC_BROWSE:
				{
					IFileOpenDialog* pOpen;
					HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, (void**)&pOpen);
					if (FAILED(hr))
					{
						MessageBox(hDlg, L"Could not create the open dialog", 0, 0);
						break;
					}

					ITEMIDLIST* pidlDesktop;
					hr = SHGetKnownFolderIDList(FOLDERID_Desktop, 0, 0, &pidlDesktop);
					if (FAILED(hr))
					{
						pOpen->Release();

						MessageBox(hDlg, L"Could not create the open dialog", 0, 0);
						break;
					}

					IShellItem* pDesktopItem;
					hr = SHCreateItemFromIDList(pidlDesktop, IID_IShellItem, (void**)&pDesktopItem);
					if (FAILED(hr))
					{
						CoTaskMemFree(pidlDesktop);
						pOpen->Release();

						MessageBox(hDlg, L"Could not create the open dialog", 0, 0);
						break;
					}

					CoTaskMemFree(pidlDesktop);
			
					pOpen->SetDefaultFolder(pDesktopItem);
					pDesktopItem->Release();

					pOpen->SetOptions(FOS_PICKFOLDERS | FOS_PATHMUSTEXIST | FOS_FILEMUSTEXIST | FOS_FORCEFILESYSTEM);
					hr = pOpen->Show(hDlg);
					if (hr == HRESULT_FROM_WIN32(ERROR_CANCELLED))
					{
						pOpen->Release();
						break;
					}

					hr = pOpen->GetResult(&pDesktopItem);
					if (FAILED(hr))
					{
						pOpen->Release();

						MessageBox(hDlg, L"Could not create the open dialog", 0, 0);
						break;
					}

					WCHAR* wsPath;
					hr = pDesktopItem->GetDisplayName(SIGDN_FILESYSPATH, &wsPath);
					if (FAILED(hr))
					{
						pOpen->Release();
						pDesktopItem->Release();

						MessageBox(hDlg, L"Could not create the open dialog", 0, 0);
						break;
					}

					SetDlgItemTextW(hDlg, IDC_EDIT_SEARCHIN, wsPath);
					CoTaskMemFree(wsPath);

					pDesktopItem->Release();
					pOpen->Release();

				}
				break;

			case IDC_MORE_OPTIONS:
				{
					COptionsDlg moreOptions;
					moreOptions.DoModal(hDlg, &pThis->m_Condition);
				}
				break;
			}
			break;

		case WM_CLOSE:
			EndDialog(hDlg, IDOK);
			break;

		case WM_SIZE:
			pThis->OnSize(wParam, LOWORD(lParam), HIWORD(lParam));
			break;

		case WM_GETMINMAXINFO:
			{
				MINMAXINFO* lpMMI = (MINMAXINFO*)lParam;
				lpMMI->ptMinTrackSize.x = 526;
				lpMMI->ptMinTrackSize.y = 340;
			}
			break;

		case WM_DESTROY:
			{
				if (pThis->m_uRegisteredValue)
				{
					SHChangeNotifyDeregister(pThis->m_uRegisteredValue);
				}
			}
			break;

		case WM_CTLCOLOREDIT:
			{
				HWND hEdit = (HWND)lParam;

				if (hEdit == pThis->m_hSizeMin || hEdit == pThis->m_hSizeMax)
				{
					HDC hDC = (HDC)wParam;
					SetTextColor(hDC, RGB(192, 0, 0));
					SetBkColor(hDC, RGB(255, 255, 255));
					HBRUSH hBrush = (HBRUSH)GetStockObject(WHITE_BRUSH);
					return (INT_PTR)hBrush;
				}
			}
			break;

		case WM_SYSCOMMAND:
			{
				if (wParam == IDM_ABOUT)
				{
					CAboutDlg aboutDlg;
					aboutDlg.DoModal(hDlg);
				}
			}
			break;
		}
	}

	return 0;
}

INT_PTR CSearchAppDlg::DoModal(void)
{
	return DialogBoxParamW(hAppInstance, MAKEINTRESOURCE(IDD_SEARCHAPP_DIALOG), NULL, DialogProc, (LPARAM)this);
}

HHOOK hHookMsg;
LRESULT CALLBACK CSearchAppDlg::MessageFilter(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode == MSGF_DIALOGBOX)
	{
		MSG* pMsg = (MSG*)lParam;
		if (pMsg->message == WM_KEYDOWN)

			if (pMsg->hwnd == GetDlgItem(hMainDlg, IDC_EDIT_SEARCHIN) && pMsg->wParam == VK_RETURN)
			{
				HWND hOld = SetFocus(hMainDlg);
				SetFocus(hOld);
			}

			if (pMsg->hwnd == GetDlgItem(hMainDlg, IDC_EDIT_SEARCH_FILENAME))
			{
				//THE USER HAS PRESSED ENTER IN THE SEARCH FILENAME EDITBOX
				if (pMsg->wParam == VK_RETURN)
				{
					SendMessage(hMainDlg, WM_COMMAND, IDC_SEARCH, 0);
				}
			}

			else if (pMsg->hwnd == GetDlgItem(hMainDlg, IDC_LIST_ITEMSVIEW))
			{
				//THE USER HAS PRESSED ENTER ON AN ITEM IN THE LISTBOX: EXECUTE COMMAND!
				if (pMsg->wParam == VK_RETURN)
				{
					if (SendMessage(pMsg->hwnd, LVM_GETSELECTEDCOUNT, 0, 0) == 1)
					{
						int nCurSel = SendMessage(pMsg->hwnd, LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);
						CSearchAppDlg* pThis = (CSearchAppDlg*)GetWindowLongW(hMainDlg, GWL_USERDATA);
						CDoubleList<FILEITEM>::Iterator I = pThis->m_FileItems.GetAt(nCurSel);
						ShellExecute(hMainDlg, L"open", I->m_Value.wsDisplayName, 0, I->m_Value.wsFullName, 1);
					}
				}

				//THE USER HAS PRESSED DELETE ON ONE OR MORE ITEMS IN THE LIST: DELETE THEM!
				else if (pMsg->wParam == VK_DELETE)
				{
					//PERMANENT DELETE
					if (GetKeyState(VK_SHIFT) & 0x8000)
					{
						CSearchAppDlg* pThis = (CSearchAppDlg*)GetWindowLongW(hMainDlg, GWL_USERDATA);

						IShellFolder* pDesktopFolder;
						HRESULT hr = SHGetDesktopFolder(&pDesktopFolder);
						if (FAILED(hr))
						{
							MessageBox(hMainDlg, L"Could not retriev the shell desktop folder", 0, 0);
							goto end;
						}

						int nSize;
						ITEMIDLIST** pidlChildren;
						pThis->FilterSelectedItems(pDesktopFolder, NULL, nSize, &pidlChildren);
						pDesktopFolder->Release();

						pThis->OnPermanentDeleteItems(pidlChildren, nSize);
						for (int i = 0; i < nSize; i++)
							CoTaskMemFree(pidlChildren[i]);
						delete[] pidlChildren;
					}

					//NORMAL DELETE
					else
					{
						CSearchAppDlg* pThis = (CSearchAppDlg*)GetWindowLongW(hMainDlg, GWL_USERDATA);

						IShellFolder* pDesktopFolder;
						HRESULT hr = SHGetDesktopFolder(&pDesktopFolder);
						if (FAILED(hr))
						{
							MessageBox(hMainDlg, L"Could not retriev the shell desktop folder", 0, 0);
							goto end;
						}

						int nSize;
						ITEMIDLIST** pidlChildren;
						pThis->FilterSelectedItems(pDesktopFolder, NULL, nSize, &pidlChildren);
						pDesktopFolder->Release();

						pThis->OnDeleteItems(pidlChildren, nSize);
						for (int i = 0; i < nSize; i++)
							CoTaskMemFree(pidlChildren[i]);
						delete[] pidlChildren;
					}
				}

				//SELECT ALL
				else if (pMsg->wParam == 'A' && (GetKeyState(VK_CONTROL) & 0x8000))
				{
					CSearchAppDlg* pThis = (CSearchAppDlg*)GetWindowLongW(hMainDlg, GWL_USERDATA);
					ListView_SetItemState(pThis->m_hListCtrl, -1, LVIS_SELECTED, LVIS_SELECTED);
				}

				//COPY ITEMS
				else if (pMsg->wParam == 'C' && (GetKeyState(VK_CONTROL) & 0x8000))
				{
					CSearchAppDlg* pThis = (CSearchAppDlg*)GetWindowLongW(hMainDlg, GWL_USERDATA);

					IShellFolder* pDesktopFolder;
					HRESULT hr = SHGetDesktopFolder(&pDesktopFolder);
					if (FAILED(hr))
					{
						MessageBox(hMainDlg, L"Could not retriev the shell desktop folder", 0, 0);
						goto end;
					}

					CDoubleList<ITEM>Items(NULL);
					int nSize;
					pThis->FilterSelectedItems(pDesktopFolder, &Items, nSize, NULL);
					pDesktopFolder->Release();
					pThis->CopyItems(Items);
				}

				//CUT ITEMS
				else if (pMsg->wParam == 'X' && (GetKeyState(VK_CONTROL) & 0x8000))
				{
					CSearchAppDlg* pThis = (CSearchAppDlg*)GetWindowLongW(hMainDlg, GWL_USERDATA);

					IShellFolder* pDesktopFolder;
					HRESULT hr = SHGetDesktopFolder(&pDesktopFolder);
					if (FAILED(hr))
					{
						MessageBox(hMainDlg, L"Could not retriev the shell desktop folder", 0, 0);
						goto end;
					}

					CDoubleList<ITEM>Items(NULL);
					int nSize;
					pThis->FilterSelectedItems(pDesktopFolder, &Items, nSize, NULL);
					pDesktopFolder->Release();
					pThis->CutItems(Items);
				}
			}
	}

end:
	return CallNextHookEx(hHookMsg, nCode, wParam, lParam);
}

BOOL CSearchAppDlg::OnInitDialog(void)
{ 
	LoadFilters();
	//the menu:
	{
		HMENU hMenu = GetSystemMenu(m_hWnd, FALSE);
		int nID = GetMenuItemCount(hMenu);
		MENUITEMINFO mii = {0};
		mii.cbSize = sizeof(mii);
		mii.fMask = MIIM_STRING | MIIM_FTYPE | MIIM_ID;
		mii.fType = MFT_STRING;
		mii.wID = IDM_ABOUT;
		mii.dwTypeData = L"&About";

		InsertMenuItemW(hMenu, nID - 2, TRUE, &mii);
	}
	//checkboxes
	SendMessage(GetDlgItem(m_hWnd, IDC_CS_FILENAME), BM_SETCHECK, m_Condition.m_bCSFileName, 0);
	SendMessage(GetDlgItem(m_hWnd, IDC_CS_FILECONTENTS), BM_SETCHECK, m_Condition.m_bCSContents, 0);
	SendMessage(GetDlgItem(m_hWnd, IDC_ON_DIRECTORIES), BM_SETCHECK, m_Condition.dwAttribON & FILE_ATTRIBUTE_DIRECTORY, 0);
	SendMessage(GetDlgItem(m_hWnd, IDC_OFF_DIRECTORIES), BM_SETCHECK, m_Condition.dwAttribOFF & FILE_ATTRIBUTE_DIRECTORY, 0);
	SendMessage(GetDlgItem(m_hWnd, IDC_ON_HIDDEN), BM_SETCHECK, m_Condition.dwAttribON & FILE_ATTRIBUTE_HIDDEN, 0);
	SendMessage(GetDlgItem(m_hWnd, IDC_OFF_HIDDEN), BM_SETCHECK, m_Condition.dwAttribOFF & FILE_ATTRIBUTE_HIDDEN, 0);

	m_hIcon = LoadIcon(hAppInstance, MAKEINTRESOURCE(IDR_MAINFRAME));
	SendMessage(m_hWnd, WM_SETICON, ICON_BIG, (LPARAM)m_hIcon);
	SendMessage(m_hWnd, WM_SETICON, ICON_SMALL, (LPARAM)m_hIcon);

	hHookMsg = SetWindowsHookEx(WH_MSGFILTER, MessageFilter, 0, GetCurrentThreadId());

	//handles to controls:
	m_hListCtrl = GetDlgItem(m_hWnd, IDC_LIST_ITEMSVIEW);
	m_hSearchFileName = GetDlgItem(m_hWnd, IDC_EDIT_SEARCH_FILENAME);
	m_hSearchIn = GetDlgItem(m_hWnd, IDC_EDIT_SEARCHIN);
	m_hSearchContents = GetDlgItem(m_hWnd, IDC_EDIT_CONTAININGTEXT);
	m_hBrowse = GetDlgItem(m_hWnd, IDC_BROWSE);
	//size:
	m_hSizeMin = GetDlgItem(m_hWnd, IDC_SIZE_MIN);
	m_hSizeMax = GetDlgItem(m_hWnd, IDC_SIZE_MAX);

	SHAutoComplete(m_hSearchIn, SHACF_AUTOSUGGEST_FORCE_ON | SHACF_FILESYS_ONLY);

	InitializeSizeBoxes();
	OldEditSizeProc = (WNDPROC)SetWindowLongW(m_hSizeMin, GWL_WNDPROC, (LONG)NewEditSizeProc);
	SetWindowLongW(m_hSizeMax, GWL_WNDPROC, (LONG)NewEditSizeProc);

	SetClientRects();

	ListView_SetExtendedListViewStyleEx(m_hListCtrl, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_HEADERDRAGDROP,
		LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_HEADERDRAGDROP);
	
	//set the default search path, in case nothing was found in opened folders and nothing valid in the clipboard.
	{
		HKEY hKey;
		DWORD dwError = RegOpenCurrentUser(KEY_READ, &hKey);
		if (dwError != ERROR_SUCCESS)
		{
			MessageBox(0, L"Could not retrieve the current user reg key", 0, 0);
			goto after_block;
		}

		DWORD cbData = MAX_PATH * 2;
		dwError = RegGetValueW(hKey, L"Volatile Environment", L"USERPROFILE", RRF_RT_REG_SZ, NULL, (void*)m_wsDesktopPath, &cbData);
		if (dwError != ERROR_SUCCESS)
		{
			MessageBox(0, L"Could not read the registry value", 0, 0);
			goto after_block;
		}

		//creating our temporary directory and store its full name
		cbData = MAX_PATH * 2;
		StringCchCopyW(m_wsTempFolder, 288, L"\\\\?\\");
		dwError = RegGetValueW(hKey, L"Volatile Environment", L"LOCALAPPDATA", RRF_RT_REG_SZ, NULL, (void*)(m_wsTempFolder + 4), &cbData);
		if (dwError != ERROR_SUCCESS)
		{
			MessageBox(0, L"Could not read the registry value", 0, 0);
			goto after_block;
		}
		StringCchCatW(m_wsTempFolder, 288, L"\\Temp\\_SearchApp_Zenith_");
		m_nTempFolderLen = wcslen(m_wsTempFolder);
		CreateDirectoryW(m_wsTempFolder, NULL);

		RegCloseKey(hKey);

		StringCchCatW(m_wsDesktopPath, MAX_PATH, L"\\Desktop");

		SetWindowText(m_hSearchIn, m_wsDesktopPath);
		//set the focus to the search filename.
		SetFocus(m_hSearchFileName);

#ifdef TESTING
		//set the default search filename
		SetWindowText(m_hSearchFileName, L"*");
		SendMessage(m_hSearchFileName, EM_SETSEL, 0, -1);

		//set the default search path
		SetWindowText(m_hSearchIn, TESTING_SEARCHIN);
#endif
	}

after_block:
	//if we have a folder opened, we get that folder's HWND! (IT IS THE FIRST of CabinetWClass CLASS)
	HWND hDesktop = GetDesktopWindow();
	HWND hWndFound = FindWindowEx(hDesktop, NULL, L"CabinetWClass", NULL);
	hDesktop = hWndFound;
	hWndFound = FindWindowEx(hWndFound, NULL, L"WorkerW", NULL);
	hWndFound = FindWindowEx(hWndFound, NULL, L"ReBarWindow32", NULL);
	hWndFound = FindWindowEx(hWndFound, NULL, L"Address Band Root", NULL);
	hWndFound = FindWindowEx(hWndFound, NULL, L"msctls_progress32", NULL);
	hWndFound = FindWindowEx(hWndFound, NULL, L"Breadcrumb Parent", NULL);
	hWndFound = FindWindowEx(hWndFound, NULL, L"ToolbarWindow32", NULL);

	//INSERTING THE COLUMNS OF THE LISTCTRL!!
	RECT rect;
	GetClientRect(m_hListCtrl, &rect);

	LVCOLUMN lvcolumn;
	lvcolumn.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
	lvcolumn.fmt = LVCFMT_LEFT;
	lvcolumn.cx = (int)(rect.right * 0.25);
	lvcolumn.pszText = L"Name";
	SendMessage(m_hListCtrl, LVM_INSERTCOLUMN, 0, (LPARAM)&lvcolumn);

	lvcolumn.cx = (int)(rect.right * 0.5);
	lvcolumn.pszText = L"Parent Folder";
	SendMessage(m_hListCtrl, LVM_INSERTCOLUMN, 1, (LPARAM)&lvcolumn);

	lvcolumn.cx = (int)(rect.right * 0.13);
	lvcolumn.pszText = L"Date Modified";
	SendMessage(m_hListCtrl, LVM_INSERTCOLUMN, 2, (LPARAM)&lvcolumn);

	lvcolumn.cx = (int)(rect.right * 0.12);
	lvcolumn.pszText = L"Type";
	SendMessage(m_hListCtrl, LVM_INSERTCOLUMN, 3, (LPARAM)&lvcolumn);

	//NU MERE BINE: TRE SA GASESC ALTA VARIANTA SA GASEASCA TOP-LEVEL FOLDER WINDOW!!!
	if (hDesktop && !IsIconic(hDesktop))
	{
		//retrieving the text of the hWndFound folder.
		//first, the length
		int len = SendMessageW(hWndFound, WM_GETTEXTLENGTH, 0, 0);
		_ASSERT(len);
		len++;

		//now, the text itself
		WCHAR* wstr;
		wstr = new WCHAR[len];
		SendMessageW(hWndFound, WM_GETTEXT, len, (LPARAM)wstr);

		WCHAR* wsAddress = wstr + 5;
		*wsAddress = '\\';
		*(wsAddress + 1) = '\\';
		*(wsAddress + 2) = '?';
		*(wsAddress + 3) = '\\';

		//now sAddress has the exact Path. We must make sure that sAddress is a FileSystem path.
		if (PathFileExistsEx(wsAddress, TRUE))
		{
			SetWindowText(m_hSearchIn, wsAddress + 4);
			delete[] wstr;
		}
		else {delete[] wstr; return 0;}
	}
	else
	{
		//we get the text contents from the clipboard
		if (!IsClipboardFormatAvailable(CF_UNICODETEXT)) 
            return 0; 
		//we open the clipboard
		OpenClipboard(m_hWnd);
		//get the clipboard data
		HANDLE hClipBoard = GetClipboardData(CF_UNICODETEXT);
		int len = wcslen((WCHAR*)hClipBoard);
		len += 5;

		WCHAR* wsClipBoard = new WCHAR[len];
		StringCchCopy(wsClipBoard, len, L"\\\\?\\");
		StringCchCat(wsClipBoard, len, (WCHAR*)hClipBoard);
		//close the clipboard
		CloseClipboard();
		
		if (!PathFileExistsEx(wsClipBoard, TRUE)) {delete[] wsClipBoard; return 0;}
		SetWindowText(m_hSearchIn, wsClipBoard + 4);
		delete[] wsClipBoard;
	}

	return TRUE;
}

inline void CSearchAppDlg::SetClientRects(void)
{
	GetClientRect(m_hWnd, &m_DefDlgRect);
	GetWindowRect(m_hBrowse, &m_DefButBrowseRect);
	ScreenToClient(m_DefButBrowseRect);

	GetWindowRect(m_hSearchIn, &m_DefEditBrowseRect);
	ScreenToClient(m_DefEditBrowseRect);

	GetWindowRect(m_hListCtrl, &m_DefListCtrlRect);
	ScreenToClient(m_DefListCtrlRect);
}

inline void CSearchAppDlg::ScreenToClient(RECT& rect)
{
	POINT pt1, pt2;

	pt1.x = rect.left;
	pt1.y = rect.top;
	::ScreenToClient(m_hWnd, &pt1);
	rect.left = pt1.x;
	rect.top = pt1.y;

	pt2.x = rect.right;
	pt2.y = rect.bottom;
	::ScreenToClient(m_hWnd, &pt2);
	rect.right = pt2.x;
	rect.bottom = pt2.y;
}


void CSearchAppDlg::OnSize(UINT nType, WORD cx, WORD cy)
{
	if (nType == SIZE_MINIMIZED || !IsWindowVisible(m_hWnd)) return;

	//positioning and sizing controls:
	//1. Moving Browse button: positioning at the specified distance from the right
	int xPos, yPos;
	RECT rect = {0};
	GetWindowRect(m_hBrowse, &rect);
	ScreenToClient(rect);
	//the original distance between the left margin of the dlg and the right margin of the button
	xPos = m_DefDlgRect.right - m_DefButBrowseRect.left;
	//moving the button by xPos points.
	xPos = cx - xPos;
	SetWindowPos(m_hBrowse, NULL, xPos, rect.top, 0, 0, SWP_NOZORDER | SWP_NOSIZE);

	//2. Resizing IDC_EDIT_SEARCHIN edit control: its right margin must be at the specified distance from the left margin of the button
	//that is, NewEditSize = OldEditSize + DlgNewSize - DlgOldSize
	GetWindowRect(m_hSearchIn, &rect);
	ScreenToClient(rect);
	xPos = m_DefEditBrowseRect.right + cx - m_DefDlgRect.right - m_DefEditBrowseRect.left;
	yPos = m_DefEditBrowseRect.bottom - m_DefEditBrowseRect.top;
	SetWindowPos(m_hSearchIn, NULL, 0, 0, xPos, yPos, SWP_NOZORDER | SWP_NOMOVE);

	//3. Resizing the ListCtrl
	GetWindowRect(m_hListCtrl, &rect);
	ScreenToClient(rect);
	xPos = m_DefListCtrlRect.right + cx - m_DefDlgRect.right - m_DefListCtrlRect.left;
	yPos = m_DefListCtrlRect.bottom + cy - m_DefDlgRect.bottom - m_DefListCtrlRect.top;
	SetWindowPos(m_hListCtrl, NULL, 0, 0, xPos, yPos, SWP_NOZORDER | SWP_NOMOVE);
}


inline bool CSearchAppDlg::PathFileExistsEx(const WCHAR* wsPath, BOOL bMustBeDir)
{
	/*WIN32_FIND_DATA data;
	HANDLE hFind = FindFirstFile(wsPath, &data);
	if (INVALID_HANDLE_VALUE == hFind)
	{
		return false;
	}

	if (bMustBeDir && !(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY || data.dwFileAttributes & FILE_ATTRIBUTE_DEVICE))
		return false;

	FindClose(hFind);
	return true;*/

	/*HANDLE hFile = CreateFileW(wsPath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);
	if (INVALID_HANDLE_VALUE == hFile)
	{
		return false;
	}*/

	DWORD dwAttr = GetFileAttributes(wsPath);

	if (dwAttr == INVALID_FILE_ATTRIBUTES) return false;

	if (bMustBeDir && !(dwAttr & FILE_ATTRIBUTE_DIRECTORY || dwAttr & FILE_ATTRIBUTE_DEVICE))
		return false;

	//CloseHandle(hFile);
	return true;
}

LRESULT CALLBACK CSearchAppDlg::SecondWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_TARYICON_DESTROY:
		{
			NOTIFYICONDATA nid = {0};

			nid.cbSize = sizeof(nid);
			nid.hWnd = m_hTaskWnd;
			nid.uID = SEARCH_TASK_ICON;
			Shell_NotifyIconW(NIM_DELETE, &nid);

			DestroyWindow(m_hTaskWnd);
			m_hTaskWnd = NULL;
		}
		break;

	case WM_TRAYICONMSG:
		{
			if (lParam == WM_RBUTTONUP)
			{
				HMENU hMenu = CreatePopupMenu();

#define ID_STOPSEARCH 1

				AppendMenuW(hMenu, MF_STRING, ID_STOPSEARCH, L"Stop searching");

				POINT pt = {0};
				GetCursorPos(&pt);

				BOOL bResult = TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_NONOTIFY | TPM_RETURNCMD | TPM_LEFTBUTTON,
					pt.x, pt.y, 0, m_hTaskWnd, NULL);

				if (bResult == ID_STOPSEARCH)
				{
					m_bStopSearch = TRUE;
				}
			}
		}
		break;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

DWORD WINAPI CSearchAppDlg::SecondThread(CSearchAppDlg* pDlg)
{
	//creating the window that receives the notifications
	m_hTaskWnd = CreateWindowW(L"SecondWndClass", 0, 0, 0, 0, 10, 10, 0, 0, hAppInstance, 0);
	if (m_hTaskWnd)
	{
		WCHAR* wsFolderName = wcsrchr(pDlg->m_wsSearchIn, '\\');
		if (wsFolderName == 0) wsFolderName = pDlg->m_wsSearchIn;
		else wsFolderName++;

		NOTIFYICONDATA nid = {0};
		nid.cbSize = sizeof(nid);
		nid.hWnd = m_hTaskWnd;
		nid.uID = SEARCH_TASK_ICON;
		nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;
		nid.hIcon = pDlg->m_hIcon;
		nid.uCallbackMessage = WM_TRAYICONMSG;
		StringCchPrintfW(nid.szTip, 128, L"Searching for \"%s\" in \"%s\"", pDlg->m_wsSearchFileName, wsFolderName);
		Shell_NotifyIconW(NIM_ADD, &nid);

		MSG msg = {0};
		int nResult;
		do
		{ 
			nResult = PeekMessageW(&msg, m_hTaskWnd, 0, 0, TRUE);
			if (nResult)
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			else
			{
				if (!m_hTaskWnd) break;
				Sleep(50);
			}
		} 
		while(msg.message != WM_DESTROY);
	}

	CloseHandle(GetCurrentThread());

	return 0;
}

inline void CSearchAppDlg::OnSearch()
{
	if (false == CheckSizes()) return;

	if (BST_UNCHECKED == SendMessage(GetDlgItem(m_hWnd, IDC_ON_DIRECTORIES), BM_GETCHECK, 0, 0) &&
	BST_UNCHECKED == SendMessage(GetDlgItem(m_hWnd, IDC_OFF_DIRECTORIES), BM_GETCHECK, 0, 0))
	{
		SendMessage(GetDlgItem(m_hWnd, IDC_ON_DIRECTORIES), BM_SETCHECK, 1, 0);
		SendMessage(GetDlgItem(m_hWnd, IDC_OFF_DIRECTORIES), BM_SETCHECK, 1, 0);
	}

	if (BST_UNCHECKED == SendMessage(GetDlgItem(m_hWnd, IDC_ON_HIDDEN), BM_GETCHECK, 0, 0) &&
	BST_UNCHECKED == SendMessage(GetDlgItem(m_hWnd, IDC_OFF_HIDDEN), BM_GETCHECK, 0, 0))
	{
		SendMessage(GetDlgItem(m_hWnd, IDC_ON_HIDDEN), BM_SETCHECK, 1, 0);
		SendMessage(GetDlgItem(m_hWnd, IDC_OFF_HIDDEN), BM_SETCHECK, 1, 0);
	}

	if (!m_FileItems.is_empty())
	{
		m_FileItems.erase_all();
		SendMessage(m_hListCtrl, LVM_DELETEALLITEMS, 0, 0);
		m_nCurrentItem = 0;
	}

	//SIZE

	if (m_SizeMin == 0 && m_SizeMax == (QWORD)-1)
	{
		if (m_Condition.Size)
		{
			delete[] m_Condition.Size;
			m_Condition.Size = NULL;
		}
	}
	else
	{
		SendMessage(GetDlgItem(m_hWnd, IDC_ON_DIRECTORIES), BM_SETCHECK, 0, 0);
		SendMessage(GetDlgItem(m_hWnd, IDC_OFF_DIRECTORIES), BM_SETCHECK, 1, 0);

		if (!m_Condition.Size)
		{
			m_Condition.Size = new QWORD[2];
		}

		m_Condition.Size[0] = m_SizeMin;
		m_Condition.Size[1] = m_SizeMax;
	}

	//case sensitive filename search
	m_Condition.m_bCSFileName = (BOOL)SendMessage(GetDlgItem(m_hWnd, IDC_CS_FILENAME), BM_GETCHECK, 0, 0);
	m_Condition.m_bCSContents = (BOOL)SendMessage(GetDlgItem(m_hWnd, IDC_CS_FILECONTENTS), BM_GETCHECK, 0, 0);

	if (m_wsSearchIn) {delete[] m_wsSearchIn; m_wsSearchIn = NULL;}
	if (m_wsSearchFileName) {delete[] m_wsSearchFileName; m_wsSearchFileName = NULL;}
	if (m_wsSearchContents) {delete[] m_wsSearchContents; m_wsSearchContents = NULL;}

#ifdef TESTING
	//performance checking only!
	LARGE_INTEGER liFirst, liSecond, liFreq;
	QueryPerformanceCounter(&liFirst);
	QueryPerformanceFrequency(&liFreq);
	//-----performance checking only!
#endif

	//retrieve the system imagelist
	if (false == Shell_GetImageLists(&m_hImageList, &m_hImageListSmall))
	{
		MessageBox(m_hWnd, L"Could not retrieve system image list!", 0, 0);
	}

	//set the system imagelist to the ListControl
	ListView_SetImageList(m_hListCtrl, m_hImageListSmall, LVSIL_SMALL);

	//the search text
	int nTextLen = GetWindowTextLengthW(m_hSearchIn);
	if (nTextLen)
	{
		nTextLen++;
		m_wsSearchIn = new WCHAR[nTextLen];
		GetWindowText(m_hSearchIn, m_wsSearchIn, nTextLen);

		if (*(m_wsSearchIn + nTextLen - 2) == '\\')
		{
			*(m_wsSearchIn + nTextLen - 2) = 0;
		}

		if (false == PathFileExistsEx(m_wsSearchIn))
		{
			MessageBox(m_hWnd, L"The path provided does not exist!", L"Improper search request!", MB_ICONWARNING);
			delete[] m_wsSearchIn;
			m_wsSearchIn = NULL;
			return;
		}
	}
	
	nTextLen = GetWindowTextLength(m_hSearchFileName);
	if (nTextLen)
	{
		nTextLen++;
		m_wsSearchFileName = new WCHAR[nTextLen];
		GetWindowText(m_hSearchFileName, m_wsSearchFileName, nTextLen);
	}

	//the search contents:
	nTextLen = GetWindowTextLength(m_hSearchContents);
	if (nTextLen)
	{
		nTextLen++;
		m_wsSearchContents = new WCHAR[nTextLen];
		GetWindowText(m_hSearchContents, m_wsSearchContents, nTextLen);

		//check FILES and uncheck FOLDERS:
		SendMessage(GetDlgItem(m_hWnd, IDC_ON_DIRECTORIES), BM_SETCHECK, 0, 0);
		SendMessage(GetDlgItem(m_hWnd, IDC_OFF_DIRECTORIES), BM_SETCHECK, 1, 0);

		CharCheckEnc = 0;
		if (m_Condition.m_bCSContents)
		{
			CharCheckEnc = CheckCharEncCS;
			SearchUsingFilter = SearchUsingFilterCS;
		}

		else
		{
			CharCheckEnc = CheckCharEnc;
			SearchUsingFilter = SearchUsingFilterNCS;
		}
	}

#ifdef TESTING
	{
		int len = wcslen(TESTING_SEARCHIN) + 1;
		delete[] m_wsSearchIn;
		m_wsSearchIn = new WCHAR[len];
		StringCchCopy(m_wsSearchIn, len, TESTING_SEARCHIN);

		delete[] m_wsSearchFileName;
		m_wsSearchFileName = new WCHAR[6];
		StringCchCopy(m_wsSearchFileName, 6, L"*");
	}
#endif

	//if not case sensitive, we make lower the search filename.
	if (!m_wsSearchFileName)
	{
		MessageBox(m_hWnd, L"You must type something to search", L"Improper search request!", MB_ICONWARNING);
		return;
	}

	//for faster search, hide the window!
	ShowWindow(m_hWnd, 0);
	m_bStopSearch = FALSE;

	m_hSecondThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)SecondThread, (void*)this, 0, 0);

#pragma warning (suppress: 4996)
	if (!m_Condition.m_bCSFileName) _wcslwr(m_wsSearchFileName); 
	GetFileExt(m_wsSearchFileName, &m_wsSearchFileExt, FALSE);

	//getting to the folder
	IShellFolder* pDesktopFolder;
	HRESULT hr = SHGetDesktopFolder(&pDesktopFolder);
	if (FAILED(hr))
	{
		_ASSERT(0);
		MessageBox(0, L"Could not retrieve the desktop folder: SHGetDesktopFolder", 0, 0);
		return;
	}

	ITEMIDLIST* pidlFolder;
	hr = pDesktopFolder->ParseDisplayName(m_hWnd, NULL, m_wsSearchIn, NULL, &pidlFolder, NULL);
	if (FAILED(hr))
	{
		pDesktopFolder->Release();

		_ASSERT(0);
		MessageBox(0, L"Could not retrieve the search folder: ParseDisplayName", 0, 0);
		return;
	}

	IShellFolder* pSearchFolder;
	hr = pDesktopFolder->BindToObject(pidlFolder, NULL, IID_IShellFolder, (void**)&pSearchFolder);
	if (FAILED(hr))
	{
		pDesktopFolder->Release();
		CoTaskMemFree(pidlFolder);

		_ASSERT(0);
		MessageBox(0, L"Could not retrieve the search folder: pDesktopFolder->BindToObject", 0, 0);
		return;
	}

	pDesktopFolder->Release();

	UpdateCondition();
	//retrieving the enumflags:
	m_ulEnumFlags = SHCONTF_FOLDERS | SHCONTF_NONFOLDERS;

	//enumerate hidden items
	if (m_Condition.dwAttribON & FILE_ATTRIBUTE_HIDDEN)
		m_ulEnumFlags |= SHCONTF_INCLUDEHIDDEN;

	if (m_uRegisteredValue)
	{
		SHChangeNotifyDeregister(m_uRegisteredValue);
		m_uRegisteredValue = 0;
	}

	//the actual search
	SearchFolder(pSearchFolder);

	pSearchFolder->Release();

	//after the searching is finish, show the dialogbox:
	ShowWindow(m_hWnd, 1);
	SendMessage(m_hTaskWnd, WM_TARYICON_DESTROY, 0, 0);
	m_bStopSearch = FALSE;

	m_dSorted = SortedUnknown;

	WatchFolder(pidlFolder);
	CoTaskMemFree(pidlFolder);

#ifdef TESTING
	QueryPerformanceCounter(&liSecond);
	double fSeconds = (liSecond.QuadPart - liFirst.QuadPart)/(double)liFreq.QuadPart;

	WCHAR wsTime[25];
	StringCchPrintf(wsTime, 25, L"Done in: %.6fs", fSeconds);
	MessageBox(0, wsTime, 0, 0);
#endif
}

void CSearchAppDlg::SearchFolder(IShellFolder* pSearchFolder)
{
	//getting the enumerator object to enumerate the items of the search folder
	IEnumIDList* pEnumIDList = NULL;
	HRESULT hr = pSearchFolder->EnumObjects(NULL, m_ulEnumFlags, &pEnumIDList);
	if (FAILED(hr))
	{
		if (hr != E_ACCESSDENIED)
			MessageBox(0, L"Could not enumerate objects of the folder: pSearchFolder->EnumObjects", 0, 0);
		return;
	}

	if (hr == S_FALSE) return;

	//getting pidl to each child item
	ITEMIDLIST* pidlChild = NULL;
	HRESULT hrEnum;
	do
	{
		hrEnum = pEnumIDList->Next(1, &pidlChild, NULL);
		if (FAILED(hrEnum))
		{
			pEnumIDList->Release();

			_ASSERT(0);
			MessageBox(0, L"Could not enumerate objects of the folder: pEnumIDList->Next", 0, 0);
			return;
		}

		if (S_FALSE == hrEnum || m_bStopSearch) break;

		//we need to know whether this is a folder or a file, and if it is a system item
		ULONG ulFlags = 0xFFFFFFFF;
		hr = pSearchFolder->GetAttributesOf(1, (LPCITEMIDLIST*)&pidlChild, &ulFlags);
		if (FAILED(hr))
		{
			CoTaskMemFree(pidlChild);
			pidlChild = NULL;
			pEnumIDList->Release();

			_ASSERT(0);
			MessageBox(0, L"Could not get the attributes of the item: pSearchFolder->GetAttributesOf", 0, 0);
			return;
		}

		if (ulFlags & SFGAO_FILESYSTEM)
		{
			if (ulFlags & SFGAO_FOLDER && ulFlags & SFGAO_FILESYSANCESTOR && ulFlags & SFGAO_STORAGE)
			{
				//we need to search it
				IShellFolder* pNewSearchFolder = NULL;
				hr = pSearchFolder->BindToObject(pidlChild, NULL, IID_IShellFolder, (void**)&pNewSearchFolder);
				if (FAILED(hr))
				{
					CoTaskMemFree(pidlChild);
					pidlChild = NULL;
					pEnumIDList->Release();

					_ASSERT(0);
					MessageBox(0, L"Could not bind to new folder: pSearchFolder->BindToObject", 0, 0);
					return;
				}
				
				CheckItem(pSearchFolder, pidlChild, ulFlags);

				SearchFolder(pNewSearchFolder);
				pNewSearchFolder->Release();
			}
			else if (ulFlags & SFGAO_STREAM)
			{
				//it is a file!!
				CheckItem(pSearchFolder, pidlChild, ulFlags);
			}
		}

		CoTaskMemFree(pidlChild);
		pidlChild = NULL;
		#pragma warning(suppress: 4127)
	}while (1);

	if (pidlChild)
		CoTaskMemFree(pidlChild);
	pEnumIDList->Release();
}

void CSearchAppDlg::CheckItem(IShellFolder* pSearchFolder, ITEMIDLIST* pidlChild, ULONG& ulFlags)
{
	//m_ulEnumFlags: if we got here, they are checked:
	//if the item is a directory, it should be accepted
	//if the item is not a directory (that is, a file), it should be accepted
	//if the item is hidden, it should be accepted
	//if the item is super-hidden, it should be accepted (we cannot check that, actually).
	if (false == CheckFlags(ulFlags))
		return;

	//retrieving the fullfilename.
	STRRET strret;
	pSearchFolder->GetDisplayNameOf(pidlChild, SHGDN_FORPARSING, &strret);
	WCHAR* wsFullName, *wsDisplayName, *wsFileExt;
	StrRetToStrW(&strret, NULL, &wsFullName);

	int len = wcslen(wsFullName) + 5;
	WCHAR* wsFileName = new WCHAR[len];
	StringCchCopy(wsFileName, len, L"\\\\?\\");
	StringCchCat(wsFileName, len, wsFullName);

	//retrieving the display name:
	GetDisplayName(wsFullName, &wsDisplayName);
	//retrieving the file extension:
	GetFileExt(wsDisplayName, &wsFileExt, FALSE);

	WIN32_FILE_ATTRIBUTE_DATA data;
	GetFileAttributesExW(wsFileName, GetFileExInfoStandard, &data);
	//checking the attributes
	if (false == CheckAttributes(data))
	{
		delete[] wsFileName;
		CoTaskMemFree(wsFullName);
		return;
	}

	if (!m_Condition.m_bCSFileName)
	{
#pragma warning(suppress: 4996)
		_wcslwr(wsDisplayName);
	}

	if (false == CheckDisplayName(wsDisplayName))
	{
		delete[] wsFileName;
		CoTaskMemFree(wsFullName);
		return;
	}

	if (m_wsSearchContents)
	{
		//if the size of the file is 0 (zero), it is NOT a result.
		if (data.nFileSizeLow == 0 && data.nFileSizeHigh == 0)
		{
			delete[] wsFileName;
			CoTaskMemFree(wsFullName);
			return;
		}

		else if (false == CheckFileContents(wsFileName, wsFileExt, data))
		{
			delete[] wsFileName;
			CoTaskMemFree(wsFullName);
			return;
		}
		//else it is good
	}

	delete[] wsFileName;

	//ADDING THE ITEM
	IShellFolder* pDesktopFolder;
	HRESULT hr = SHGetDesktopFolder(&pDesktopFolder);
	if (FAILED(hr))
	{
		_ASSERT(0);
		MessageBox(0, L"Could not receive desktop folder during search", 0, 0);
		CoTaskMemFree(wsFullName);
		return;
	}

	ITEMIDLIST* pidlItem;
	hr = pDesktopFolder->ParseDisplayName(m_hWnd, NULL, wsFullName, NULL, &pidlItem, NULL);
	if (FAILED(hr))
	{
		_ASSERT(0);
		MessageBox(0, L"Could not receive parse item display name during search", 0, 0);
		pDesktopFolder->Release();
		CoTaskMemFree(wsFullName);
		return;
	}

	pDesktopFolder->Release();

	//getting the file info
	SHFILEINFO fileinfo;
	DWORD_PTR dwResult = SHGetFileInfoW((WCHAR*)pidlItem, 0, &fileinfo, sizeof(fileinfo), SHGFI_SYSICONINDEX | SHGFI_PIDL);
	if (false == dwResult)
	{
		DisplayError();
		CoTaskMemFree(wsFullName);
		return;
	}

	CoTaskMemFree(pidlItem);

	//ADDING THE ITEM TO THE FILEITEM DEQUE
	FILEITEM fileitem;
	//fileitem.wsFullName
	fileitem.wsFullName = wsFullName;
	//fileitem.wsDisplayName
	fileitem.wsDisplayName = fileitem.wsFullName + (wsDisplayName - wsFullName);
	//fileitem.wsFileExt
	if (wsFileExt)
	{
		fileitem.wsExt = fileitem.wsDisplayName + (wsFileExt - wsDisplayName);
#pragma warning(suppress: 4996)
		_wcslwr(fileitem.wsExt);
	}

	//data
	fileitem.data = data;
	//dIcon
	fileitem.dIcon = fileinfo.iIcon;
	m_FileItems.push_back(fileitem);

	//ADDING THE ITEM TO THE LISTCTRL:
	//A. FIRST COLUMN: ITEM DISPLAY NAME
	LVITEM lvItem;
	lvItem.mask = LVIF_IMAGE | LVIF_TEXT;
	lvItem.iItem = m_nCurrentItem;
	lvItem.iSubItem = 0;
	lvItem.pszText = fileitem.wsDisplayName;
	lvItem.iImage = fileinfo.iIcon;
	SendMessage(m_hListCtrl, LVM_INSERTITEM, 0, (LPARAM)&lvItem);

	//B. SECOND COLUMN: PARENT FOLDER'S PATH
	*(--wsDisplayName) = 0;
	ListView_SetItemText(m_hListCtrl, m_nCurrentItem, 1, wsFullName);

	//if it is a fast search, we don't retrieve the other items.
	if (m_bSearchFast)
	{
		m_nCurrentItem++;
		return;
	}

	//C. THIRD COLUMN: ITEM'S DATE & TIME
	SYSTEMTIME SystemTime = {0};
	_ASSERTE(FileTimeToSystemTime(&data.ftLastWriteTime, &SystemTime));
	
	WCHAR wsTime[20];
	StringCchPrintfW(wsTime, 20, L"%02d.%02d.%d %02d:%02d", SystemTime.wDay, SystemTime.wMonth, SystemTime.wYear,
SystemTime.wHour, SystemTime.wMinute);
	ListView_SetItemText(m_hListCtrl, m_nCurrentItem, 2, wsTime);

	//D. FOURTH COLUMN: ITEM'S TYPE
	if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
	{
		ListView_SetItemText(m_hListCtrl, m_nCurrentItem, 3, L"folder");
		m_nCurrentItem++;
		return;
	}

	//in case it is a file, we write its extension
	ListView_SetItemText(m_hListCtrl, m_nCurrentItem, 3, wsFileExt);

	m_nCurrentItem++;
}

inline BOOL CSearchAppDlg::CheckFlags(const ULONG& ulFlags)
{
	//HIDDEN
	if (ulFlags & SFGAO_HIDDEN)
	{
		//m_ulEnumFlags specified that hidden items should be included!
		//if (!(m_Condition.dwAttribON & FILE_ATTRIBUTE_HIDDEN)) return false;
		return true;
	}
	else
	{
		//it must be set the search option for non-hidden items
		if (!(m_Condition.dwAttribOFF & FILE_ATTRIBUTE_HIDDEN)) return false;
	}

	//FOLDER
	if (ulFlags & SFGAO_FOLDER && ulFlags & SFGAO_FILESYSANCESTOR)
	{
		//it must be set the search option for directory items
		if (!(m_Condition.dwAttribON & FILE_ATTRIBUTE_DIRECTORY)) return false;
	}
	else
	{
		//it must be set the search option for non-directory items
		if (!(m_Condition.dwAttribOFF & FILE_ATTRIBUTE_DIRECTORY)) return false;
	}

	return true;
}

BOOL CSearchAppDlg::CheckAttributes(const WIN32_FILE_ATTRIBUTE_DATA& data)
{
	//NOW, WE CHECK SIZE & TIME.
	if (m_Condition.Size)
	{
		ULARGE_INTEGER liSize;
		liSize.HighPart = data.nFileSizeHigh;
		liSize.LowPart = data.nFileSizeLow;

		//the data size must be: cond.size[0] <= data.size <= cond.size[1]
		if (m_Condition.Size[0] > (QWORD)liSize.QuadPart) return false;
		if (m_Condition.Size[1] < (QWORD)liSize.QuadPart) return false;
	}

	//creation time
	if (m_Condition.ftCreationTime)
	{
		ULARGE_INTEGER liTime;
		liTime.HighPart = data.ftCreationTime.dwHighDateTime;
		liTime.LowPart = data.ftCreationTime.dwLowDateTime;

		//the data.time must be: cond.time[0] <= data.time <= cond.time[1]
		if (m_Condition.ftCreationTime[0] > (QWORD)liTime.QuadPart) return false;
		if (m_Condition.ftCreationTime[1] < (QWORD)liTime.QuadPart) return false;
	}

	//access time
	if (m_Condition.ftAccessTime)
	{
		ULARGE_INTEGER liTime;
		liTime.HighPart = data.ftLastAccessTime.dwHighDateTime;
		liTime.LowPart = data.ftLastAccessTime.dwLowDateTime;

		//the data.time must be: cond.time[0] <= data.time <= cond.time[1]
		if (m_Condition.ftAccessTime[0] > (QWORD)liTime.QuadPart) return false;
		if (m_Condition.ftAccessTime[1] < (QWORD)liTime.QuadPart) return false;
	}

	//modify time
	if (m_Condition.ftModifyTime)
	{
		ULARGE_INTEGER liTime;
		liTime.HighPart = data.ftLastWriteTime.dwHighDateTime;
		liTime.LowPart = data.ftLastWriteTime.dwLowDateTime;

		//the data.time must be: cond.time[0] <= data.time <= cond.time[1]
		if (m_Condition.ftModifyTime[0] > (QWORD)liTime.QuadPart) return false;
		if (m_Condition.ftModifyTime[1] < (QWORD)liTime.QuadPart) return false;
	}

	return true;
}

inline void CSearchAppDlg::GetDisplayName(const WCHAR* wsFullName, WCHAR** wsDisplayName)
{
	WCHAR* wPos = (WCHAR*)wcsrchr(wsFullName, '\\');
	//should never be NULL;
	_ASSERT(wPos);
	*wsDisplayName = ++wPos;
}

inline void CSearchAppDlg::GetFileExt(const WCHAR* wsDisplayName, WCHAR** wsFileExt, BOOL bRetrieveDot)
{
	WCHAR* wPos = (WCHAR*)wcsrchr(wsDisplayName, '.');
	if (wPos == NULL)
	{
		*wsFileExt = NULL;
		return;
	}

	if (bRetrieveDot)
	{
		*wsFileExt = wPos;
	}
	else
	{
		*wsFileExt = ++wPos;
	}
}

bool CSearchAppDlg::CheckDisplayName(WCHAR* wsDisplayName)
{
	WCHAR* wsSearch = m_wsSearchFileName;

	WCHAR *chFileDispName = NULL, *chSearch = NULL;

	//while we did not reach to the end (wsFileExt) with the wsDisplayName
	//and we did not reach '*' in wsSearch
	while ((*wsDisplayName) && (*wsSearch != '*'))
	{
		//if the two characters found are not the same
		//exception if wsSearch == '?' - in this case we continue;
		//else they are not the same.
		if ((*wsSearch != *wsDisplayName) && (*wsSearch != '?'))
		{
			return false;
		}
		//we step further with both strings.
		wsSearch++;
		wsDisplayName++;
	}

	//here we've met a '*' in wsSearch, or, we've reached the end in wsDisplayName.
	//while we're not at the end with wsDisplayName:
	while (*wsDisplayName)
	{
		//if we met a '*'
		if (*wsSearch == '*')
		{
			//if the following character after '*' is a NULL, we have a match!
			//that is, there is no point in checking the rest of wsDisplayName because all match '*'
			if (NULL == *++wsSearch)
			{
				return 1;
			}
			//if the following character after '*' is not null
			//we store the position of this '*' and go on with the characters of wsDisplayName
			chSearch = wsSearch;
			chFileDispName = wsDisplayName+1;
		}
		//the current character of wsSearch is not '*:
		//the two characters must match, unless the current character of wsSearch is '?'
		else if ((*wsSearch == *wsDisplayName) || (*wsSearch == '?'))
		{
			//if they do match, we step further with both strings
			wsSearch++;
			wsDisplayName++;
		}
		//the current character of wsSearch is not '*':
		//the two caracters do not math. in this case, we get back to the old positions
		//we step further with wsDisplayName, to see if that '*' found time ago means also the characters found until now. 
		else 
		{
			wsSearch = chSearch;
			wsDisplayName = chFileDispName++;
		}
	}

	//ok, we've reached the end with wsDisplayName
	//if the current character is '*', we step further until we find no more '*'-s
	while (*wsSearch == '*')
	{
		wsSearch++;
	}
	//there are two posibilities:
	//a. after '*' there is a character - in this case it returns NULL.
	//b. after '*' there is m_wsSearchFileExt - in this case, the two strings match.
	return !*wsSearch;
}

void CSearchAppDlg::CompareDisplayNames(CDoubleList<FILEITEM>::Iterator I, CDoubleList<FILEITEM>::Iterator J)
{
	FILEITEM item;
	WCHAR* wPos1, *wPos2;
	WCHAR u1 = 0, u2 = 0;

	wPos1 = I->m_Value.wsDisplayName;
	wPos2 = J->m_Value.wsDisplayName;
						
	do
	{
		u1 = towupper(*wPos1);
		u2 = towupper(*wPos2);
		if (u1 == 0)
		{
			if (u2 != 0)
			{
				//first is shorter
				u2 = 1;
				break;
			}
			//if (u1 ==0 && u2 == 0) goes to while
		}
		else
		{
			if (u2 == 0)
			{
				//second is shorter
				u1 = 1;
				break;
			}
			//if (u1 !=0 && u2 != 0) goes to while
		}

		wPos1++;
		wPos2++;
	} while (u1 == u2);

	if (u1 > u2)
	{
		item = I->m_Value;
		I->m_Value = J->m_Value;
		J->m_Value = item;
	}
}

void CSearchAppDlg::CompareFileTypes(CDoubleList<FILEITEM>::Iterator I, CDoubleList<FILEITEM>::Iterator J)
{
	FILEITEM item;
	WCHAR *wPos1, *wPos2;

	WCHAR u1 = 0, u2 = 0;

	wPos1 = I->m_Value.wsExt;
	wPos2 = J->m_Value.wsExt;

	if (wPos1 == NULL)
	{
		if (wPos2 == NULL)
		{
			//both are folders, or files without extension:

			//if the second is a file and the first is a folder
			if (J->m_Value.data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY && !(I->m_Value.data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			{
				item = I->m_Value;
				I->m_Value = J->m_Value;
				J->m_Value = item;
			}

			return;
		}
		else
		{
			//first is a folder/no ext file, second is a file with ext. we leave it so
			return;
		}
	}
	else
	{
		if (wPos2 == NULL)
		{
			//first is a file with ext, second is a folder/no ext file, we switch
			item = I->m_Value;
			I->m_Value = J->m_Value;
			J->m_Value = item;
			return;
		}
		//else: both are files with extensions, we check them below
	}

	//both are files with extensions here					
	do
	{
		u1 = towupper(*wPos1);
		u2 = towupper(*wPos2);
		if (u1 == 0)
		{
			if (u2 != 0)
			{
				//first is shorter
				u2 = 1;
				break;
			}
			else break;
			//if (u1 ==0 && u2 == 0) goes to while
		}
		else
		{
			if (u2 == 0)
			{
				//second is shorter
				u1 = 1;
				break;
			}
			//if (u1 !=0 && u2 != 0) goes to while
		}

		wPos1++;
		wPos2++;
	} while (u1 == u2);

	if (u1 > u2)
	{
		item = I->m_Value;
		I->m_Value = J->m_Value;
		J->m_Value = item;
	}
}

void CSearchAppDlg::ComparePathNames(CDoubleList<FILEITEM>::Iterator I, CDoubleList<FILEITEM>::Iterator J)
{
	FILEITEM item;
	WCHAR* wPos1, *wPos2;
	WCHAR u1 = 0, u2 = 0;

	//these are the folder names: after the null character we have the display names.
	wPos1 = I->m_Value.wsFullName;
	wPos2 = J->m_Value.wsFullName;
						
	do
	{
		u1 = towupper(*wPos1);
		u2 = towupper(*wPos2);
		if (u1 == 0)
		{
			if (u2 != 0)
			{
				//first is shorter
				u2 = 1;
				break;
			}
			//if (u1 ==0 && u2 == 0) goes to while
		}
		else
		{
			if (u2 == 0)
			{
				//second is shorter
				u1 = 1;
				break;
			}
			//if (u1 !=0 && u2 != 0) goes to while
		}

		wPos1++;
		wPos2++;
	} while (u1 == u2);

	if (u1 > u2)
	{
		item = I->m_Value;
		I->m_Value = J->m_Value;
		J->m_Value = item;
	}
}



void CSearchAppDlg::OnHeaderItemClicked(int iItem)
{
	//I MUST CHANGE SOMETHING, FOR REVERSE ORDER: DESCENDING, IF ALREADY ARRANGED ASCENDING

	switch (iItem)
	{
		//name
	case 0: 
		{
			if (m_dSorted == AscByName || m_dSorted == DescByName)
			{
				m_FileItems.reverse();
			}
			else
			{
				CDoubleList<FILEITEM>::Iterator I, J;
				for (I = m_FileItems.begin(); I != m_FileItems.end(); I = I->pNext)
				{
					for (J = I->pNext; J != NULL; J = J->pNext)
					{
						CompareDisplayNames(I, J);
					}
				}
			}

			//if the list was before sorted Ascending By Name, now it's Descending By Name
			if (m_dSorted == AscByName) m_dSorted = DescByName;
			//if the list was not sorted by name, or it was sorted Descending By Name, now it is Ascending By Name
			else m_dSorted = AscByName;
		}
		break;

		//path
	case 1: 
		{
			if (m_dSorted == AscByPath || m_dSorted == DescByPath)
			{
				m_FileItems.reverse();
			}
			else
			{
				//ok, it is not sorted by path:
				//we first sort it ascending by name

				if (m_dSorted == DescByName)
				{
					m_FileItems.reverse();
				}

				//if the list is already sorted ascending by name, we do not sort it again.
				//but if it's not sorted AscByName nor DescByName (if above) we sort it AscByName
				else if (m_dSorted != AscByName)
				{
					CDoubleList<FILEITEM>::Iterator I, J;
					for (I = m_FileItems.begin(); I != m_FileItems.end(); I = I->pNext)
					{
						for (J = I->pNext; J != NULL; J = J->pNext)
						{
							CompareDisplayNames(I, J);
						}
					}
				}

				//the actualy comparison by path
				CDoubleList<FILEITEM>::Iterator I, J;
				for (I = m_FileItems.begin(); I != m_FileItems.end(); I = I->pNext)
				{
					for (J = I->pNext; J != NULL; J = J->pNext)
					{
						ComparePathNames(I, J);
					}
				}
			}

			//if the list was before sorted Ascending By Path, now it's Descending By Path
			if (m_dSorted == AscByPath) m_dSorted = DescByPath;
			//if the list was not sorted by path, or it was sorted Descending By Path, now it is Ascending By Path
			else m_dSorted = AscByPath;
		}
		break;

		//date&time
	case 2:
		{
			if (m_bSearchFast) break;

			if (m_dSorted == AscByDate || m_dSorted == DescByDate)
			{
				m_FileItems.reverse();
			}
			else
			{
				//ok, it is not sorted by date:
				//we first sort it ascending by name

				if (m_dSorted == DescByName)
				{
					m_FileItems.reverse();
				}

				//if the list is already sorted ascending by name, we do not sort it again.
				//but if it's not sorted AscByName nor DescByName (if above) we sort it AscByName
				else if (m_dSorted != AscByName)
				{
					CDoubleList<FILEITEM>::Iterator I, J;
					for (I = m_FileItems.begin(); I != m_FileItems.end(); I = I->pNext)
					{
						for (J = I->pNext; J != NULL; J = J->pNext)
						{
							CompareDisplayNames(I, J);
						}
					}
				}

				//the actualy comparison by date
				FILEITEM item;
				CDoubleList<FILEITEM>::Iterator I, J;
				for (I = m_FileItems.begin(); I != m_FileItems.end(); I = I->pNext)
				{
					for (J = I->pNext; J != NULL; J = J->pNext)
					{
						ULARGE_INTEGER first, second;
						first.HighPart = I->m_Value.data.ftLastWriteTime.dwHighDateTime;
						first.LowPart = I->m_Value.data.ftLastWriteTime.dwLowDateTime;

						second.HighPart = J->m_Value.data.ftLastWriteTime.dwHighDateTime;
						second.LowPart = J->m_Value.data.ftLastWriteTime.dwLowDateTime;
						if (first.QuadPart > second.QuadPart)
						{
							item = I->m_Value;
							I->m_Value = J->m_Value;
							J->m_Value = item;
						}
					}
				}
			}

			//if the list was before sorted Ascending By Date, now it's Descending By Date
			if (m_dSorted == AscByDate) m_dSorted = DescByDate;
			//if the list was not sorted by Date, or it was sorted Descending By Date, now it is Ascending By Date
			else m_dSorted = AscByDate;
		}
		break;

		//type
	case 3:
		{
			if (m_bSearchFast) break;


			if (m_dSorted == AscByType || m_dSorted == DescByType)
			{
				m_FileItems.reverse();
			}
			else
			{
				//ok, it is not sorted by type:
				//we first sort it ascending by name

				if (m_dSorted == DescByName)
				{
					m_FileItems.reverse();
				}

				//if the list is already sorted ascending by name, we do not sort it again.
				//but if it's not sorted AscByName nor DescByName (if above) we sort it AscByName
				else if (m_dSorted != AscByName)
				{
					CDoubleList<FILEITEM>::Iterator I, J;
					for (I = m_FileItems.begin(); I != m_FileItems.end(); I = I->pNext)
					{
						for (J = I->pNext; J != NULL; J = J->pNext)
						{
							CompareDisplayNames(I, J);
						}
					}
				}

				//the actualy comparison by type
				CDoubleList<FILEITEM>::Iterator I, J;
				for (I = m_FileItems.begin(); I != m_FileItems.end(); I = I->pNext)
				{
					for (J = I->pNext; J != NULL; J = J->pNext)
					{
						CompareFileTypes(I, J);
					}
				}
			}

			//if the list was before sorted Ascending By Type, now it's Descending By Type
			if (m_dSorted == AscByType) m_dSorted = DescByType;
			//if the list was not sorted by type, or it was sorted Descending By Type, now it is Ascending By Type
			else m_dSorted = AscByType;
		}
		break;
	}

	//writing the results on the ListCtrl:
	CDoubleList<FILEITEM>::Iterator I;
	for (I = m_FileItems.begin(), m_nCurrentItem = 0; I != NULL; I = I->pNext, m_nCurrentItem++)
	{
		//display name
		LVITEM lvItem;
		lvItem.mask = LVIF_IMAGE | LVIF_TEXT | LVIF_PARAM;

		lvItem.iItem = m_nCurrentItem;
		lvItem.iSubItem = 0;
		lvItem.pszText = I->m_Value.wsDisplayName;
		lvItem.iImage = I->m_Value.dIcon;
		SendMessage(m_hListCtrl, LVM_SETITEM, 0, (LPARAM)&lvItem);

		//path
		ListView_SetItemText(m_hListCtrl, m_nCurrentItem, 1, I->m_Value.wsFullName);
		WCHAR wsTime[20];

		if (!m_bSearchFast)
		{
			//date&time
			SYSTEMTIME SystemTime = {0};
			_ASSERTE(FileTimeToSystemTime(&I->m_Value.data.ftLastWriteTime, &SystemTime));
	
			StringCchPrintfW(wsTime, 20, L"%02d.%02d.%d %02d:%02d", SystemTime.wDay, SystemTime.wMonth, SystemTime.wYear,
		SystemTime.wHour, SystemTime.wMinute);
			ListView_SetItemText(m_hListCtrl, m_nCurrentItem, 2, wsTime);

			//type
			if (I->m_Value.data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				ListView_SetItemText(m_hListCtrl, m_nCurrentItem, 3, L"folder");
			}
			else 
			{
				//in case it is a file, we write its extension
				ListView_SetItemText(m_hListCtrl, m_nCurrentItem, 3, I->m_Value.wsExt);
			}
		}
	}
}


void CSearchAppDlg::OnListItemContextMenu(int nItem, POINT& ptAction)
{
	//get the item by its ID
	FILEITEM curItem = m_FileItems[nItem];
	*(curItem.wsDisplayName - 1) = '\\';
	if (!PathFileExistsEx(curItem.wsFullName))
	{
		MessageBox(m_hWnd, L"This item does not exist - it has been deleted or moved.", 0, 0);
	}
	*(curItem.wsDisplayName - 1) = '\0';
	
	//GETTING THE PIDL AND ISHELLFOLDER OF THE CURRENT FOLDER
	IShellFolder* pDesktopFolder, *pCurrentFolder;
	ITEMIDLIST* pidlCurFolder;
	//getting the desktop folder
	if (false == GetShellFolder(curItem.wsFullName, &pDesktopFolder, &pCurrentFolder, &pidlCurFolder))
		return;

	//RETRIEVING THE PIDL OF THE CURRENT ITEM
	ITEMIDLIST* pidlCurItem, *pidlChildItem;
	*(curItem.wsDisplayName - 1) = '\\';
	pDesktopFolder->ParseDisplayName(m_hWnd, NULL, curItem.wsFullName, NULL, &pidlCurItem, NULL);
	pidlChildItem = ILFindLastID(pidlCurItem);

	//GETTING A LIST OF THE SELECTED ITEMS
	int nNrSel = SendMessage(m_hListCtrl, LVM_GETSELECTEDCOUNT, 0, 0);
	CDoubleList<ITEM> Items(NULL);
	int nSize;
	ITEMIDLIST** pidlChildren;
	FilterSelectedItems(pDesktopFolder, &Items, nSize, &pidlChildren);

	pDesktopFolder->Release();
	//-----------------------------------------------------------------------------

	int i;
	HMENU hMenu = CreatePopupMenu();
	IContextMenu* pCM;

	HRESULT hr = pCurrentFolder->GetUIObjectOf(m_hWnd, 1, (LPCITEMIDLIST*)&pidlChildItem, IID_IContextMenu, NULL, (void**)&pCM);
	if (FAILED(hr))
	{
		for (i = 0; i < nSize; i++)
			CoTaskMemFree(pidlChildren[i]);
		delete[] pidlChildren;
		CoTaskMemFree(pidlCurItem);
		CoTaskMemFree(pidlCurFolder);
		pCurrentFolder->Release();

		_ASSERT(0);
		MessageBox(0, L"Could not retrieve IContextMenu", 0, 0);
		return;
	}

	pCurrentFolder->Release();

	hr = pCM->QueryInterface(IID_IContextMenu3, (void**)&pContextMenu);
	if (FAILED(hr))
	{
		pCM->Release();
		for (i = 0; i < nSize; i++)
			CoTaskMemFree(pidlChildren[i]);
		delete[] pidlChildren;
		CoTaskMemFree(pidlCurItem);
		CoTaskMemFree(pidlCurFolder);

		_ASSERT(0);
		MessageBox(0, L"Could not query for IContextMenu3", 0, 0);
		return;
	}

	pCM->Release();

	//adding the items to the context menu
	hr = pContextMenu->QueryContextMenu(hMenu, 0, 1, 32767, CMF_NORMAL);
	if (FAILED(hr))
	{
		for (i = 0; i < nSize; i++)
			CoTaskMemFree(pidlChildren[i]);
		delete[] pidlChildren;
		CoTaskMemFree(pidlCurItem);
		CoTaskMemFree(pidlCurFolder);
		pContextMenu->Release();
		pContextMenu = NULL;

		_ASSERT(0);
		MessageBox(0, L"Could not query the Context Menu!", 0, 0);
		return;
	}

	//adding "Open Location"
	{
		MENUITEMINFO mii = {0};
		mii.cbSize = sizeof(mii);
		mii.fMask = MIIM_ID | MIIM_STRING | MIIM_FTYPE;
		mii.fType = MFT_STRING;
		mii.wID = 32768;
		mii.dwTypeData = L"Open Location";

		_ASSERT(InsertMenuItemW(hMenu, 1, TRUE, &mii));
	}

	//adding "Copy Path" and "Remove from the search list" and "Create shortcut to Desktop"
	int nrMenuItems = GetMenuItemCount(hMenu);
	for (int i = 0; i < nrMenuItems; i++)
	{
		int nID = GetMenuItemID(hMenu, i);
		if (nID == -1) continue;

		if (nID == 26)
		{
			MENUITEMINFO mii = {0};
			mii.cbSize = sizeof(mii);
			mii.fMask = MIIM_ID | MIIM_STRING | MIIM_FTYPE;
			mii.fType = MFT_STRING;
			mii.wID = 32769;
			mii.dwTypeData = L"Copy Path";

			_ASSERT(InsertMenuItemW(hMenu, i, TRUE, &mii));
			i++;
		}
		else if (nID == 18)
		{
			MENUITEMINFO mii = {0};
			mii.cbSize = sizeof(mii);
			mii.fMask = MIIM_ID | MIIM_STRING | MIIM_FTYPE;
			mii.fType = MFT_STRING;
			mii.wID = 32770;
			mii.dwTypeData = L"Remove from the search list";

			_ASSERT(InsertMenuItemW(hMenu, i, TRUE, &mii));
			i++;
		}
	}

	POINT loc = ptAction;
	ClientToScreen(m_hListCtrl, &loc);
	UINT nRet = TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD | TPM_LEFTBUTTON, loc.x, loc.y, NULL, m_hWnd, NULL);

	switch (nRet)
	{
	case 0 :
		{
			DWORD dwError = GetLastError();
			if (dwError != 0)
			{
				DisplayError(dwError);
			}
		}
		break;

	case 32768:
		//open location
		{
			//opens the location of a single item
			if (nNrSel == 1)
			{
				//select one item, if only one item
				hr = SHOpenFolderAndSelectItems(pidlCurFolder, 1, (LPCITEMIDLIST*)&pidlChildItem, 0);
				if (FAILED(hr))
				{
					MessageBox(0, L"Could not open the specified folder", L"Error!", 0);
				}
			}
			else
			{
				ShellExecuteW(m_hWnd, L"open", curItem.wsFullName, NULL, NULL, 1);
			}
		}
		break;

	case 32769:
		//copy path
		{
			//copies the path of a single item into the clipboard
			if (false == OpenClipboard(m_hWnd)) break;

			EmptyClipboard();

			*(curItem.wsDisplayName - 1) = '\\';
			int len = wcslen(curItem.wsFullName);
			len++;
			HANDLE hGlobal = GlobalAlloc(GMEM_FIXED, len * sizeof(WCHAR));
			WCHAR* wsText = (WCHAR*)GlobalLock(hGlobal);
			StringCchCopyW(wsText, len, curItem.wsFullName);
			GlobalUnlock(hGlobal);
			if (false == SetClipboardData(CF_UNICODETEXT, hGlobal))
			{
				DisplayError();
			}

			*(curItem.wsDisplayName - 1) = '\0';

			CloseClipboard();
		}
		break;

	case 32770:
		//remove from the search list
		{
			int nCurSel = SendMessage(m_hListCtrl, LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);

			CDoubleList<FILEITEM>::Iterator I;
			while (nCurSel != -1)
			{
				//getting each item bt ID
				if (TRUE == SendMessage(m_hListCtrl, LVM_DELETEITEM, nCurSel, 0))
				{
					I = m_FileItems.GetAt(nCurSel);
					m_FileItems.erase(I);

					nCurSel--;
				}

				nCurSel = SendMessage(m_hListCtrl, LVM_GETNEXTITEM, (WPARAM)nCurSel, LVNI_SELECTED);
			}
		}
		break;

	case 26:
		//copy files
		{
			CopyItems(Items);
		}
		break;

	case 25://25
		//cut
		{
			CutItems(Items);
		}
		break;

	case 18:
		//delete
		{
			OnDeleteItems(pidlChildren, nSize);
		}
		break;

	default:
		{
			nRet--;

			CMINVOKECOMMANDINFOEX cmdinfo = {0};
			cmdinfo.cbSize = sizeof(cmdinfo);
			cmdinfo.fMask = CMIC_MASK_NOASYNC;
			cmdinfo.lpVerb = (LPSTR)nRet;
			cmdinfo.nShow = 1;

			hr = pContextMenu->InvokeCommand((CMINVOKECOMMANDINFO*)&cmdinfo);
			if (FAILED(hr))
			{
				MessageBox(0, L"Cannot execute command!", 0, 0);
				break;
			}
		}
		break;
	}

	//now we have all items unique (not a file and the containg folder in the same list).
	CoTaskMemFree(pidlCurFolder);
	CoTaskMemFree(pidlCurItem);

	for (i = 0; i < nSize; i++)
		CoTaskMemFree(pidlChildren[i]);
	delete[] pidlChildren;

	pContextMenu->Release();
	pContextMenu = NULL;
}


void CSearchAppDlg::OnDeleteItems(ITEMIDLIST** pidlChildren, int nSize)
{
	IShellItemArray* pShellItemArray;
	HRESULT hr = SHCreateShellItemArrayFromIDLists(nSize, (LPCITEMIDLIST*)pidlChildren, &pShellItemArray);
	if (FAILED(hr))
	{
		MessageBox(0, L"Could not create the shell item array", 0, 0);
		return;
	}

	IFileOperation* pFileOperation;
	hr = CoCreateInstance(CLSID_FileOperation, NULL, CLSCTX_ALL, IID_IFileOperation, (VOID**)&pFileOperation);
	if (FAILED(hr))
	{
		MessageBox(0, L"Could not create IFileOperation object", 0, 0);
		pShellItemArray->Release();
		return;
	}

	IFileOperationProgressSink* pSink = new FileOperationProgressSink();

	DWORD dwToken;
	hr = pFileOperation->Advise(pSink, &dwToken);
	if (FAILED(hr))
	{
		MessageBox(0, L"could not advise", 0, 0);

		pFileOperation->Release();
		pShellItemArray->Release();
		return;
	}

	hr = pFileOperation->DeleteItems(pShellItemArray);
	if (FAILED(hr))
	{
		MessageBox(0, L"could not delete items", 0, 0);
		
		hr = pFileOperation->Unadvise(dwToken);
		if (FAILED(hr))
		{
			MessageBox(0, L"could not unadvise", 0, 0);
		}
		pFileOperation->Release();
		pShellItemArray->Release();
		return;
	}

	hr = pFileOperation->PerformOperations();
	if (FAILED(hr))
	{
		BOOL bAborted;
		HRESULT hr2 = pFileOperation->GetAnyOperationsAborted(&bAborted);
		if (FAILED(hr2))
		{
			MessageBox(0, L"could not perform operations; could not get bAborted. Re-searching data", 0, 0);
			OnSearch();
		}
		if (!bAborted)
		{
			MessageBox(0, L"could not perform operations. Re-searching data", 0, 0);
			OnSearch();
		}
	}

	hr = pFileOperation->Unadvise(dwToken);
	if (FAILED(hr))
	{
		MessageBox(0, L"could not unadvise", 0, 0);
	}


	pFileOperation->Release();
	pShellItemArray->Release();
}

void CSearchAppDlg::CutItems(CDoubleList<ITEM>& Items)
{
	IDataObject* pDataObject = new CDataObject();

	WCHAR* wsLarge = NULL;
	int len = 0;

	CDoubleList<ITEM>::Iterator I;
	for (I = Items.begin(); I != NULL; I = I->pNext)
	{
		*(I->m_Value.wsDisplayName - 1) = '\\';
		WCHAR* wsAux;
		if (wsLarge)
		{
			int oldlen = len;
			len += wcslen(I->m_Value.wsFullName) + 1;
			wsAux = new WCHAR[len];
			memcpy_s(wsAux, len * 2, wsLarge, len * 2);
			StringCchCopy(wsAux + oldlen, len, I->m_Value.wsFullName);
			delete[] wsLarge;
		}
		else
		{
			len = wcslen(I->m_Value.wsFullName) + 1;
			wsAux = new WCHAR[len];
			StringCchCopy(wsAux, len, I->m_Value.wsFullName);
		}
		wsLarge = wsAux;

		*(I->m_Value.wsDisplayName - 1) = '\0';
	}

	//appending a 0 (zero)
	len++;
	WCHAR* wsAux = new WCHAR[len];
	memcpy_s(wsAux, len * 2, wsLarge, len * 2);
	*(wsAux + len - 1) = 0;
	delete[] wsLarge;
	wsLarge = wsAux;

	DROPFILES drop;
	BYTE* lpGlobal;

	len *= 2;
	HANDLE hGlobal = GlobalAlloc(GMEM_FIXED, sizeof(drop) + len);
	lpGlobal = (BYTE*)GlobalLock(hGlobal);

	drop.pFiles = sizeof(drop);
	drop.fWide = TRUE;
	//copy drop to lpGlobal
	memcpy_s(lpGlobal, sizeof(drop), &drop, sizeof(drop));
	//copy the list (wstr)
	memcpy_s(lpGlobal + sizeof(drop), len, wsLarge, len);
	
	GlobalUnlock(hGlobal);
	delete[] wsLarge;

	FORMATETC formatetc;
	formatetc.cfFormat = CF_HDROP;
	formatetc.ptd = NULL;
	formatetc.dwAspect = DVASPECT_CONTENT;
	formatetc.lindex = -1;
	formatetc.tymed = TYMED_HGLOBAL;
	
	STGMEDIUM stgmedium;
	stgmedium.tymed = TYMED_HGLOBAL;
	stgmedium.pUnkForRelease = NULL;
	stgmedium.hGlobal = hGlobal;

	//data: CF_HDROP
	HRESULT hr = pDataObject->SetData(&formatetc, &stgmedium, TRUE);
	if (FAILED(hr))
	{
		MessageBox(0, L"Could not SetData", 0, 0);
		ReleaseStgMedium(&stgmedium);
		pDataObject->Release();
		return;
	}

	UINT uFormat = RegisterClipboardFormatW(CFSTR_PREFERREDDROPEFFECT);
	if (uFormat == 0)
	{
		DisplayError();
		GlobalFree(stgmedium.hGlobal);
		pDataObject->Release();
		return;
	}

	//drop effect:
	STGMEDIUM stgmedium2;
	formatetc.cfFormat = (CLIPFORMAT)uFormat;

	stgmedium2.tymed = TYMED_HGLOBAL;
	stgmedium2.pUnkForRelease = NULL;
	stgmedium2.hGlobal = GlobalAlloc(GMEM_FIXED, sizeof(DWORD));

	DWORD* pDropEffect = (DWORD*)GlobalLock(stgmedium2.hGlobal);
	*pDropEffect = DROPEFFECT_MOVE;
	GlobalUnlock(stgmedium2.hGlobal);

	//data: CFSTR_PREFERREDDROPEFFECT
	hr = pDataObject->SetData(&formatetc, &stgmedium2, TRUE);
	if (FAILED(hr))
	{
		MessageBox(0, L"Could not SetData", 0, 0);
		GlobalFree(stgmedium.hGlobal);
		GlobalFree(stgmedium2.hGlobal);
		pDataObject->Release();
		return;
	}

	hr = OleSetClipboard(pDataObject);
	if (FAILED(hr))
	{
		MessageBox(0, L"Could not SetClipboard", 0, 0);
		GlobalFree(stgmedium.hGlobal);
		GlobalFree(stgmedium2.hGlobal);
		pDataObject->Release();
		return;
	}

	hr = OleFlushClipboard();
	if (FAILED(hr))
	{
		MessageBox(0, L"Could not FlushClipboard", 0, 0);
	}

	GlobalFree(stgmedium.hGlobal);
	GlobalFree(stgmedium2.hGlobal);
}

void CSearchAppDlg::CopyItems(CDoubleList<ITEM>& Items)
{
	IDataObject* pDataObject = new CDataObject();
	pDataObject->AddRef();

	WCHAR* wsLarge = NULL;
	int len = 0;

	CDoubleList<ITEM>::Iterator I;
	for (I = Items.begin(); I != NULL; I = I->pNext)
	{
		*(I->m_Value.wsDisplayName - 1) = '\\';
		WCHAR* wsAux;
		if (wsLarge)
		{
			int oldlen = len;
			len += wcslen(I->m_Value.wsFullName) + 1;
			wsAux = new WCHAR[len];
			memcpy_s(wsAux, len * 2, wsLarge, len * 2);
			StringCchCopy(wsAux + oldlen, len, I->m_Value.wsFullName);
			delete[] wsLarge;
		}
		else
		{
			len = wcslen(I->m_Value.wsFullName) + 1;
			wsAux = new WCHAR[len];
			StringCchCopy(wsAux, len, I->m_Value.wsFullName);
		}
		wsLarge = wsAux;

		*(I->m_Value.wsDisplayName - 1) = '\0';
	}

	//appending a Zero
	len++;
	WCHAR* wsAux = new WCHAR[len];
	memcpy_s(wsAux, len * 2, wsLarge, len * 2);
	*(wsAux + len - 1) = 0;
	delete[] wsLarge;
	wsLarge = wsAux;

	DROPFILES drop;
	BYTE* lpGlobal;

	len *= 2;
	HANDLE hGlobal = GlobalAlloc(GMEM_FIXED, sizeof(drop) + len);
	lpGlobal = (BYTE*)GlobalLock(hGlobal);

	drop.pFiles = sizeof(drop);
	drop.fWide = TRUE;
	//copy drop to lpGlobal
	memcpy_s(lpGlobal, sizeof(drop), &drop, sizeof(drop));
	//copy the list (wstr)
	memcpy_s(lpGlobal + sizeof(drop), len, wsLarge, len);
	
	GlobalUnlock(hGlobal);
	delete[] wsLarge;

	FORMATETC formatetc;
	formatetc.cfFormat = CF_HDROP;
	formatetc.ptd = NULL;
	formatetc.dwAspect = DVASPECT_CONTENT;
	formatetc.lindex = -1;
	formatetc.tymed = TYMED_HGLOBAL;
	
	STGMEDIUM stgmedium;
	stgmedium.tymed = TYMED_HGLOBAL;
	stgmedium.pUnkForRelease = NULL;
	stgmedium.hGlobal = hGlobal;

	//data: CF_HDROP
	HRESULT hr = pDataObject->SetData(&formatetc, &stgmedium, TRUE);
	if (FAILED(hr))
	{
		MessageBox(0, L"Could not SetData", 0, 0);
		ReleaseStgMedium(&stgmedium);
		pDataObject->Release();
		return;
	}

	hr = OleSetClipboard(pDataObject);
	if (FAILED(hr))
	{
		MessageBox(0, L"Could not SetClipboard", 0, 0);
		ReleaseStgMedium(&stgmedium);
		pDataObject->Release();
		return;
	}

	hr = OleFlushClipboard();
	if (FAILED(hr))
	{
		MessageBox(0, L"Could not FlushClipboard", 0, 0);
	}

	ReleaseStgMedium(&stgmedium);
	pDataObject->Release();
}

void CSearchAppDlg::FilterSelectedItems(IShellFolder* pDesktopFolder, CDoubleList<ITEM>* Items, int& nSize, ITEMIDLIST*** Children)
{
	//if pItems == NULL, we must destroy pItems afterwards.
	BOOL bDestroy = !Items;
	if (bDestroy)
		Items = new CDoubleList<ITEM>(NULL);

	int nCurSel = SendMessage(m_hListCtrl, LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);
	while (nCurSel != -1)
	{
		//for every item, I need the full name (not folder name)
		FILEITEM fItem = m_FileItems[nCurSel];

		ITEM item;
		item.nIndex = nCurSel;

		//change from folder name to full name:
		*(fItem.wsDisplayName - 1) = '\\';
		item.wsFullName = fItem.wsFullName;
		item.len = wcslen(item.wsFullName);
		item.wsDisplayName = fItem.wsDisplayName;
		//I WILL DO THESE LATER:
		/**(fItem.wsDisplayName - 1) = '\0';*/

		Items->push_back(item);

		nCurSel = SendMessage(m_hListCtrl, LVM_GETNEXTITEM, nCurSel, LVNI_SELECTED);
	}

	//sorting the list ascending by folder path length.
	CDoubleList<ITEM>::Iterator I, J;
	for (I = Items->begin(); I != Items->end(); I = I->pNext)
	{
		ITEM aux;
		for (J = I->pNext; J != NULL; J = J->pNext)
		{
			if (I->m_Value.len > J->m_Value.len)
			{
				aux = I->m_Value;
				I->m_Value = J->m_Value;
				J->m_Value = aux;
			}
		}
	}

	//removing the files from this deque, if the files are already contained in the selected folders
	//searching to see if I strings are substrings of J strings
	for (I = Items->begin(); I != Items->end(); I = I->pNext)
	{
		for (J = I->pNext; J != NULL; )
		{
			WCHAR* wsResult = wcsstr(J->m_Value.wsFullName, I->m_Value.wsFullName);

			//the I str is a substring of J str, starting with the position wsResult in J str. we remove element J
			//the next character after substring must be '\\'
			if (wsResult && *(J->m_Value.wsFullName + wcslen(I->m_Value.wsFullName)) == '\\')
			{	
				*(J->m_Value.wsDisplayName - 1) = 0;
				Items->erase(J);
				if (I->pNext)
					continue;
				else {I = Items->end()->pPrev; break;}
			}
			else J = J->pNext;
		}
	}

	//creating the PIDLs
	nSize = Items->size();
	ITEMIDLIST** pidlChildren = NULL;
	if (Children)
	{
		pidlChildren = new ITEMIDLIST*[nSize];
		int i = 0;
		for (I = Items->begin(); I != NULL; I = I->pNext, i++)
		{
			pDesktopFolder->ParseDisplayName(m_hWnd, NULL, I->m_Value.wsFullName, NULL, &pidlChildren[i], NULL);
		}
	}
	
	//now change back from '\\' to '\0' for all.
	for (I = Items->begin(); I != NULL; I = I->pNext)
	{
		*(I->m_Value.wsDisplayName - 1) = '\0';
	}

	if (Children)
	{
		*Children = pidlChildren;
	}

	if (bDestroy) {delete Items; Items = NULL;}
}

BOOL CSearchAppDlg::GetShellFolder(WCHAR* wsFolderName, IShellFolder** pDesktopFolder, IShellFolder** pCurrentFolder, ITEMIDLIST** pidlCurFolder)
{
	IShellFolder* pDeskFolder, *pCurFolder;
	HRESULT hr = SHGetDesktopFolder(&pDeskFolder);
	if (FAILED(hr))
	{
		MessageBox(hMainDlg, L"Could not retrieve desktop folder", 0, 0);
		return false;
	}

	ITEMIDLIST* pidl;
	hr = pDeskFolder->ParseDisplayName(hMainDlg, NULL, wsFolderName, NULL, &pidl, NULL);
	if (FAILED(hr))
	{
		pDeskFolder->Release();
		MessageBox(hMainDlg, L"Could not retrieve PIDL of the parent folder", 0, 0);
		return false;
	}

	hr = pDeskFolder->BindToObject(pidl, NULL, IID_IShellFolder, (void**)&pCurFolder);
	if (FAILED(hr))
	{
		CoTaskMemFree(pidl);
		pDeskFolder->Release();
		MessageBox(hMainDlg, L"Could not retrieve IShellFolder of the parent folder", 0, 0);
		return false;
	}

	if (NULL == pidlCurFolder)
		CoTaskMemFree(pidl);
	else *pidlCurFolder = pidl;

	if (pCurrentFolder) *pCurrentFolder = pCurFolder;
	else pCurFolder->Release();

	if (pDesktopFolder) *pDesktopFolder = pDeskFolder;
	else pDeskFolder->Release();

	return true;
}

void CSearchAppDlg::OnPermanentDeleteItems(ITEMIDLIST** pidlChildren, int nSize)
{
	IShellItemArray* pShellItemArray;
	HRESULT hr = SHCreateShellItemArrayFromIDLists(nSize, (LPCITEMIDLIST*)pidlChildren, &pShellItemArray);
	if (FAILED(hr))
	{
		MessageBox(0, L"Could not create the shell item array", 0, 0);
		return;
	}

	IFileOperation* pFileOperation;
	hr = CoCreateInstance(CLSID_FileOperation, NULL, CLSCTX_ALL, IID_IFileOperation, (VOID**)&pFileOperation);
	if (FAILED(hr))
	{
		MessageBox(0, L"Could not create IFileOperation object", 0, 0);
		pShellItemArray->Release();
		return;
	}

	IFileOperationProgressSink* pSink = new FileOperationProgressSink();

	DWORD dwToken;
	hr = pFileOperation->Advise(pSink, &dwToken);
	if (FAILED(hr))
	{
		MessageBox(0, L"could not advise", 0, 0);

		pFileOperation->Release();
		pShellItemArray->Release();
		return;
	}

	hr = pFileOperation->DeleteItems(pShellItemArray);
	if (FAILED(hr))
	{
		MessageBox(0, L"could not delete items", 0, 0);
		
		hr = pFileOperation->Unadvise(dwToken);
		if (FAILED(hr))
		{
			MessageBox(0, L"could not unadvise", 0, 0);
		}
		pFileOperation->Release();
		pShellItemArray->Release();
		return;
	}

	hr = pFileOperation->SetOperationFlags(FOF_WANTNUKEWARNING);
	if (FAILED(hr))
	{
		MessageBox(0, L"could not set nuke warning flag", 0, 0);
	}

	hr = pFileOperation->PerformOperations();
	if (FAILED(hr))
	{
		BOOL bAborted;
		HRESULT hr2 = pFileOperation->GetAnyOperationsAborted(&bAborted);
		if (FAILED(hr2))
		{
			MessageBox(0, L"could not perform operations; could not get bAborted. Re-searching data", 0, 0);
			OnSearch();
		}
		if (!bAborted)
		{
			MessageBox(0, L"could not perform operations. Re-searching data", 0, 0);
			OnSearch();
		}
	}

	hr = pFileOperation->Unadvise(dwToken);
	if (FAILED(hr))
	{
		MessageBox(0, L"could not unadvise", 0, 0);
	}


	pFileOperation->Release();
	pShellItemArray->Release();
}

void CSearchAppDlg::UpdateCondition()
{
	//directories will be included
	if (BST_CHECKED == SendMessage(GetDlgItem(m_hWnd, IDC_ON_DIRECTORIES), BM_GETCHECK, 0, 0))
		m_Condition.dwAttribON |= FILE_ATTRIBUTE_DIRECTORY;
	//directories will not be included => files MUST be included
	else m_Condition.dwAttribON &= ~FILE_ATTRIBUTE_DIRECTORY;
	//files will be included
	if (BST_CHECKED == SendMessage(GetDlgItem(m_hWnd, IDC_OFF_DIRECTORIES), BM_GETCHECK, 0, 0))
		m_Condition.dwAttribOFF |= FILE_ATTRIBUTE_DIRECTORY;
	//files will not be included => directories MUST be included
	else m_Condition.dwAttribOFF &= ~FILE_ATTRIBUTE_DIRECTORY;

	//hidden items will be included
	if (BST_CHECKED == SendMessage(GetDlgItem(m_hWnd, IDC_ON_HIDDEN), BM_GETCHECK, 0, 0))
		m_Condition.dwAttribON |= FILE_ATTRIBUTE_HIDDEN;
	//hidden items will not be included => visible items MUST be included
	else m_Condition.dwAttribON &= ~FILE_ATTRIBUTE_HIDDEN;
	//visible items will be included
	if (BST_CHECKED == SendMessage(GetDlgItem(m_hWnd, IDC_OFF_HIDDEN), BM_GETCHECK, 0, 0))
		m_Condition.dwAttribOFF |= FILE_ATTRIBUTE_HIDDEN;
	//visible items will not be included => hidden items MUST be included
	else m_Condition.dwAttribOFF &= ~FILE_ATTRIBUTE_HIDDEN;
}

BOOL CSearchAppDlg::CheckSizes(void)
{
	//SIZE
	//size min
	int len = GetWindowTextLengthW(m_hSizeMin);
	len++;
	WCHAR* wsText = new WCHAR[len];
	GetWindowTextW(m_hSizeMin, wsText, len);
	if (false == CheckSizeText(wsText, len, m_SizeMin, true))
	{
		delete[] wsText;
		return false;
	}
	delete[] wsText;
	//size max
	len = GetWindowTextLengthW(m_hSizeMax);
	len++;
	wsText = new WCHAR[len];
	GetWindowTextW(m_hSizeMax, wsText, len);
	if (false == CheckSizeText(wsText, len, m_SizeMax, false))
	{
		delete[] wsText;
		return false;
	}
	delete[] wsText;

	if (m_SizeMin > m_SizeMax)
	{
		MessageBox(m_hWnd, L"The minimum size must be less or equal to the maximum size.", L"Error!", 0);
		return false;
	}

	return true;
}

BOOL CSearchAppDlg::GetNextQWORD(QWORD& nr, WCHAR** wPos)
{
	nr = 0;
	int count = 0;
	while (**wPos && **wPos >= '0' && **wPos <= '9')
	{
		count++;
		nr *= 10;
		nr += **wPos - '0';
		(*wPos)++;

		if (count > 16)
		{
			MessageBox(m_hWnd, L"Invalid Size: the number is too large", 0, 0);
			return FALSE;
		}
	}

	return TRUE;
}

void CSearchAppDlg::InitializeSizeBoxes(void)
{
	if (m_Condition.Size)
	{
		//MIN
		if (*(m_Condition.Size) == 0)
			SetWindowTextW(m_hSizeMin, L"0 B");
		else
		{
			QWORD Size = *m_Condition.Size;
			int sz = 0;
			//kb?
			if (Size == (Size >> 10) << 10)
			{
				Size = Size >> 10;
				sz = 1;
			
				//mb?
				if (Size == (Size >> 10) << 10)
				{
					Size = Size >> 10;
					sz = 2;
			
					//gb?
					if (Size == (Size >> 10) << 10) 
					{
						Size = Size >> 10;
						sz = 3;
					}
				}
			}

			WCHAR wstr[25];
			switch (sz)
			{
			case 0:
				StringCchPrintfW(wstr, 25, L"%u B", Size);
				break;

			case 1:
				StringCchPrintf(wstr, 25, L"%u KB", Size);
				break;

			case 2:
				StringCchPrintf(wstr, 25, L"%u MB", Size);
				break;

			case 3:
				StringCchPrintf(wstr, 25, L"%u GB", Size);
				break;
			}
			
			SetWindowTextW(m_hSizeMin, wstr);
		}

		//MAX
		if (*(m_Condition.Size + 1) == 0)
		{
			SetWindowTextW(m_hSizeMax, L"Any");
		}
		else
		{
			QWORD Size = *(m_Condition.Size + 1);
			int sz = 0;
			//kb?
			if (Size == (Size >> 10) << 10)
			{
				Size = Size >> 10;
				sz = 1;
			
				//mb?
				if (Size == (Size >> 10) << 10)
				{
					Size = Size >> 10;
					sz = 2;
			
					//gb?
					if (Size == (Size >> 10) << 10) 
					{
						Size = Size >> 10;
						sz = 3;
					}
				}
			}

			WCHAR wstr[25];
			switch (sz)
			{
			case 0:
				StringCchPrintf(wstr, 25, L"%u B", Size);
				break;

			case 1:
				StringCchPrintf(wstr, 25, L"%u KB", Size);
				break;

			case 2:
				StringCchPrintf(wstr, 25, L"%u MB", Size);
				break;

			case 3:
				StringCchPrintf(wstr, 25, L"%u GB", Size);
				break;
			}

			SetWindowTextW(m_hSizeMax, wstr);
		}
	}
	else
	{
		SetWindowTextW(m_hSizeMin, L"0 B");
		SetWindowTextW(m_hSizeMax, L"Any");
	}
}

BOOL CSearchAppDlg::CheckSizeText(WCHAR* wsText, int len, QWORD& qwSize, BOOL bIsMin)
{
	_wcslwr_s(wsText, len);

	WCHAR* wsResult = new WCHAR[len];
	WCHAR* wchRes = wsResult;

	//eliminating spaces (' ')
	for (WCHAR* wch = wsText; *wch != NULL; wch++)
	{
		if (*wch != ' ')
		{
			*wchRes = *wch;
			wchRes++;
		}
	}
	*wchRes = 0;

	//available strings:
	//any
	//any size
	if (iswalpha(*wsResult))
	{
		if (wcscmp(wsResult, L"any") == 0 || wcscmp(wsResult, L"anysize") == 0)
		{
			if (bIsMin) qwSize = 0;
			else qwSize = (QWORD)-1;
		}
		else
		{
			MessageBox(m_hWnd, L"Invalid size specified!", 0, 0);
			delete[] wsResult;
			return false;
		}
	}
	else if (*wsResult >= '0' && *wsResult <= '9')
	//[X] B/KB/MB/GB
	//[X]
	{
		WCHAR* wPos = wsResult;
		QWORD nr;
		if (false == GetNextQWORD(nr, &wPos))
		{
			delete[] wsResult;
			return false;
		}

		//if end of string was reached
		//[X]
		if (*wPos == NULL)
		{
			//the number is in bytes
			qwSize = nr;
		}

		//else we have a string, we get that string and compare it to B/KB/MB/GB
		//[X] B/KB/MB/GB
		else
		{
			KEYSIZE key;
			if (false == GetNextStringToSize(wPos, key))
			{
				MessageBox(m_hWnd, L"Invalid size specified!", 0, 0);
				delete[] wsResult;
				return false;
			}

			switch (key)
			{
			case CSearchAppDlg::Bytes:
				{
					qwSize = nr;
				}
				break;

			case CSearchAppDlg::KBytes:
				{
					if (nr > (((QWORD)-1) >> 10))
					{
						MessageBox(m_hWnd, L"The value for size is too much", 0, 0);
						delete[] wsResult;
						return false;
					}

					else
					{
						nr <<= 10;
						qwSize = nr;
					}
				}
				break;

			case CSearchAppDlg::MBytes:
				{
					if (nr > (((QWORD)-1) >> 20))
					{
						MessageBox(m_hWnd, L"The value for size is too much", 0, 0);
						delete[] wsResult;
						return false;
					}

					else
					{
						nr <<= 20;
						qwSize = nr;
					}
				}
				break;

			case CSearchAppDlg::GBytes:
				{
					if (nr > (((QWORD)-1) >> 30))
					{
						MessageBox(m_hWnd, L"The value for size is too much", 0, 0);
						delete[] wsResult;
						return false;
					}

					else
					{
						nr <<= 30;
						qwSize = nr;
					}
				}
				break;

			default:
				{
					MessageBox(m_hWnd, L"Invalid size specified", 0, 0);
					delete[] wsResult;
					return false;
				}

				break;
			}

			if (*wPos)
			{
				MessageBox(m_hWnd, L"Invalid size specified", 0, 0);
				delete[] wsResult;
				return false;
			}
		}
	}
	else
	{
		MessageBox(m_hWnd, L"Invalid size specified!", 0, 0);
		delete[] wsResult;
		return false;
	}

	delete[] wsResult;

	return TRUE;
}

BOOL CSearchAppDlg::GetNextStringToSize(WCHAR*& wPos, KEYSIZE& key)
{
	WCHAR* wsStart = wPos;

	while (wPos && iswalpha(*wPos))
	{
		wPos++;
	}

	//january; february; march; april; may; june; july; august; september; october; november; december;
	//monday; tuesday; wednesday; thursday; friday; saturday; sunday
	//today; yesterday;
	//last:
	//hour; day; week; month; year;
	//anytime

	WCHAR wsKeys[][3] = {L"b", L"kb", L"mb", L"gb"};

	int checked[] = {0, 0, 0, 0};

	//compare the string from wsStart until wPos, to see if it is one of the keywords:
	//i = position in the string
	int i = 0, lastvalid = -1;
	for (WCHAR* wch = wsStart; wch != wPos; wch++, i++)
	{
		//j = the ordinal (ID) of the string
		for (int j = 0; j < 4; j++)
		{
			//if it is still valid
			if (*(checked + j) > -1)
			{
				//compare with the coresponding string:
				if (*(*(wsKeys + j) + i) != *wch)
					*(checked + j) = -1;
				else lastvalid = j;
			}
		}

		if (lastvalid == -1) break;
	}

	if (lastvalid > -1)
	{
		if (i < (int)wcslen(*(wsKeys + lastvalid))) *(checked + lastvalid) = - 1;
		if (*(checked + lastvalid) == - 1) lastvalid = 500;
	}

	//only one must be valid now: lastvalid
	if (lastvalid <= GBytes)
		key = (KEYSIZE)lastvalid;
	else return false;

	return true;
}

void CSearchAppDlg::WatchFolder(ITEMIDLIST* pidlFolder)
{
	SHChangeNotifyEntry entries[1] = {pidlFolder, TRUE};
	//perhaps I should remove shell level
	m_uRegisteredValue = SHChangeNotifyRegister(m_hWnd, SHCNRF_ShellLevel | SHCNRF_InterruptLevel | SHCNRF_NewDelivery,
SHCNE_ATTRIBUTES | SHCNE_DELETE | SHCNE_RMDIR | SHCNE_RENAMEFOLDER | SHCNE_RENAMEITEM,
WM_SHELLNOTIFY, ARRAYSIZE(entries), entries);

	if (false == m_uRegisteredValue)
	{
		MessageBox(m_hWnd, L"Could not register shell notification", 0, 0);
		return;
	}
}

void CSearchAppDlg::OnShellNotify(WPARAM wParam, LPARAM lParam)
{
	PIDLIST_ABSOLUTE* pidlList;
	LONG lEvent;
	HANDLE hLock = SHChangeNotification_Lock((HANDLE)wParam, (DWORD)lParam, &pidlList, &lEvent);
	if (hLock)
	{
		WCHAR* wsName1 = NULL, *wsName2 = NULL;
		if (pidlList[0])
		{
			IShellItem* pShellItem;
			HRESULT hr = SHCreateItemFromIDList(pidlList[0], IID_IShellItem, (void**)&pShellItem);
			hr = pShellItem->GetDisplayName(SIGDN_FILESYSPATH, &wsName1);
			pShellItem->Release();
		}

		if (pidlList[1])
		{
			IShellItem* pShellItem;
			HRESULT hr = SHCreateItemFromIDList(pidlList[1], IID_IShellItem, (void**)&pShellItem);
			hr = pShellItem->GetDisplayName(SIGDN_FILESYSPATH, &wsName2);
			pShellItem->Release();
		}

		switch (lEvent)
		{
			//file deleted
		case SHCNE_DELETE:
			{
				OnNotifyDeleteFile(wsName1);
			}
			break;

			//folder deleted
		case SHCNE_RMDIR:
			{
				OnNotifyDeleteFolder(wsName1);
			}
			break;

			//file renamed
		case SHCNE_RENAMEITEM:
			{
				OnNotifyRenameFile(wsName1, wsName2);
			}
			break;

			//folder renamed
		case SHCNE_RENAMEFOLDER:
			{
				OnNotifyRenameFolder(wsName1, wsName2);
			}
			break;
		}

		if (wsName1)
			CoTaskMemFree(wsName1);
		if (wsName2)
			CoTaskMemFree(wsName2);

		SHChangeNotification_Unlock(hLock);
	}
}

//file deleted
void CSearchAppDlg::OnNotifyDeleteFile(WCHAR* wsFileName)
{
	//seek the filename in the list
	CDoubleList<FILEITEM>::Iterator I;
	
	int i = 0;
	for (I = m_FileItems.begin(); I != NULL; I = I->pNext, i++)
	{
		*(I->m_Value.wsDisplayName-1) = '\\';
		if (_wcsicmp(I->m_Value.wsFullName, wsFileName) == 0)
		{
			//remove from the list
			if (TRUE == SendMessage(m_hListCtrl, LVM_DELETEITEM, i, 0))
			{
				m_FileItems.erase(I);
				break;
			}
		}
		*(I->m_Value.wsDisplayName-1) = '\0';
	}
}

//folder deleted
void CSearchAppDlg::OnNotifyDeleteFolder(WCHAR* wsFolderName)
{
	//seek the filename in the list
	CDoubleList<FILEITEM>::Iterator I;
	WCHAR* wsFolder = NULL;
	
	int i = 0;
	for (I = m_FileItems.begin(); I != NULL; I = I->pNext, i++)
	{
		*(I->m_Value.wsDisplayName-1) = '\\';
		if (_wcsicmp(I->m_Value.wsFullName, wsFolderName) == 0)
		{
			int len = wcslen(I->m_Value.wsFullName) + 1;
			wsFolder = new WCHAR[len];
			StringCchCopyW(wsFolder, len, I->m_Value.wsFullName);
			//I->m_Value.wsFullName is the folder!!
			//remove from the list
			if (TRUE == SendMessage(m_hListCtrl, LVM_DELETEITEM, i, 0))
			{
				m_FileItems.erase(I);
				break;
			}
		}
		*(I->m_Value.wsDisplayName-1) = '\0';
	}

	if (wsFolder)
	{
		i = 0;
		for (I = m_FileItems.begin(); I != NULL;)
		{
			*(I->m_Value.wsDisplayName-1) = '\\';
			if (wcsstr(I->m_Value.wsFullName, wsFolder))
			{
				//remove from the list
				if (TRUE == SendMessage(m_hListCtrl, LVM_DELETEITEM, i, 0))
				{
					m_FileItems.erase(I);
				}
			}
			else
			{
				*(I->m_Value.wsDisplayName-1) = '\0';
				I = I->pNext;
				i++;
			}
		}

		delete[] wsFolder;
	}
}
	
//file renamed
void CSearchAppDlg::OnNotifyRenameFile(WCHAR* wsOldFileName, WCHAR* wsNewFileName)
{
	//seek the filename in the list
	CDoubleList<FILEITEM>::Iterator I;
	
	int i = 0;
	for (I = m_FileItems.begin(); I != NULL; I = I->pNext, i++)
	{
		*(I->m_Value.wsDisplayName-1) = '\\';
		if (_wcsicmp(I->m_Value.wsFullName, wsOldFileName) == 0)
		{
			//rename:
			CoTaskMemFree(I->m_Value.wsFullName);
			int len = (wcslen(wsNewFileName) + 1) * 2;
			I->m_Value.wsFullName = (WCHAR*)CoTaskMemAlloc(len);
			StringCchCopyW(I->m_Value.wsFullName, len, wsNewFileName);
			//retrieving the display name:
			GetDisplayName(I->m_Value.wsFullName, &I->m_Value.wsDisplayName);
			//retrieving the file extension:
			GetFileExt(I->m_Value.wsDisplayName, &I->m_Value.wsExt, FALSE);
			//setting the zero
			*(I->m_Value.wsDisplayName - 1) = 0;

			//setting the displayname:
			ListView_SetItemText(m_hListCtrl, i, 0, I->m_Value.wsDisplayName);
			break;
		}
		*(I->m_Value.wsDisplayName-1) = '\0';
	}
}

//folder renamed
void CSearchAppDlg::OnNotifyRenameFolder(WCHAR* wsOldFolderName, WCHAR* wsNewFolderName)
{
	//seek the filename in the list
	CDoubleList<FILEITEM>::Iterator I;
	WCHAR* wsFolder = NULL;
	
	int i = 0;
	for (I = m_FileItems.begin(); I != NULL; I = I->pNext, i++)
	{
		*(I->m_Value.wsDisplayName-1) = '\\';
		if (_wcsicmp(I->m_Value.wsFullName, wsOldFolderName) == 0)
		{
			//rename:
			wsFolder = I->m_Value.wsFullName;
			int len = (wcslen(wsNewFolderName) + 1) * 2;
			I->m_Value.wsFullName = (WCHAR*)CoTaskMemAlloc(len);
			StringCchCopyW(I->m_Value.wsFullName, len, wsNewFolderName);
			//retrieving the display name:
			GetDisplayName(I->m_Value.wsFullName, &I->m_Value.wsDisplayName);
			//setting the zero
			*(I->m_Value.wsDisplayName - 1) = 0;
			//retrieving the file extension:
			GetFileExt(I->m_Value.wsDisplayName, &I->m_Value.wsExt, FALSE);

			//setting the displayname:
			ListView_SetItemText(m_hListCtrl, i, 0, I->m_Value.wsDisplayName);
			break;
		}
		*(I->m_Value.wsDisplayName-1) = '\0';
	}

	if (wsFolder)
	{
		i = 0;
		for (I = m_FileItems.begin(); I != NULL; I = I->pNext, i++)
		{
			*(I->m_Value.wsDisplayName-1) = '\\';
			WCHAR* wsResult = wcsstr(I->m_Value.wsFullName, wsFolder);
			if (wsResult && *(I->m_Value.wsFullName + wcslen(wsFolder)) == '\\')
			{
				//rename:
				//1. getting the display name:
				int len = wcslen(I->m_Value.wsDisplayName) + 1;
				WCHAR* wsDisplayName = new WCHAR[len];
				StringCchCopyW(wsDisplayName, len, I->m_Value.wsDisplayName);
				
				//2. deleting old full display name
				CoTaskMemFree(I->m_Value.wsFullName);

				//3. allocating the new full display name
				len = (wcslen(wsNewFolderName) + 1 + len) * 2;
				I->m_Value.wsFullName = (WCHAR*)CoTaskMemAlloc(len);
				//4. copying the folder's full name
				StringCchCopyW(I->m_Value.wsFullName, len, wsNewFolderName);
				//5. appending the '\\'
				StringCchCatW(I->m_Value.wsFullName, len, L"\\");
				//6. appending the display name
				StringCchCatW(I->m_Value.wsFullName, len, wsDisplayName);
				//7. deleting the unneeded display name
				delete[] wsDisplayName;

				//retrieving the display name:
				GetDisplayName(I->m_Value.wsFullName, &I->m_Value.wsDisplayName);
				//lowering the display name, if needed:
				if (!m_Condition.m_bCSFileName)
#pragma warning (suppress: 4996)
					_wcslwr(I->m_Value.wsDisplayName);
				//setting the '\0'
				*(I->m_Value.wsDisplayName - 1) = 0;
				//retrieving the file extension:
				GetFileExt(I->m_Value.wsDisplayName, &I->m_Value.wsExt, FALSE);

				//setting the displayname:
				ListView_SetItemText(m_hListCtrl, i, 1, I->m_Value.wsFullName);
			}
			else
			{
				*(I->m_Value.wsDisplayName-1) = '\0';
			}
		}

		CoTaskMemFree(wsFolder);
	}
}

inline BOOL CSearchAppDlg::CheckFileContents(const WCHAR* wsFullName, const WCHAR* wsExtName, WIN32_FILE_ATTRIBUTE_DATA& data)
{
	IClassFactory* pClassFactory = FindFilter(wsExtName);
	if (pClassFactory)
		return SearchUsingFilter(wsFullName, m_wsSearchContents, pClassFactory, data);

	HANDLE hFile = CreateFileW(wsFullName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, 0);
	if (INVALID_HANDLE_VALUE == hFile) return FALSE;

	BOOL bResult = FALSE;
	LARGE_INTEGER liSize;
	liSize.HighPart = data.nFileSizeHigh;
	liSize.LowPart = data.nFileSizeLow;

	bResult = CheckDefaultContents(hFile, liSize);

	CloseHandle(hFile);
	return bResult;
}

#define ENC_INVALID		0

#define ENC_ASCII		1
#define ENC_UTF16LE		2
#define ENC_UTF16BE		4
#define ENC_UTF8		8

#define ENC_ALL			15 //ENC_ASCII | ENC_UTF16LE | ENC_UTF16BE | ENC_UTF8

#define NRENC_ASCII			0
#define NRENC_UTF16LE		1
#define NRENC_UTF16BE		2
#define NRENC_UTF8			3

#define NR_ENCODINGS		4

BOOL CSearchAppDlg::CheckDefaultContents(HANDLE hFile, LARGE_INTEGER& liSize)
{
	BYTE* Buffer;
	DWORD dwRead;
	int pointer = 0;
	int length = wcslen(m_wsSearchContents);

	DWORD dwLastEnc = ENC_ALL;

	DWORD dwSzReq = /*100*/0x1400000;

	//0x1400000 = 20 MB
	if (liSize.QuadPart < dwSzReq)
	{
		Buffer = new BYTE[liSize.LowPart];
		if (false == ReadFile(hFile, Buffer, liSize.LowPart, &dwRead, NULL))
			return FALSE;

		//checking each byte
		DWORD dwEnc;
		DWORD iFirstFound = (DWORD)-1;

		int nr[NR_ENCODINGS] = {0, 0, 0, 0};
		for (DWORD i = 0; i < dwRead; i++)
		{
			//retrieve the next char available encodings, and the number of character it needs for each encoding.
			if (false == GetNextChar(Buffer + i, dwEnc, dwRead - i, nr))
			{
				pointer = 0;
				dwLastEnc = ENC_ALL;
				continue;
			}

			//if we found already elements within our search string of certain encodings, it must mach our current encoding
			else if ((dwEnc = dwEnc & dwLastEnc) == 0)
			{
				if (pointer)
				{
					if (iFirstFound == (DWORD)-1)
					{
						i = 0;
					}

					else
					{
						i = iFirstFound;
						iFirstFound = (DWORD)-1;
					}
				}

				pointer = 0;
				dwLastEnc = ENC_ALL;
				continue;
			}
			
			else
			{
				//we retrieve each char as its encodings and compare it to the current character in the search str.
				//the encodings that fail will have the "nr" set to 0.
				BOOL bResult = CharCheckEnc(Buffer + i, (BYTE*)(m_wsSearchContents + pointer), dwEnc, nr, (length - pointer) << 1);
				if (false == bResult)
				{
					if (pointer)
					{
						if (iFirstFound == (DWORD)-1)
						{
							i = 0;
						}

						else
						{
							i = iFirstFound;
							iFirstFound = (DWORD)-1;
						}
					}

					pointer = 0;
					dwLastEnc = ENC_ALL;
				}
				else
				{
					if (iFirstFound == (DWORD)-1)
						iFirstFound = i;
					pointer++;
					//is this current wchar a surrogate?
					if (bResult == 4) pointer++;

					i += (--bResult);
					dwLastEnc = dwEnc;

					if (pointer == length)
					{
						delete[] Buffer;
						return TRUE;
					}
				}
			}
		}
		delete[] Buffer;
	}

	else
	{
		QWORD qwTotalRead = 0, qwSizeFileLeft = 0;
		Buffer = new BYTE[dwSzReq];
		BYTE* LastPart = NULL;
		DWORD nLastPartSize = 0;
		BOOL bRead = TRUE;
		DWORD BeginI = 0;
		while (bRead)
		{
			if (LastPart)
			{
				memcpy_s(Buffer, dwSzReq, LastPart, nLastPartSize);
				delete[] LastPart;
				LastPart = NULL;
			}

			bRead = ReadFile(hFile, Buffer + nLastPartSize, dwSzReq - nLastPartSize, &dwRead, NULL);
			if (0 == dwRead) break;
			if (false == bRead) break;
			qwTotalRead += dwRead;
			dwRead += nLastPartSize;

			//checking each byte
			DWORD dwEnc;
			BOOL bNeedMore;
			DWORD iFirstFound = (DWORD)-1;

			int nr[NR_ENCODINGS] = {0, 0, 0, 0};
			for (DWORD i = BeginI; i < dwRead; i++)
			{
				qwSizeFileLeft = liSize.QuadPart - qwTotalRead + dwRead - i;
				//retrieve the next char available encodings, and the number of character it needs for each encoding.
				if (false == GetNextChar(Buffer + i, dwEnc, dwRead - i, nr, bNeedMore, qwSizeFileLeft))
				{
					if (bNeedMore)
					{
						//a second buffer is needed to store the last part of the buffer (beggining with iFirstFound)
						if (iFirstFound != (DWORD)-1)
						{
							nLastPartSize = dwRead - iFirstFound;
							//BeginI = i - iFirstFound;

							if (LastPart) delete[] LastPart;
							LastPart = new BYTE[nLastPartSize];
							memcpy_s(LastPart, nLastPartSize, Buffer + iFirstFound, nLastPartSize);
						}

						else
						{
							nLastPartSize = dwRead - i;
							//BeginI = 0;

							if (LastPart) delete[] LastPart;
							LastPart = new BYTE[nLastPartSize];
							memcpy_s(LastPart, nLastPartSize, Buffer + i, nLastPartSize);
						}
						//break;
						pointer = 0;
						dwLastEnc = ENC_ALL;
						continue;
					}

					pointer = 0;
					dwLastEnc = ENC_ALL;
					continue;
				}

				//if we found already elements within our search string of certain encodings, it must mach our current encoding
				else if ((dwEnc = dwEnc & dwLastEnc) == 0)
				{
					if (pointer)
					{
						if (iFirstFound == (DWORD)-1)
						{
							i = 0;
						}

						else
						{
							i = iFirstFound;
							iFirstFound = (DWORD)-1;
						}
					}

					pointer = 0;
					dwLastEnc = ENC_ALL;
					continue;
				}
				
				else
				{
					//we retrieve each char as its encodings and compare it to the current character in the search str.
					//the encodings that fail will have the "nr" set to 0.
					BOOL bResult = CharCheckEnc(Buffer + i, (BYTE*)(m_wsSearchContents + pointer), dwEnc, nr, (length - pointer) << 1);
					if (false == bResult)
					{
						if (pointer)
						{
							if (iFirstFound == (DWORD)-1)
							{
								i = 0;
							}

							else
							{
								i = iFirstFound;
								iFirstFound = (DWORD)-1;
							}
						}

						pointer = 0;
						dwLastEnc = ENC_ALL;
					}
					else
					{
						if (iFirstFound == (DWORD)-1)
							iFirstFound = i;
						pointer++;
						//is this current wchar a surrogate?
						if (bResult == 4) pointer++;

						i += (--bResult);
						dwLastEnc = dwEnc;

						if (pointer == length)
						{
							delete[] Buffer;
							return TRUE;
						}
					}
				}
			}

			if (!bNeedMore)
			{
				//a second buffer is needed to store the last part of the buffer (beggining with iFirstFound)
				if (iFirstFound != (DWORD)-1)
				{
					nLastPartSize = dwRead - iFirstFound;
					//BeginI = dwRead - iFirstFound;

					if (LastPart) delete[] LastPart;
					LastPart = new BYTE[nLastPartSize];
					memcpy_s(LastPart, nLastPartSize, Buffer + iFirstFound, nLastPartSize);
				}

				else
				{
					nLastPartSize = 0;
					//BeginI = 0;
				}

				pointer = 0;
				dwLastEnc = ENC_ALL;
			}
		}
		delete[] Buffer;
		if (LastPart) delete[] LastPart;
	}

	return FALSE;
}

BOOL CSearchAppDlg::GetNextChar(const BYTE* wPos, DWORD& dwEnc, DWORD nMax, int nr[])
{
	dwEnc = ENC_ALL;

	//check for UTF-8:----------------------------------------------->UTF8<----------------------------------------
	if (*wPos >= 0xF0)
	{
		//UTF-8 4 bytes:
		if (*wPos > 0xF4) dwEnc &= ~ENC_UTF8;
		else if (nMax < 4) dwEnc &= ~ENC_UTF8;

		else //nMax >= 4, so we can check the other THREE bytes
		{
			if (*wPos == 0xF0)
			{
				//the other 3 bytes should be: 90->BF 80->BF 80->BF
				if (!(*(wPos + 1) >= 0x90 && *(wPos + 1) <= 0xBF &&
					*(wPos + 2) >= 0x80 && *(wPos + 2) <= 0xBF &&
					*(wPos + 3) >= 0x80 && *(wPos + 3) <= 0xBF)) dwEnc &= ~ENC_UTF8;
				else nr[NRENC_UTF8] = 4;
			}

			else if (*wPos >= 0xF1 && *wPos <= 0xF3)
			{
				//the other 3 bytes should be: 80->BF 80->BF 80->BF
				if (!(*(wPos + 1) >= 0x80 && *(wPos + 1) <= 0xBF &&
					*(wPos + 2) >= 0x80 && *(wPos + 2) <= 0xBF &&
					*(wPos + 3) >= 0x80 && *(wPos + 3) <= 0xBF)) dwEnc &= ~ENC_UTF8;
				else nr[NRENC_UTF8] = 4;
			}

			else if (*wPos == 0xF4)
			{
				//the other 3 bytes should be: 80->8F 80->BF 80->BF
				if (!(*(wPos + 1) >= 0x80 && *(wPos + 1) <= 0x8F &&
					*(wPos + 2) >= 0x80 && *(wPos + 2) <= 0xBF &&
					*(wPos + 3) >= 0x80 && *(wPos + 3) <= 0xBF)) dwEnc &= ~ENC_UTF8;
				else nr[NRENC_UTF8] = 4;
			}
		}
	}

	else if (*wPos >= 0xE0)
	{
		//UTF-8 3 bytes:
		if (*wPos > 0xEF) dwEnc &= ~ENC_UTF8;
		else if (nMax < 3) dwEnc &= ~ENC_UTF8;

		else
		{
			//nMax >= 3, so we can check the other TWO bytes
			if (*wPos == 0xE0)
			{
				//the other 2 bytes should be: A0->BF 80->BF
				if (!(*(wPos + 1) >= 0xA0 && *(wPos + 1) <= 0xBF &&
					*(wPos + 2) >= 0x80 && *(wPos + 2) <= 0xBF)) dwEnc &= ~ENC_UTF8;
				else nr[NRENC_UTF8] = 3;
			}
			
			else if (*wPos >= 0xE1 && *wPos <= 0xEC)
			{
				//the other 2 bytes should be: 80->BF 80->BF
				if (!(*(wPos + 1) >= 0x80 && *(wPos + 1) <= 0xBF &&
					*(wPos + 2) >= 0x80 && *(wPos + 2) <= 0xBF)) dwEnc &= ~ENC_UTF8;
				else nr[NRENC_UTF8] = 3;
			}

			else if (*wPos == 0xED)
			{
				//the other 2 bytes should be: 80->9F 80->BF
				if (!(*(wPos + 1) >= 0x80 && *(wPos + 1) <= 0x9F &&
					*(wPos + 2) >= 0x80 && *(wPos + 2) <= 0xBF)) dwEnc &= ~ENC_UTF8;
				else nr[NRENC_UTF8] = 3;
			}

			else if (*wPos >= 0xEE && *wPos <= 0xEF)
			{
				//the other 2 bytes should be: 80->BF 80->BF
				if (!(*(wPos + 1) >= 0x80 && *(wPos + 1) <= 0xBF &&
					*(wPos + 2) >= 0x80 && *(wPos + 2) <= 0xBF)) dwEnc &= ~ENC_UTF8;
				else nr[NRENC_UTF8] = 3;
			}
		}
	}

	else if (*wPos >= 0xC2)
	{
		//UTF-8 2 bytes:
		if (*wPos > 0xDF) dwEnc &= ~ENC_UTF8;
		else if (nMax < 2) dwEnc &= ~ENC_UTF8;

		else //nMax >= 2, so we can check the other ONE byte
		{
			//the other 1 byte should be: 80->BF
			if (!(*(wPos + 1) >= 0x80 && *(wPos + 1) <= 0xBF)) dwEnc &= ~ENC_UTF8;
			else nr[NRENC_UTF8] = 2;
		}
	}

	else if (*wPos > 0x7F)
	{
		//non-utf-8 (0x7F < *wPos < 0xC2)
		dwEnc &= ~ENC_UTF8;
	}
	else nr[NRENC_UTF8] = 1;

	//check for UTF-16LE:----------------------------------------------->UTF16-LE<-------------------------------------
	if (dwEnc & ENC_UTF16LE)
	{
		//FF FE == 0xFEFF (HI = FE (second); LO = FF (first))
		if (nMax < 4)
		{
			//should not be surogates:
			//makeword(low, high)
			//WORD val = MAKEWORD(*wPos, *(wPos + 1));
			//if (val > 0xD7FF)//surrogate!
			//	dwEnc &= ~ENC_UTF16LE;
			/*else */nr[NRENC_UTF16LE] = 2;
		}

		else //nMax >= 4
		{
			//WORD val = MAKEWORD(*wPos, *(wPos + 1));
			//if (val > 0xD7FF)//surrogate!
			//{
			//	//high surrogate must be in the range: U+D800 to U+DBFF
			//	if (val > 0xDBFF)//invalid for HIGH SUROGATE!
			//	{
			//		dwEnc &= ~ENC_UTF16LE;
			//	}
			//	else
			//	{
			//		WORD val2 = MAKEWORD(*(wPos + 2), *(wPos + 3));
			//		//low surrogate must be in the range: U+DC00 to U+DFFF
			//		if (!(val2 >= 0xDC00 && val2 <= 0xDFFF))
			//			dwEnc &= ~ENC_UTF16LE;
			//		else nr[NRENC_UTF16LE] = 4;
			//	}
			//}
			//else <= U+D7FF (no surrogate, valid UTF)
			/*else */nr[NRENC_UTF16LE] = 2;
		}
	}

	//check for UTF-16BE:----------------------------------------------->UTF16-BE<-------------------------------------
	if (dwEnc & ENC_UTF16BE)
	{
		//FE FF == 0xFEFF (HI = FF (first); LO = FE (second))
		if (nMax < 4)
		{
			//should not be surogates:
			//makeword(low, high)
			//WORD val = MAKEWORD(*(wPos + 1), *wPos);
			//if (val > 0xD7FF)//surrogate!
			//	dwEnc &= ~ENC_UTF16BE;
			/*else */nr[NRENC_UTF16BE] = 2;
		}

		else //nMax >= 4
		{
			//WORD val = MAKEWORD(*(wPos + 1), *wPos);
			//if (val > 0xD7FF)//surrogate!
			//{
			//	//high surrogate must be in the range: U+D800 to U+DBFF
			//	if (val > 0xDBFF)//invalid for HIGH SUROGATE!
			//	{
			//		dwEnc &= ~ENC_UTF16BE;
			//	}
			//	else
			//	{
			//		WORD val2 = MAKEWORD(*(wPos + 3), *(wPos + 2));
			//		//low surrogate must be in the range: U+DC00 to U+DFFF
			//		if (!(val2 >= 0xDC00 && val2 <= 0xDFFF))
			//			dwEnc &= ~ENC_UTF16BE;
			//		else nr[NRENC_UTF16BE] = 4;
			//	}
			//}
			//else <= U+D7FF (no surrogate, valid UTF)
			/*else */nr[NRENC_UTF16BE] = 2;
		}
	}

	//check for ASCII:----------------------------------------------->ASCII<----------------------------------------
	if (*wPos > 0x7F)
	{
		dwEnc &= ~ENC_ASCII;
	}
	else nr[NRENC_ASCII] = 1;

	return dwEnc;
}

BOOL CSearchAppDlg::GetNextChar(const BYTE* wPos, DWORD& dwEnc, DWORD nMax, int nr[], BOOL& bNeedMore, const QWORD& nMaxFile)
{
	dwEnc = ENC_ALL;
	bNeedMore = FALSE;

	//check for UTF-8:----------------------------------------------->UTF8<----------------------------------------
	if (*wPos >= 0xF0)
	{
		//UTF-8 4 bytes:
		if (*wPos > 0xF4) dwEnc &= ~ENC_UTF8;
		else if (nMax < 4) {dwEnc &= ~ENC_UTF8; bNeedMore = TRUE; if (nMaxFile >= 4) return FALSE;}

		else //nMax >= 4, so we can check the other THREE bytes
		{
			if (*wPos == 0xF0)
			{
				//the other 3 bytes should be: 90->BF 80->BF 80->BF
				if (!(*(wPos + 1) >= 0x90 && *(wPos + 1) <= 0xBF &&
					*(wPos + 2) >= 0x80 && *(wPos + 2) <= 0xBF &&
					*(wPos + 3) >= 0x80 && *(wPos + 3) <= 0xBF)) dwEnc &= ~ENC_UTF8;
				else nr[NRENC_UTF8] = 4;
			}

			else if (*wPos >= 0xF1 && *wPos <= 0xF3)
			{
				//the other 3 bytes should be: 80->BF 80->BF 80->BF
				if (!(*(wPos + 1) >= 0x80 && *(wPos + 1) <= 0xBF &&
					*(wPos + 2) >= 0x80 && *(wPos + 2) <= 0xBF &&
					*(wPos + 3) >= 0x80 && *(wPos + 3) <= 0xBF)) dwEnc &= ~ENC_UTF8;
				else nr[NRENC_UTF8] = 4;
			}

			else if (*wPos == 0xF4)
			{
				//the other 3 bytes should be: 80->8F 80->BF 80->BF
				if (!(*(wPos + 1) >= 0x80 && *(wPos + 1) <= 0x8F &&
					*(wPos + 2) >= 0x80 && *(wPos + 2) <= 0xBF &&
					*(wPos + 3) >= 0x80 && *(wPos + 3) <= 0xBF)) dwEnc &= ~ENC_UTF8;
				else nr[NRENC_UTF8] = 4;
			}
		}
	}

	else if (*wPos >= 0xE0)
	{
		//UTF-8 3 bytes:
		if (*wPos > 0xEF) dwEnc &= ~ENC_UTF8;
		else if (nMax < 3) {dwEnc &= ~ENC_UTF8; bNeedMore = TRUE; if (nMaxFile >= 3) return FALSE;}

		else
		{
			//nMax >= 3, so we can check the other TWO bytes
			if (*wPos == 0xE0)
			{
				//the other 2 bytes should be: A0->BF 80->BF
				if (!(*(wPos + 1) >= 0xA0 && *(wPos + 1) <= 0xBF &&
					*(wPos + 2) >= 0x80 && *(wPos + 2) <= 0xBF)) dwEnc &= ~ENC_UTF8;
				else nr[NRENC_UTF8] = 3;
			}
			
			else if (*wPos >= 0xE1 && *wPos <= 0xEC)
			{
				//the other 2 bytes should be: 80->BF 80->BF
				if (!(*(wPos + 1) >= 0x80 && *(wPos + 1) <= 0xBF &&
					*(wPos + 2) >= 0x80 && *(wPos + 2) <= 0xBF)) dwEnc &= ~ENC_UTF8;
				else nr[NRENC_UTF8] = 3;
			}

			else if (*wPos == 0xED)
			{
				//the other 2 bytes should be: 80->9F 80->BF
				if (!(*(wPos + 1) >= 0x80 && *(wPos + 1) <= 0x9F &&
					*(wPos + 2) >= 0x80 && *(wPos + 2) <= 0xBF)) dwEnc &= ~ENC_UTF8;
				else nr[NRENC_UTF8] = 3;
			}

			else if (*wPos >= 0xEE && *wPos <= 0xEF)
			{
				//the other 2 bytes should be: 80->BF 80->BF
				if (!(*(wPos + 1) >= 0x80 && *(wPos + 1) <= 0xBF &&
					*(wPos + 2) >= 0x80 && *(wPos + 2) <= 0xBF)) dwEnc &= ~ENC_UTF8;
				else nr[NRENC_UTF8] = 3;
			}
		}
	}

	else if (*wPos >= 0xC2)
	{
		//UTF-8 2 bytes:
		if (*wPos > 0xDF) dwEnc &= ~ENC_UTF8;
		else if (nMax < 2) {dwEnc &= ~ENC_UTF8; bNeedMore = TRUE; if (nMaxFile >= 2) return FALSE;}

		else //nMax >= 2, so we can check the other ONE byte
		{
			//the other 1 byte should be: 80->BF
			if (!(*(wPos + 1) >= 0x80 && *(wPos + 1) <= 0xBF)) dwEnc &= ~ENC_UTF8;
			else nr[NRENC_UTF8] = 2;
		}
	}

	else if (*wPos > 0x7F)
	{
		//non-utf-8 (0x7F < *wPos < 0xC2)
		dwEnc &= ~ENC_UTF8;
	}
	else nr[NRENC_UTF8] = 1;

	//check for UTF-16LE:----------------------------------------------->UTF16-LE<-------------------------------------
	if (dwEnc & ENC_UTF16LE)
	{
		//FF FE == 0xFEFF (HI = FE (second); LO = FF (first))
		if (nMax < 4)
		{
			bNeedMore = TRUE;
			if (nMaxFile >= 4) return FALSE;
			//should not be surogates:
			//makeword(low, high)
			//WORD val = MAKEWORD(*wPos, *(wPos + 1));
			nr[NRENC_UTF16LE] = 2;
		}

		else //nMax >= 4
		{
			//WORD val = MAKEWORD(*wPos, *(wPos + 1));
			nr[NRENC_UTF16LE] = 2;
		}
	}

	//check for UTF-16BE:----------------------------------------------->UTF16-BE<-------------------------------------
	if (dwEnc & ENC_UTF16BE)
	{
		//FE FF == 0xFEFF (HI = FF (first); LO = FE (second))
		if (nMax < 4)
		{
			bNeedMore = TRUE;
			if (nMaxFile >= 4) return FALSE;
			//should not be surogates:
			//makeword(low, high)
			//WORD val = MAKEWORD(*(wPos + 1), *wPos);
			nr[NRENC_UTF16BE] = 2;
		}

		else //nMax >= 4
		{
			//WORD val = MAKEWORD(*(wPos + 1), *wPos);
			nr[NRENC_UTF16BE] = 2;
		}
	}

	//check for ASCII:----------------------------------------------->ASCII<----------------------------------------
	if (*wPos > 0x7F)
	{
		dwEnc &= ~ENC_ASCII;
	}
	else nr[NRENC_ASCII] = 1;

	return dwEnc;
}

BOOL CSearchAppDlg::CheckCharEncCS(const BYTE* wcContents, const BYTE* wcSearch, DWORD& dwEnc, int* nr, int nrMaxSearch)
{
	//we retrieve each char as its encodings and compare it to the current character in the search str.
	//the encodings that fail will have the "nr" set to 0.

	//x can only be 1!
	DWORD x = *(nr + NRENC_ASCII);
	if (x)
	{
		//the search string is little endian! (as far as I know)
		WORD wSearch = *((WCHAR*)wcSearch);

		if (*wcContents != wSearch)
			*(nr + NRENC_ASCII) = 0;
	}

	//nr can be 2 or 4!
	x = *(nr + NRENC_UTF16LE);
	if (x)
	{
		if (x == 2)
		{
			if (nrMaxSearch < 2)
				*(nr + NRENC_UTF16LE) = 0;

			else
			{
				//the search string is little endian! (as far as I know)
				WORD wSearch = *((WCHAR*)wcSearch);

				WORD wCont = MAKEWORD(*wcContents, *(wcContents + 1));
				if (wCont != wSearch)
					 *(nr + NRENC_UTF16LE) = 0;
			}
		}

		else //x == 4
		{
			if (nrMaxSearch < 4)
				*(nr + NRENC_UTF16LE) = 0;
			else
			{
				//the search string is little endian! (as far as I know)
				WORD wSearch1 = MAKEWORD(*wcSearch, *(wcSearch + 1));
				WORD wSearch2 = MAKEWORD(*(wcSearch + 2), *(wcSearch + 3));

				WORD wCont1 = MAKEWORD(*wcContents, *(wcContents + 1));
				WORD wCont2 = MAKEWORD(*(wcContents + 2), *(wcContents + 3));

				if (wCont1 != wSearch1 || wCont2 != wSearch2)
					 *(nr + NRENC_UTF16LE) = 0;
			}
		}
	}

	//nr can be 2 or 4!
	x = *(nr + NRENC_UTF16BE);
	if (x)
	{
		if (x == 2)
		{
			if (nrMaxSearch < 2)
				*(nr + NRENC_UTF16BE) = 0;
			else
			{
				//the search string is little endian! (as far as I know)
				WORD wSearch = *((WCHAR*)wcSearch);

				WORD wCont = MAKEWORD(*(wcContents + 1), *wcContents);
				if (wCont != wSearch)
					 *(nr + NRENC_UTF16BE) = 0;
			}
		}

		else //x == 4
		{
			if (nrMaxSearch < 4)
				*(nr + NRENC_UTF16BE) = 0;
			else
			{
				//the search string is little endian! (as far as I know)
				WORD wSearch1 = MAKEWORD(*(wcSearch + 1), *wcSearch);
				WORD wSearch2 = MAKEWORD(*(wcSearch + 3), *(wcSearch + 2));

				WORD wCont1 = MAKEWORD(*(wcContents + 1), *wcContents);
				WORD wCont2 = MAKEWORD(*(wcContents + 3), *(wcContents + 2));

				if (wCont1 != wSearch1 || wCont2 != wSearch2)
					 *(nr + NRENC_UTF16LE) = 0;
			}
		}
	}

	//nr can be 1 or 2 or 3 or 4
	x = *(nr + NRENC_UTF8);
	if (x)
	{		
		WCHAR wcChar[2];
		int nrUsed = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, (LPCSTR)wcContents, x, wcChar, 2);
		if (!nrUsed)
		{
			*(nr + NRENC_UTF8) = 0;
			/*DWORD dwError = GetLastError();
			if (dwError != ERROR_NO_UNICODE_TRANSLATION) DisplayError(dwError);*/
		}

		else if (nrUsed == 1)
		{
			if (nrMaxSearch < 2)
				*(nr + NRENC_UTF8) = 0;

			else if (*wcChar != *((WCHAR*)wcSearch))
				*(nr + NRENC_UTF8) = 0;
		}

		else if (nrUsed == 2)
		{
			if (nrMaxSearch < 4)
				*(nr + NRENC_UTF8) = 0;

			else
			{
				WORD wSearch1 = MAKEWORD(*wcSearch, *(wcSearch + 1));
				WORD wSearch2 = MAKEWORD(*(wcSearch + 2), *(wcSearch + 3));

				if (*wcChar != wSearch1 || *(wcChar + 1) != wSearch2)
					 *(nr + NRENC_UTF8) = 0;
			}
		}
	}

	int max = *(nr + NRENC_ASCII);
	if (max < *(nr + NRENC_UTF16LE)) max = *(nr + NRENC_UTF16LE);
	if (max < *(nr + NRENC_UTF16BE)) max = *(nr + NRENC_UTF16BE);
	if (max < *(nr + NRENC_UTF8)) max = *(nr + NRENC_UTF8);

	if (!nr[NRENC_ASCII]) dwEnc &= ~ENC_ASCII;
	if (!nr[NRENC_UTF16LE]) dwEnc &= ~ENC_UTF16LE;
	if (!nr[NRENC_UTF16BE]) dwEnc &= ~ENC_UTF16BE;
	if (!nr[NRENC_UTF8]) dwEnc &= ~ENC_UTF8;

	return max;
}

BOOL CSearchAppDlg::CheckCharEnc(const BYTE* wcContents, const BYTE* wcSearch, DWORD& dwEnc, int* nr, int nrMaxSearch)
{
	//we retrieve each char as its encodings and compare it to the current character in the search str.
	//the encodings that fail will have the "nr" set to 0.

	//x can only be 1!
	DWORD x = *(nr + NRENC_ASCII);
	if (x)
	{
		//the search string is little endian! (as far as I know)
		WORD wSearch = *((WCHAR*)wcSearch);

		if (tolower(*wcContents) != tolower(wSearch))
			*(nr + NRENC_ASCII) = 0;
	}

	//nr can be 2 or 4!
	x = *(nr + NRENC_UTF16LE);
	if (x)
	{
		if (x == 2)
		{
			if (nrMaxSearch < 2)
				*(nr + NRENC_UTF16LE) = 0;

			else
			{
				//the search string is little endian! (as far as I know)
				WORD wSearch = *((WCHAR*)wcSearch);

				WORD wCont = MAKEWORD(*wcContents, *(wcContents + 1));
				if (tolower(wCont) != tolower(wSearch))
					 *(nr + NRENC_UTF16LE) = 0;
			}
		}

		else //x == 4
		{
			if (nrMaxSearch < 4)
				*(nr + NRENC_UTF16LE) = 0;
			else
			{
				//the search string is little endian! (as far as I know)
				WORD wSearch1 = MAKEWORD(*wcSearch, *(wcSearch + 1));
				WORD wSearch2 = MAKEWORD(*(wcSearch + 2), *(wcSearch + 3));

				WORD wCont1 = MAKEWORD(*wcContents, *(wcContents + 1));
				WORD wCont2 = MAKEWORD(*(wcContents + 2), *(wcContents + 3));

				if (tolower(wCont1) != tolower(wSearch1) || tolower(wCont2) != tolower(wSearch2))
					 *(nr + NRENC_UTF16LE) = 0;
			}
		}
	}

	//nr can be 2 or 4!
	x = *(nr + NRENC_UTF16BE);
	if (x)
	{
		if (x == 2)
		{
			if (nrMaxSearch < 2)
				*(nr + NRENC_UTF16BE) = 0;
			else
			{
				//the search string is little endian! (as far as I know)
				WORD wSearch = *((WCHAR*)wcSearch);

				WORD wCont = MAKEWORD(*(wcContents + 1), *wcContents);
				if (tolower(wCont) != tolower(wSearch))
					 *(nr + NRENC_UTF16BE) = 0;
			}
		}

		else //x == 4
		{
			if (nrMaxSearch < 4)
				*(nr + NRENC_UTF16BE) = 0;
			else
			{
				//the search string is little endian! (as far as I know)
				WORD wSearch1 = MAKEWORD(*(wcSearch + 1), *wcSearch);
				WORD wSearch2 = MAKEWORD(*(wcSearch + 3), *(wcSearch + 2));

				WORD wCont1 = MAKEWORD(*(wcContents + 1), *wcContents);
				WORD wCont2 = MAKEWORD(*(wcContents + 3), *(wcContents + 2));

				if (tolower(wCont1) != tolower(wSearch1) || tolower(wCont2) != tolower(wSearch2))
					 *(nr + NRENC_UTF16LE) = 0;
			}
		}
	}

	//nr can be 1 or 2 or 3 or 4
	x = *(nr + NRENC_UTF8);
	if (x)
	{		
		WCHAR wcChar[2];
		int nrUsed = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, (LPCSTR)wcContents, x, wcChar, 2);
		if (!nrUsed)
		{
			*(nr + NRENC_UTF8) = 0;
			/*DWORD dwError = GetLastError();
			if (dwError != ERROR_NO_UNICODE_TRANSLATION) DisplayError(dwError);*/
		}

		else if (nrUsed == 1)
		{
			if (nrMaxSearch < 2)
				*(nr + NRENC_UTF8) = 0;

			else if (tolower(*wcChar) != tolower(*((WCHAR*)wcSearch)))
				*(nr + NRENC_UTF8) = 0;
		}

		else if (nrUsed == 2)
		{
			if (nrMaxSearch < 4)
				*(nr + NRENC_UTF8) = 0;

			else
			{
				WORD wSearch1 = MAKEWORD(*wcSearch, *(wcSearch + 1));
				WORD wSearch2 = MAKEWORD(*(wcSearch + 2), *(wcSearch + 3));

				if (tolower(*wcChar) != tolower(wSearch1) || tolower(*(wcChar + 1)) != tolower(wSearch2))
					 *(nr + NRENC_UTF8) = 0;
			}
		}
	}

	int max = *(nr + NRENC_ASCII);
	if (max < *(nr + NRENC_UTF16LE)) max = *(nr + NRENC_UTF16LE);
	if (max < *(nr + NRENC_UTF16BE)) max = *(nr + NRENC_UTF16BE);
	if (max < *(nr + NRENC_UTF8)) max = *(nr + NRENC_UTF8);

	if (!nr[NRENC_ASCII]) dwEnc &= ~ENC_ASCII;
	if (!nr[NRENC_UTF16LE]) dwEnc &= ~ENC_UTF16LE;
	if (!nr[NRENC_UTF16BE]) dwEnc &= ~ENC_UTF16BE;
	if (!nr[NRENC_UTF8]) dwEnc &= ~ENC_UTF8;

	return max;
}

void CSearchAppDlg::LoadFilters()
{
	m_FilterList.erase_all();

	//ok, get to HKLM\Software\Classes and go to the first '.'
	HKEY hKey;
	DWORD dwError = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Software", 0, KEY_READ, &hKey);
	if (dwError != 0)
	{
		return;
	}

	HKEY hAux = hKey;
	dwError = RegOpenKeyExW(hAux, L"Classes", 0, KEY_READ, &hKey);
	if (dwError != 0)
	{
		RegCloseKey(hAux);
		return;
	}
	RegCloseKey(hAux);

	//enumerate the subkeys:
	WCHAR wsKeyName[256];
	DWORD nKeyNameLen = 256;
	int nIndex = 0;
	WCHAR lastchar = 0;
	do
	{
		dwError = RegEnumKeyExW(hKey, nIndex, wsKeyName, &nKeyNameLen, NULL, NULL, NULL, NULL);
		if (dwError != 0)
		{
			RegCloseKey(hKey);
			return;
		}

		TryLoadFilter(hKey, wsKeyName, lastchar);

		nIndex++;
		nKeyNameLen = 256;
	}while (*wsKeyName <= '.');

	RegCloseKey(hKey);
}

void CSearchAppDlg::TryLoadFilter(const HKEY hParentKey, WCHAR* wsKeyName, WCHAR& lastchar)
{
	//check to see if it has a filter associated
	//first, open that key
	HKEY hFilter;
	DWORD dwError = RegOpenKeyExW(hParentKey, wsKeyName, 0, KEY_READ, &hFilter);
	if (dwError != 0)
	{
		return;
	}

	//retrieve the default value of PersistentHandler
	WCHAR wsValueName[100];
	DWORD nValueNameLen = 100;
	dwError = RegGetValueW(hFilter, L"PersistentHandler", 0, RRF_RT_REG_SZ, 0, (void*)wsValueName, &nValueNameLen);
	if (dwError != 0)
	{
		RegCloseKey(hFilter);

		//retrieve the handler from the CLSID of the filetype:
		//1. retrieve the filetype = default value of wsKeyName key.
		nValueNameLen = 100;
		dwError = RegGetValueW(hParentKey, wsKeyName, 0, RRF_RT_REG_SZ, 0, wsValueName, &nValueNameLen);
		if (dwError != 0)
		{
			return;
		}

		//2. get into HKLM\Software\Classes\<filetype>
		DWORD dwError = RegOpenKeyExW(hParentKey, wsValueName, 0, KEY_READ, &hFilter);
		if (dwError != 0)
		{
			return;
		}

		//3. get the default value of this key's CLSID. 
		nValueNameLen = 100;
		dwError = RegGetValueW(hFilter, L"CLSID", 0, RRF_RT_REG_SZ, 0, wsValueName, &nValueNameLen);
		if (dwError != 0)
		{
			RegCloseKey(hFilter);
			return;
		}
		RegCloseKey(hFilter);

		//4. go to HKLM\Software\Classes\CLSID
		dwError = RegOpenKeyExW(hParentKey, L"CLSID", 0, KEY_READ, &hFilter);
		if (dwError != 0)
		{
			return;
		}

		HKEY hAux = hFilter;

		//5. go to HKLM\Software\Classes\CLSID\<givenCLSID>
		dwError = RegOpenKeyExW(hAux, wsValueName, 0, KEY_READ, &hFilter);
		if (dwError != 0)
		{
			RegCloseKey(hAux);
			return;
		}
		RegCloseKey(hAux);

		//6. retrieve "PersistentHandler" from there
		nValueNameLen = 100;
		dwError = RegGetValueW(hFilter, L"PersistentHandler", 0, RRF_RT_REG_SZ, 0, wsValueName, &nValueNameLen);
		if (dwError != 0)
		{
			RegCloseKey(hFilter);
			return;
		}
	}
	RegCloseKey(hFilter);

	//we now must have the PersistentHandler. we now need the filter:
	//the following path will be: HKLM\Classes\CLSID
	dwError = RegOpenKeyExW(hParentKey, L"CLSID", 0, KEY_READ, &hFilter);
	if (dwError != 0)
	{
		return;
	}

	//the following path will be: HKLM\Classes\CLSID\<PersistentHandlerCLSID>
	HKEY hAux = hFilter;
	dwError = RegOpenKeyExW(hAux, wsValueName, 0, KEY_READ, &hFilter);
	if (dwError != 0)
	{
		RegCloseKey(hAux);
		return;
	}

	//the following path will be: HKLM\Classes\CLSID\<PersistentHandlerCLSID>\PersistentAddinsRegistered
	hAux = hFilter;
	dwError = RegOpenKeyExW(hAux, L"PersistentAddinsRegistered", 0, KEY_READ, &hFilter);
	if (dwError != 0)
	{
		RegCloseKey(hAux);
		return;
	}
	RegCloseKey(hAux);

	//we get the default value of the IID_IFilter CLSID key
	nValueNameLen = 100;
	dwError = RegGetValueW(hFilter, L"{89BCB740-6119-101A-BCB7-00DD010655AF}", 0, RRF_RT_REG_SZ, 0, wsValueName, &nValueNameLen);
	if (dwError != 0)
	{
		RegCloseKey(hFilter);
		return;
	}
	RegCloseKey(hFilter);

	//creating the IClassFactory
	CLSID cls;
	HRESULT hr = CLSIDFromString(wsValueName, &cls);

	IClassFactory* pClassFactory;
	hr = CoGetClassObject(cls, CLSCTX_ALL, NULL, IID_IClassFactory, (void**)&pClassFactory);
	if (FAILED(hr)) return;

	wsKeyName++;
	if (towlower(lastchar) == towlower(*wsKeyName))
	{
		CDoubleList<FILTERLIST>::Iterator I;

		for (I = m_FilterList.begin(); I != NULL; I = I->pNext)
		{
			if (I->m_Value.letter == lastchar)
			{
				FILTER filter;
				int len = wcslen(wsKeyName);
				len++;

				filter.wsExt = new WCHAR[len];
				StringCchCopyW(filter.wsExt, len, wsKeyName);
				filter.pClassFactory = pClassFactory;
				I->m_Value.pFilterList->push_back(filter);
			}
		}

	}
	else
	{
		//init filter:
		lastchar = *wsKeyName;

		FILTERLIST filterlist;
		filterlist.letter = towlower(lastchar);
		filterlist.pFilterList = new CDoubleList<FILTER>(OnDestroyFilter);

		FILTER filter;
		int len = wcslen(wsKeyName);
		len++;

		filter.wsExt = new WCHAR[len];
		StringCchCopyW(filter.wsExt, len, wsKeyName);
		filter.pClassFactory = pClassFactory;

		filterlist.pFilterList->push_back(filter);
		m_FilterList.push_back(filterlist);
	}
}

IClassFactory* CSearchAppDlg::FindFilter(const WCHAR* wsExtName)
{
	CDoubleList<FILTERLIST>::Iterator I;

	for (I = m_FilterList.begin(); I != NULL; I = I->pNext)
	{
		if (I->m_Value.letter == *wsExtName)
		{
			CDoubleList<FILTER>::Iterator J;
			for (J = I->m_Value.pFilterList->begin(); J != NULL; J = J->pNext)
			{
				if (wcscmp(wsExtName, J->m_Value.wsExt) == 0) return J->m_Value.pClassFactory;
			}

			return NULL;
		}
	}

	return NULL;
}

BOOL CSearchAppDlg::SearchUsingFilterCS(const WCHAR* wsFullName, const WCHAR* wsSearchContents, IClassFactory* pClassFactory, WIN32_FILE_ATTRIBUTE_DATA& data)
{
	IFilter* pFilter;
	HRESULT hr = pClassFactory->CreateInstance(NULL, IID_IFilter, (void**)&pFilter);
	if (FAILED(hr))
	{
		return FALSE;
	}

	IPersistFile* pPersistFile;
	hr = pFilter->QueryInterface(IID_IPersistFile, (void**)&pPersistFile);
	if (FAILED(hr)) {pFilter->Release(); return FALSE;}

	hr = pPersistFile->Load(wsFullName, STGM_READ);
	if (FAILED(hr)) {pFilter->Release(); pPersistFile->Release(); return FALSE;}

	ULONG finit = IFILTER_INIT_CANON_HYPHENS | IFILTER_INIT_CANON_PARAGRAPHS | IFILTER_INIT_CANON_SPACES |
						IFILTER_INIT_APPLY_INDEX_ATTRIBUTES | IFILTER_INIT_HARD_LINE_BREAKS | IFILTER_INIT_FILTER_OWNED_VALUE_OK;
	ULONG flags;
	hr = pFilter->Init(finit, 0, 0, &flags);
	if (FAILED(hr)) {pFilter->Release(); pPersistFile->Release(); return FALSE;}

	STAT_CHUNK chunk;
	chunk.flags = (CHUNKSTATE)0;

	LARGE_INTEGER liSize;
	liSize.HighPart = data.nFileSizeHigh;
	liSize.LowPart = data.nFileSizeLow;

	DWORD cbText, cbMaxText;

	if (liSize.QuadPart > 0xA00000)
		cbText = 0x500000;
	else cbText = liSize.LowPart;
	cbMaxText = cbText;

	WCHAR* wsText = new WCHAR[cbText];

	int pointer = 0;
	int contLen = wcslen(wsSearchContents);

	WCHAR* LastPart = NULL;
	DWORD nLastPartSize = 0;
	DWORD iFirstFound = (DWORD)-1;

	do
	{
		hr = pFilter->GetChunk(&chunk);
		
		if (SUCCEEDED(hr))
		{
			if (chunk.flags == CHUNK_TEXT)
			{
				while (SUCCEEDED(hr))
				{
					if (LastPart)
					{
						memcpy_s(wsText, cbMaxText * 2, LastPart, nLastPartSize * 2);
						delete[] LastPart;
						LastPart = NULL;
					}

					cbText = cbMaxText - nLastPartSize;

					hr = pFilter->GetText(&cbText, wsText + nLastPartSize);
					if (SUCCEEDED(hr))
					{
						for (DWORD i = 0; i < cbText; i++)
						{
							if (*(wsSearchContents + pointer) == *(wsText + i))
							{
								pointer++;

								if (iFirstFound == (DWORD)-1)
									iFirstFound = i;

								if (pointer == contLen)
								{
									delete[] wsText;
									pFilter->Release();
									pPersistFile->Release();
									return TRUE;
								}
							}
							else 
							{
								if (pointer)
								{
									if (iFirstFound == (DWORD)-1)
									{
										i = 0;
									}

									else
									{
										i = iFirstFound;
										iFirstFound = (DWORD)-1;
									}

									pointer = 0;
								}
							}
						}

						//ok, if we've searched through all the chunk, but the chunk ended before we reached the end.
						if (pointer)
						{
							if (iFirstFound != (DWORD)-1)
							{
								nLastPartSize = cbText - iFirstFound;

								if (LastPart) delete[] LastPart;
								LastPart = new WCHAR[nLastPartSize];
								memcpy_s(LastPart, nLastPartSize * 2, wsText + iFirstFound, nLastPartSize * 2);
								iFirstFound = 0;
							}

							else
							{
								nLastPartSize = 0;
							}

							pointer = 0;
						}
					}

					cbText = cbMaxText;
				}
			}
		}

		else break;

	} while (SUCCEEDED(hr));

	delete[] wsText;
	pPersistFile->Release();
	pFilter->Release();

	return FALSE;
}

BOOL CSearchAppDlg::SearchUsingFilterNCS(const WCHAR* wsFullName, const WCHAR* wsSearchContents, IClassFactory* pClassFactory, WIN32_FILE_ATTRIBUTE_DATA& data)
{
	IFilter* pFilter;
	HRESULT hr = pClassFactory->CreateInstance(NULL, IID_IFilter, (void**)&pFilter);
	if (FAILED(hr))
	{
		return FALSE;
	}

	IPersistFile* pPersistFile;
	hr = pFilter->QueryInterface(IID_IPersistFile, (void**)&pPersistFile);
	if (FAILED(hr)) {pFilter->Release(); return FALSE;}

	hr = pPersistFile->Load(wsFullName, STGM_READ);
	if (FAILED(hr)) {pFilter->Release(); pPersistFile->Release(); return FALSE;}

	ULONG finit = IFILTER_INIT_CANON_HYPHENS | IFILTER_INIT_CANON_PARAGRAPHS | IFILTER_INIT_CANON_SPACES |
						IFILTER_INIT_APPLY_INDEX_ATTRIBUTES | IFILTER_INIT_HARD_LINE_BREAKS | IFILTER_INIT_FILTER_OWNED_VALUE_OK;
	ULONG flags;
	hr = pFilter->Init(finit, 0, 0, &flags);
	if (FAILED(hr)) {pFilter->Release(); pPersistFile->Release(); return FALSE;}

	STAT_CHUNK chunk;
	chunk.flags = (CHUNKSTATE)0;

	LARGE_INTEGER liSize;
	liSize.HighPart = data.nFileSizeHigh;
	liSize.LowPart = data.nFileSizeLow;

	DWORD cbText, cbMaxText;

	if (liSize.QuadPart > 0xA00000)
		cbText = 0x500000;
	else cbText = liSize.LowPart;
	cbMaxText = cbText;

	WCHAR* wsText = new WCHAR[cbText];

	int pointer = 0;
	int contLen = wcslen(wsSearchContents);

	WCHAR* LastPart = NULL;
	DWORD nLastPartSize = 0;
	DWORD iFirstFound = (DWORD)-1;

	do
	{
		hr = pFilter->GetChunk(&chunk);
		
		if (SUCCEEDED(hr))
		{
			if (chunk.flags == CHUNK_TEXT)
			{
				while (SUCCEEDED(hr))
				{
					if (LastPart)
					{
						memcpy_s(wsText, cbMaxText * 2, LastPart, nLastPartSize * 2);
						delete[] LastPart;
						LastPart = NULL;
					}

					cbText = cbMaxText - nLastPartSize;

					hr = pFilter->GetText(&cbText, wsText + nLastPartSize);
					if (SUCCEEDED(hr))
					{
						for (DWORD i = 0; i < cbText; i++)
						{
							if (towlower(*(wsSearchContents + pointer)) == towlower(*(wsText + i)))
							{
								pointer++;

								if (iFirstFound == (DWORD)-1)
									iFirstFound = i;

								if (pointer == contLen)
								{
									delete[] wsText;
									pFilter->Release();
									pPersistFile->Release();
									return TRUE;
								}
							}
							else 
							{
								if (pointer)
								{
									if (iFirstFound == (DWORD)-1)
									{
										i = 0;
									}

									else
									{
										i = iFirstFound;
										iFirstFound = (DWORD)-1;
									}

									pointer = 0;
								}
							}
						}

						//ok, if we've searched through all the chunk, but the chunk ended before we reached the end.
						if (pointer)
						{
							if (iFirstFound != (DWORD)-1)
							{
								nLastPartSize = cbText - iFirstFound;

								if (LastPart) delete[] LastPart;
								LastPart = new WCHAR[nLastPartSize];
								memcpy_s(LastPart, nLastPartSize * 2, wsText + iFirstFound, nLastPartSize * 2);
								iFirstFound = 0;
							}

							else
							{
								nLastPartSize = 0;
							}

							pointer = 0;
						}
					}

					cbText = cbMaxText;
				}
				hr = S_OK;
			}
		}

		else break;

	} while (SUCCEEDED(hr));

	delete[] wsText;
	pPersistFile->Release();
	pFilter->Release();

	return FALSE;
}