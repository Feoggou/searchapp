#include "OptionsDlg.h"
#include "resource.h"

//#define TESTING

extern HINSTANCE hAppInstance;
WNDPROC COptionsDlg::OldEditTimeProc = NULL;

COptionsDlg::COptionsDlg(void)
{
	m_pCondition = NULL;
	m_hAccessedSince = m_hAccessedUntil = m_hCreatedSince = m_hCreatedUntil = m_hWrittenSince = m_hWrittenUntil = 0;
	m_hWnd = 0;
}

COptionsDlg::~COptionsDlg(void)
{
}

LRESULT CALLBACK COptionsDlg::NewEditTimeProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_KILLFOCUS:
		{
			int len = GetWindowTextLengthW(hWnd);
			if (len == 0)
				SetWindowTextW(hWnd, L"Anytime");
			else
			{
				len++;
				WCHAR* wsText = new WCHAR[len];
				GetWindowTextW(hWnd, wsText, len);
				if (wcscmp(wsText, L"0") == 0) 
				{
					SetWindowTextW(hWnd, L"Anytime");
					delete[] wsText;
					break;
				}
				
				delete[] wsText;
			}
		}
		break;
	}

	return CallWindowProcW(OldEditTimeProc, hWnd, uMsg, wParam, lParam);
}

INT_PTR CALLBACK COptionsDlg::DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static COptionsDlg* pThis = NULL;
	
	if (uMsg == WM_INITDIALOG)
	{
		pThis = (COptionsDlg*)lParam;
		SetWindowLongW(hDlg, GWL_USERDATA, lParam);
		pThis->m_hWnd = hDlg;
		return pThis->OnInitDialog();
	}

	if (pThis)
	{
		switch (uMsg)
		{
		case WM_CLOSE:
			EndDialog(hDlg, IDCANCEL);
			break;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
			case IDOK: 
				{
					if (pThis->OnOK())
						EndDialog(hDlg, IDOK);
				}

				break;

			case IDCANCEL:
				EndDialog(hDlg, IDCANCEL);
				break;
			}

		case WM_CTLCOLOREDIT:
			{
				HWND hEdit = (HWND)lParam;
				
				if (hEdit == pThis->m_hAccessedSince || hEdit == pThis->m_hAccessedUntil ||
					hEdit == pThis->m_hCreatedSince || hEdit == pThis->m_hCreatedUntil ||
					hEdit == pThis->m_hWrittenSince || hEdit == pThis->m_hWrittenUntil)
				{
					HDC hDC = (HDC)wParam;
					SetTextColor(hDC, RGB(0, 0, 255));
					SetBkColor(hDC, RGB(255, 255, 255));
					HBRUSH hBrush = (HBRUSH)GetStockObject(WHITE_BRUSH);
					return (INT_PTR)hBrush;
				}
			}
			break;
		}
	}

	return 0;
}

INT_PTR COptionsDlg::DoModal(HWND hParent, CONDITION* pCondition)
{
	m_pCondition = pCondition;
	return DialogBoxParamW(hAppInstance, MAKEINTRESOURCE(IDD_SEARCH_OPTIONS), hParent, DialogProc, (LPARAM)this);
}

INT_PTR COptionsDlg::OnInitDialog()
{
	//time:
	m_hCreatedSince = GetDlgItem(m_hWnd, IDC_CREATED_SINCE);
	m_hCreatedUntil = GetDlgItem(m_hWnd, IDC_CREATED_UNTIL);
	m_hWrittenSince = GetDlgItem(m_hWnd, IDC_WRITTEN_SINCE);
	m_hWrittenUntil = GetDlgItem(m_hWnd, IDC_WRITTEN_UNTIL);
	m_hAccessedSince = GetDlgItem(m_hWnd, IDC_ACCESSED_SINCE);
	m_hAccessedUntil = GetDlgItem(m_hWnd, IDC_ACCESSED_UNTIL);

	InitializeTimeBoxes();

	//window long:
	OldEditTimeProc = (WNDPROC)SetWindowLongW(m_hCreatedSince, GWL_WNDPROC, (LONG)NewEditTimeProc);
	SetWindowLongW(m_hCreatedUntil, GWL_WNDPROC, (LONG)NewEditTimeProc);
	SetWindowLongW(m_hWrittenSince, GWL_WNDPROC, (LONG)NewEditTimeProc);
	SetWindowLongW(m_hWrittenUntil, GWL_WNDPROC, (LONG)NewEditTimeProc);
	SetWindowLongW(m_hAccessedSince, GWL_WNDPROC, (LONG)NewEditTimeProc);
	SetWindowLongW(m_hAccessedUntil, GWL_WNDPROC, (LONG)NewEditTimeProc);

	return TRUE;
}

BOOL COptionsDlg::OnOK()
{
	if (false == CheckTimes()) return FALSE;

	//writing into m_Condition
	//ACCESS TIME
	ULARGE_INTEGER uli1, uli2;

	uli1.HighPart = m_ftAccessedSince.dwHighDateTime;
	uli1.LowPart = m_ftAccessedSince.dwLowDateTime;
	uli2.HighPart = m_ftAccessedUntil.dwHighDateTime;
	uli2.LowPart = m_ftAccessedUntil.dwLowDateTime;

	if (uli1.QuadPart == 0 && uli2.QuadPart == 0)
	{
		if (m_pCondition->ftAccessTime)
		{
			delete[] m_pCondition->ftAccessTime;
			m_pCondition->ftAccessTime = NULL;
		}
	}
	else
	{
		if (!m_pCondition->ftAccessTime)
		{
			m_pCondition->ftAccessTime = new QWORD[2];
		}

		m_pCondition->ftAccessTime[0] = uli1.QuadPart;
		m_pCondition->ftAccessTime[1] = uli2.QuadPart;
	}

	//CREATEDSINCE
	uli1.HighPart = m_ftCreatedSince.dwHighDateTime;
	uli1.LowPart = m_ftCreatedSince.dwLowDateTime;
	uli2.HighPart = m_ftCreatedUntil.dwHighDateTime;
	uli2.LowPart = m_ftCreatedUntil.dwLowDateTime;

	if (uli1.QuadPart == 0 && uli2.QuadPart == 0)
	{
		if (m_pCondition->ftCreationTime)
		{
			delete[] m_pCondition->ftCreationTime;
			m_pCondition->ftCreationTime = NULL;
		}
	}
	else
	{
		if (!m_pCondition->ftCreationTime)
		{
			m_pCondition->ftCreationTime = new QWORD[2];
		}

		m_pCondition->ftCreationTime[0] = uli1.QuadPart;
		m_pCondition->ftCreationTime[1] = uli2.QuadPart;
	}

	//MODIFIDEDSINCE
	uli1.HighPart = m_ftWrittenSince.dwHighDateTime;
	uli1.LowPart = m_ftWrittenSince.dwLowDateTime;
	uli2.HighPart = m_ftWrittenUntil.dwHighDateTime;
	uli2.LowPart = m_ftWrittenUntil.dwLowDateTime;

	if (uli1.QuadPart == 0 && uli2.QuadPart == 0)
	{
		if (m_pCondition->ftModifyTime)
		{
			delete[] m_pCondition->ftModifyTime;
			m_pCondition->ftModifyTime = NULL;
		}
	}
	else
	{
		if (!m_pCondition->ftModifyTime)
		{
			m_pCondition->ftModifyTime = new QWORD[2];
		}

		m_pCondition->ftModifyTime[0] = uli1.QuadPart;
		m_pCondition->ftModifyTime[1] = uli2.QuadPart;
	}

	return TRUE;
}

BOOL COptionsDlg::CheckTimes(void)
{
	//CREATED
	//created since
	int len = GetWindowTextLengthW(m_hCreatedSince);
	len++;
	WCHAR* wsText = new WCHAR[len];
	GetWindowTextW(m_hCreatedSince, wsText, len);
	if (false == CheckTimeText(wsText, len, m_ftCreatedSince, TRUE))
	{
		delete[] wsText;
		return false;
	}
	delete[] wsText;

	//created until
	len = GetWindowTextLengthW(m_hCreatedUntil);
	len++;
	wsText = new WCHAR[len];
	GetWindowTextW(m_hCreatedUntil, wsText, len);
	if (false == CheckTimeText(wsText, len, m_ftCreatedUntil, FALSE))
	{
		delete[] wsText;
		return false;
	}
	delete[] wsText;

	ULARGE_INTEGER uli_min, uli_max;
	uli_min.HighPart = m_ftCreatedSince.dwHighDateTime;
	uli_min.LowPart = m_ftCreatedSince.dwLowDateTime;

	uli_max.HighPart = m_ftCreatedUntil.dwHighDateTime;
	uli_max.LowPart = m_ftCreatedUntil.dwLowDateTime;

	if (uli_min.QuadPart > uli_max.QuadPart)
	{
		MessageBox(m_hWnd, L"The creation time of \"since\" must be lower or equal to the creation time of \"until\".", L"Error!", 0);
		return false;
	}

	//WRITTEN
	//written since
	len = GetWindowTextLengthW(m_hWrittenSince);
	len++;
	wsText = new WCHAR[len];
	GetWindowTextW(m_hWrittenSince, wsText, len);
	if (false == CheckTimeText(wsText, len, m_ftWrittenSince, TRUE))
	{
		delete[] wsText;
		return false;
	}
	delete[] wsText;

	//written until
	len = GetWindowTextLengthW(m_hWrittenUntil);
	len++;
	wsText = new WCHAR[len];
	GetWindowTextW(m_hWrittenUntil, wsText, len);
	if (false == CheckTimeText(wsText, len, m_ftWrittenUntil, FALSE))
	{
		delete[] wsText;
		return false;
	}
	delete[] wsText;

	uli_min, uli_max;
	uli_min.HighPart = m_ftWrittenSince.dwHighDateTime;
	uli_min.LowPart = m_ftWrittenSince.dwLowDateTime;

	uli_max.HighPart = m_ftWrittenUntil.dwHighDateTime;
	uli_max.LowPart = m_ftWrittenUntil.dwLowDateTime;

	if (uli_min.QuadPart > uli_max.QuadPart)
	{
		MessageBox(m_hWnd, L"The written time of \"since\" must be lower or equal to the creation time of \"until\".", L"Error!", 0);
		return false;
	}

	//ACCESSED
	//accessed since
	len = GetWindowTextLengthW(m_hAccessedSince);
	len++;
	wsText = new WCHAR[len];
	GetWindowTextW(m_hAccessedSince, wsText, len);
	if (false == CheckTimeText(wsText, len, m_ftAccessedSince, TRUE))
	{
		delete[] wsText;
		return false;
	}
	delete[] wsText;

	//accessed until
	len = GetWindowTextLengthW(m_hAccessedUntil);
	len++;
	wsText = new WCHAR[len];
	GetWindowTextW(m_hAccessedUntil, wsText, len);
	if (false == CheckTimeText(wsText, len, m_ftAccessedUntil, FALSE))
	{
		delete[] wsText;
		return false;
	}
	delete[] wsText;

	uli_min, uli_max;
	uli_min.HighPart = m_ftAccessedSince.dwHighDateTime;
	uli_min.LowPart = m_ftAccessedSince.dwLowDateTime;

	uli_max.HighPart = m_ftAccessedUntil.dwHighDateTime;
	uli_max.LowPart = m_ftAccessedUntil.dwLowDateTime;

	if (uli_min.QuadPart > uli_max.QuadPart)
	{
		MessageBox(m_hWnd, L"The access time of \"since\" must be lower or equal to the creation time of \"until\".", L"Error!", 0);
		return false;
	}

	return true;
}



BOOL COptionsDlg::CheckTimeText(WCHAR* wsText, int len, FILETIME& fileTime, BOOL bIsSince)
{
	SYSTEMTIME localTime;
	GetLocalTime(&localTime);
	localTime.wMilliseconds = 0;
	localTime.wSecond = 0;
	localTime.wMinute = 0;
	localTime.wHour = 0;

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
	//january; february; march; april; may; june; july; august; september; october; november; december;
	//monday; tuesday; wednesday; thursday; friday; saturday; sunday
	//today; yesterday;
	//last:
	//day; week; month; year;
	//anytime
	if (iswalpha(*wsResult))
	{
		//MONTHS
		wchRes = wsResult;
		KEYWORD key;

		if (false == GetNextString(wchRes, key))
		{
			MessageBox(m_hWnd, L"Invalid time specified!", 0, MB_ICONERROR);
			delete[] wsResult;
			return false;
		}
		else
		{
			if (key >= COptionsDlg::January && key <= COptionsDlg::December)
			{
				if (localTime.wMonth < key && !*wchRes)
					localTime.wYear--;
				
				localTime.wMonth = (WORD)key;
				localTime.wDay = 1;

				//is there something after the month? (year)
				if (*wchRes)
				{
					if (*wchRes >= '0' && *wchRes <= '9')
					{
						int year;
						if (false == GetNextInt(year, &wchRes))
						{
							MessageBox(m_hWnd, L"Invalid time specified!", 0, MB_ICONERROR);
							delete[] wsResult;
							return false;
						}

						if (year < 1601 || year > localTime.wYear)
						{
							MessageBox(m_hWnd, L"Invalid time specified!", 0, MB_ICONERROR);
							delete[] wsResult;
							return false;
						}

						else localTime.wYear = (WORD)year;
					}
					else
					{
						MessageBox(m_hWnd, L"Invalid time specified!", 0, MB_ICONERROR);
						delete[] wsResult;
						return false;
					}
				}
			}

			else if (key >= COptionsDlg::Monday && key <= COptionsDlg::Sunday)
			{
				int iDayOfWeek;
				if (key == COptionsDlg::Sunday)
					iDayOfWeek = 0;
				else iDayOfWeek = key - COptionsDlg::Monday + 1;

				BOOL bResult = CalcDay(iDayOfWeek, localTime, fileTime);
				delete[] wsResult;
				return bResult;
			}

			else if (key == COptionsDlg::Today)
			{
			}

			else if (key == COptionsDlg::Yesterday)
			{
				BOOL bResult = DecDays(1, localTime, fileTime);
				delete[] wsResult;
				return bResult;
			}

			else if (key == COptionsDlg::Anytime)
			{
				if (bIsSince)
					memset(&fileTime, 0, sizeof(fileTime));
				else memset(&fileTime, 0xFF, sizeof(fileTime));

				delete[] wsResult;
				return TRUE;
			}

			else if (key == COptionsDlg::LastYear)
			{
				localTime.wYear--;
				localTime.wMonth = 1;
				localTime.wDay = 1;
			}

			else if (key == COptionsDlg::LastMonth)
			{
				DecMonth(1, localTime);
			}

			else if (key == COptionsDlg::LastWeek)
			{
				BOOL bResult = DecWeeks(1, localTime, fileTime);
				delete[] wsResult;
				return bResult;
			}

			else if (key == COptionsDlg::LastDay)
			{
				BOOL bResult = DecDays(1, localTime, fileTime);
				delete[] wsResult;
				return bResult;
			}

			else
			{
				//invalid keyword!
				MessageBox(m_hWnd, L"Invalid time specified!", 0, MB_ICONERROR);
				delete[] wsResult;
				return false;
			}
		}
	}
	
	else if (*wsResult >= '0' && *wsResult <= '9')
	{
		//with numbers:
		//[X] (e.g. 5):
		//days; weeks; months; years; + ago

		//[X]January; March; ...; 
		//[X]January; March; ... [YYYY] (year)

		//[DD].[MM].[YYYY]; . = '.'; '-'; '/';
		//[D].[M].[YYYY]; [DD].[M].[YYYY]; [D].[MM].[YYYY]
		//[YYYY] (that is, the year).

		//retrieving the number (integer)
		WCHAR* wPos = wsResult;
		int nr;
		if (false == GetNextInt(nr, &wPos))
		{
			MessageBox(m_hWnd, L"Invalid time specified!", 0, MB_ICONERROR);
			delete[] wsResult;
			return false;
		}

		KEYWORD Key;

		//check date here:
		if (*wPos == 0)//end of string
		{
			//[YYYY]
			localTime.wDay = 1;
			localTime.wMonth = 1;

			if (nr < 1601 || nr > localTime.wYear)
			{
				MessageBox(m_hWnd, L"Invalid time specified!", 0, MB_ICONERROR);
				delete[] wsResult;
				return false;
			}

			else localTime.wYear = (WORD)nr;
		}
		else if (*wPos == '/' || *wPos == '.' || *wPos == '-')
		{
			//[DD].[MM].[YYYY]; . = '.'; '-'; '/';
			//[D].[M].[YYYY]; [DD].[M].[YYYY]; [D].[MM].[YYYY]
			int day, month, year;
			day = nr;

			//month
			wPos++;
			if (*wPos >= '0' && *wPos <= '9')
			{
				if (false == GetNextInt(nr, &wPos))
				{
					MessageBox(m_hWnd, L"Invalid time specified!", 0, MB_ICONERROR);
					delete[] wsResult;
					return false;
				}

				month = nr;

				if (*wPos)
				{
					if (!(*wPos == '/' || *wPos == '.' || *wPos == '-'))
					{
						MessageBox(m_hWnd, L"Invalid time specified!", 0, MB_ICONERROR);
						delete[] wsResult;
						return false;
					}

					//year
					wPos++;
					if (*wPos >= '0' && *wPos <= '9')
					{
						if (false == GetNextInt(nr, &wPos))
						{
							MessageBox(m_hWnd, L"Invalid time specified!", 0, MB_ICONERROR);
							delete[] wsResult;
							return false;
						}

						year = nr;
					}
					else
					{
						MessageBox(m_hWnd, L"Invalid time specified!", 0, MB_ICONERROR);
						delete[] wsResult;
						return false;
					}

					if (*wPos)
					{
						MessageBox(m_hWnd, L"Invalid time specified!", 0, MB_ICONERROR);
						delete[] wsResult;
						return false;
					}
				}

				else
				{
					year = localTime.wYear;
					if (localTime.wMonth < month)
						year--;
				}
			}
			else
			{
				MessageBox(m_hWnd, L"Invalid time specified!", 0, MB_ICONERROR);
				delete[] wsResult;
				return false;
			}

			localTime.wDay = (WORD)day;
			localTime.wMonth = (WORD)month;
			localTime.wYear = (WORD)year;
		}

		//with numbers:
		//[X] (e.g. 5):
		//days; weeks; months; years; + ago

		//[X]January; March; ...; 
		//[X]January; March; ... [YYYY] (year)
		else if (false == GetNextString(wPos, Key))
		{
			delete[] wsResult;
			return false;
		}
		
		else if (Key >= COptionsDlg::Days && Key <= COptionsDlg::Years)
		{
			//[X] (e.g. 5):
			//days, weeks; months; years; + ago
			if (nr > 50)
			{
				MessageBox(m_hWnd, L"Invalid time specified: the number is too large", 0, MB_ICONERROR);
				delete[] wsResult;
				return false;
			}

			if (*wPos)
			{
				MessageBox(m_hWnd, L"Invalid time specified", 0, MB_ICONERROR);
				delete[] wsResult;
				return false;
			}

			switch (Key)
			{
			case COptionsDlg::Days:
				{
					BOOL bResult = DecDays((WORD)nr, localTime, fileTime);
					delete[] wsResult;
					return bResult;
				}

			case COptionsDlg::Weeks:
				{
					BOOL bResult = DecWeeks((WORD)nr, localTime, fileTime);
					delete[] wsResult;
					return bResult;
				}
				break;

			case COptionsDlg::Months:
				{
					DecMonth(1, localTime);
				}
				break;

			case COptionsDlg::Years:
				{
					localTime.wDay = 1;
					localTime.wMonth = 1;
					localTime.wYear -= nr;
				}
				break;
			}
		}

		else if (Key >= COptionsDlg::January && Key <= COptionsDlg::December)
		{
			//[X]January; March; ...; 
			//[X]January; March; ... [YYYY] (year)

			//is X a valid number for this month?

			if (localTime.wMonth < Key && !*wPos)
					localTime.wYear--;

			localTime.wDay = (WORD)nr;
			localTime.wMonth = (WORD)Key;

			if (false == SystemTimeToFileTime(&localTime, &fileTime))
			{
				DWORD dwError = GetLastError();
				if (dwError == 0x57)
				{
					MessageBox(m_hWnd, L"Invalid time specified!", 0, MB_ICONERROR);
				}
				
				else
				{
					DisplayError();
				}

				delete[] wsResult;
				return false;
			}

			//is there something after the month? (year)
			if (*wPos)
			{
				if (*wPos >= '0' && *wPos <= '9')
				{
					int year;
					if (false == GetNextInt(year, &wPos))
					{
						MessageBox(m_hWnd, L"Invalid time specified!", 0, MB_ICONERROR);
						delete[] wsResult;
						return false;
					}

					if (year < 1601 || year > localTime.wYear)
					{
						MessageBox(m_hWnd, L"Invalid time specified!", 0, MB_ICONERROR);
						delete[] wsResult;
						return false;
					}

					else localTime.wYear = (WORD)year;
				}
				else
				{
					MessageBox(m_hWnd, L"Invalid time specified!", 0, MB_ICONERROR);
					delete[] wsResult;
					return false;
				}
			}
		}

		else
		{
			MessageBox(m_hWnd, L"Invalid time specified!", 0, MB_ICONERROR);
			delete[] wsResult;
			return false;
		}
	}
	else
	{
		MessageBox(m_hWnd, L"Invalid time specified!", 0, MB_ICONERROR);
		delete[] wsResult;
		return false;
	}

	if (false == SystemTimeToFileTime(&localTime, &fileTime))
	{
		DWORD dwError = GetLastError();
		if (dwError == 0x57)
		{
			MessageBox(m_hWnd, L"Invalid time specified!", 0, MB_ICONERROR);
		}
		else
		{
			DisplayError(dwError);
		}

		delete[] wsResult;
		return false;
	}

#ifdef TESTING
	FileTimeToSystemTime(&fileTime, &localTime);
#endif

	delete[] wsResult;

	return true;
}

BOOL COptionsDlg::CalcDay(int iChosenDay, SYSTEMTIME& localTime, FILETIME& fileTime)
{
	int substract = 0;
	if (localTime.wDayOfWeek == iChosenDay)
	{
		localTime.wHour = 0;
	}
	//current day > chosen day
	else if (localTime.wDayOfWeek > iChosenDay) 
	{
		substract = localTime.wDayOfWeek - iChosenDay;
	}
	//current day < chosen day
	else
	{
		substract = localTime.wDayOfWeek + 7 - iChosenDay;
	}

	if (false == SystemTimeToFileTime(&localTime, &fileTime))
	{
		DisplayError();
		return false;
	}

	ULARGE_INTEGER uli;
	uli.HighPart = fileTime.dwHighDateTime;
	uli.LowPart = fileTime.dwLowDateTime;
	//1 day = 864000000000
	//substract = nr days
	uli.QuadPart -= substract * 864000000000;
	fileTime.dwHighDateTime = uli.HighPart;
	fileTime.dwLowDateTime = uli.LowPart;

#ifdef TESTING
	FileTimeToSystemTime(&fileTime, &localTime);
#endif

	return true;
}

void COptionsDlg::DecMonth(WORD nCountDec, SYSTEMTIME& localTime)
{
	localTime.wDay = 1;

	if (localTime.wMonth > nCountDec)
	{
		localTime.wMonth -= nCountDec;
	}
	else
	{
		short nr = localTime.wMonth - nCountDec;
		while (nr <= 0)
		{
			localTime.wYear--;
			nr += 12;
		}

		localTime.wMonth = (WORD)nr;
	}
}

BOOL COptionsDlg::DecDays(WORD nCountDec, SYSTEMTIME& localTime, FILETIME& fileTime)
{
	if (false == SystemTimeToFileTime(&localTime, &fileTime))
	{
		DisplayError();
		return false;
	}

	ULARGE_INTEGER uli;
	uli.HighPart = fileTime.dwHighDateTime;
	uli.LowPart = fileTime.dwLowDateTime;
	//864000000000 = 1 day
	uli.QuadPart -= nCountDec * 864000000000;
	fileTime.dwHighDateTime = uli.HighPart;
	fileTime.dwLowDateTime = uli.LowPart;

#ifdef TESTING
	FileTimeToSystemTime(&fileTime, &localTime);
#endif

	return true;
}

BOOL COptionsDlg::DecWeeks(WORD nCountDec, SYSTEMTIME& localTime, FILETIME& fileTime)
{
	WORD nCount = localTime.wDayOfWeek + 7 * nCountDec - 1;
	if  (localTime.wDayOfWeek == 0) nCount += 7;

	return DecDays(nCount, localTime, fileTime);
}

BOOL COptionsDlg::GetNextInt(int& nr, WCHAR** wPos)
{
	nr = 0;
	while (**wPos && **wPos >= '0' && **wPos <= '9')
	{
		nr *= 10;
		nr += **wPos - '0';
		(*wPos)++;

		if (nr >9999)
		{
			MessageBox(m_hWnd, L"Invalid Time: the number is too large", 0, 0);
			return FALSE;
		}
	}

	return true;
}


BOOL COptionsDlg::GetNextString(WCHAR*& wPos, KEYWORD& key)
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

	WCHAR wsKeys[][10] =
//0 --> 11
{L"january", L"february", L"march", L"april", L"may", L"june", L"july", L"august", L"september", L"october", L"november", L"december",
//12 --> 23
L"jan", L"feb", L"mar", L"apr", L"may", L"jun", L"jul", L"aug", L"sep", L"oct", L"nov", L"dec",

//24 --> 30
L"monday", L"tuesday", L"wednesday", L"thursday", L"friday", L"saturday", L"sunday",
//31 --> 37
L"mon", L"tue", L"wed", L"thu", L"fri", L"sat", L"sun",

//38 --> 43
L"today", L"yesterday", L"lastday", L"lastweek", L"lastmonth", L"lastyear",
//44 --> 45
L"anytime", L"any",

//46 --> 49
L"days", L"weeks", L"months", L"years",

//50 --> 53
L"daysago", L"weeksago", L"monthsago", L"yearsago"};

	//54
	//-1 = invalid (do not check it anymore)
	//otherwise, the value is position.
	int checked[] =
{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0,

0, 0, 0, 0, 0, 0,
0, 0,

0, 0, 0, 0,
0, 0, 0, 0};

	//compare the string from wsStart until wPos, to see if it is one of the keywords:
	//i = position in the string
	int i = 0, lastvalid = -1;
	for (WCHAR* wch = wsStart; wch != wPos; wch++, i++)
	{
		//j = the ordinal (ID) of the string
		for (int j = 0; j < 54; j++)
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
		if (i < wcslen(*(wsKeys + lastvalid))) *(checked + lastvalid) = - 1;
		if (*(checked + lastvalid) == - 1) lastvalid = 500;
	}

	//only one must be valid now: lastvalid
	switch (lastvalid)
	{
		//months
	case 0:
	case 12:
		key = COptionsDlg::January;
		break;

	case 1:
	case 13:
		key = COptionsDlg::February;
		break;

	case 2:
	case 14:
		key = COptionsDlg::March;
		break;

	case 3:
	case 15:
		key = COptionsDlg::April;
		break;

	case 4:
	case 16:
		key = COptionsDlg::May;
		break;

	case 5:
	case 17:
		key = COptionsDlg::June;
		break;

	case 6:
	case 18:
		key = COptionsDlg::July;
		break;

	case 7:
	case 19:
		key = COptionsDlg::August;
		break;

	case 8:
	case 20:
		key = COptionsDlg::September;
		break;

	case 9:
	case 21:
		key = COptionsDlg::October;
		break;

	case 10:
	case 22:
		key = COptionsDlg::November;
		break;
		
	case 11:
	case 23:
		key = COptionsDlg::December;
		break;

		//days of the week:
	case 24:
	case 31:
		key = COptionsDlg::Monday;
		break;

	case 25:
	case 32:
		key = COptionsDlg::Tuesday;
		break;

	case 26:
	case 33:
		key = COptionsDlg::Wednesday;
		break;

	case 27:
	case 34:
		key = COptionsDlg::Thursday;
		break;

	case 28:
	case 35:
		key = COptionsDlg::Friday;
		break;

	case 29:
	case 36:
		key = COptionsDlg::Saturday;
		break;

	case 30:
	case 37:
		key = COptionsDlg::Sunday;
		break;

		//relative
	case 38:
		key = COptionsDlg::Today;
		break;

	case 39:
		key = COptionsDlg::Yesterday;
		break;

	case 40:
		key = COptionsDlg::LastDay;
		break;

	case 41:
		key = COptionsDlg::LastWeek;
		break;

	case 42:
		key = COptionsDlg::LastMonth;
		break;

	case 43:
		key = COptionsDlg::LastYear;
		break;

	case 44:
	case 45:
		key = COptionsDlg::Anytime;
		break;

	case 46:
	case 50:
		key = COptionsDlg::Days;
		break;

	case 47:
	case 51:
		key = COptionsDlg::Weeks;
		break;

	case 48:
	case 52:
		key = COptionsDlg::Months;
		break;

	case 49:
	case 53:
		key = COptionsDlg::Years;
		break;

	default:
		return false;
	}

	return true;
}

void COptionsDlg::InitializeTimeBoxes(void)
{
	//ACCESS TIME
	if (m_pCondition->ftAccessTime)
	{
		//since
		if (*(m_pCondition->ftAccessTime) == 0)
			SetWindowTextW(m_hAccessedSince, L"Anytime");
		else
		{
			FILETIME* pFT = (FILETIME*)m_pCondition->ftAccessTime;
			SYSTEMTIME sysTime;
			FileTimeToSystemTime(pFT, &sysTime);

			WCHAR wstr[25];
			StringCchPrintf(wstr, 25, L"%02d.%02d.%04d", sysTime.wDay, sysTime.wMonth, sysTime.wYear);
			SetWindowTextW(m_hAccessedSince, wstr);
		}

		//until
		if (*(m_pCondition->ftAccessTime + 1) == 0)
		{
			SetWindowTextW(m_hAccessedUntil, L"Anytime");
		}
		else
		{
			FILETIME* pFT = (FILETIME*)(m_pCondition->ftAccessTime + 1);
			if (pFT->dwHighDateTime == (DWORD)-1 && pFT->dwLowDateTime == (DWORD)-1)
			{
				SetWindowTextW(m_hAccessedUntil, L"Anytime");
			}

			else
			{
				SYSTEMTIME sysTime;
				FileTimeToSystemTime(pFT, &sysTime);

				WCHAR wstr[25];
				StringCchPrintf(wstr, 25, L"%02d.%02d.%04d", sysTime.wDay, sysTime.wMonth, sysTime.wYear);
				SetWindowTextW(m_hAccessedUntil, wstr);
			}
		}
	}
	else
	{
		SetWindowTextW(m_hAccessedSince, L"Anytime");
		SetWindowTextW(m_hAccessedUntil, L"Anytime");
	}

	//CREATION TIME
	if (m_pCondition->ftCreationTime)
	{
		//since
		if (*(m_pCondition->ftCreationTime) == 0)
			SetWindowTextW(m_hCreatedSince, L"Anytime");
		else
		{
			FILETIME* pFT = (FILETIME*)m_pCondition->ftCreationTime;
			SYSTEMTIME sysTime;
			FileTimeToSystemTime(pFT, &sysTime);

			WCHAR wstr[25];
			StringCchPrintf(wstr, 25, L"%02d.%02d.%04d", sysTime.wDay, sysTime.wMonth, sysTime.wYear);
			SetWindowTextW(m_hCreatedSince, wstr);
		}

		//until
		if (*(m_pCondition->ftCreationTime + 1) == 0)
		{
			SetWindowTextW(m_hCreatedUntil, L"Anytime");
		}
		else
		{
			FILETIME* pFT = (FILETIME*)(m_pCondition->ftCreationTime + 1);
			if (pFT->dwHighDateTime == (DWORD)-1 && pFT->dwLowDateTime == (DWORD)-1)
			{
				SetWindowTextW(m_hCreatedUntil, L"Anytime");
			}

			else
			{
				SYSTEMTIME sysTime;
				FileTimeToSystemTime(pFT, &sysTime);

				WCHAR wstr[25];
				StringCchPrintf(wstr, 25, L"%02d.%02d.%04d", sysTime.wDay, sysTime.wMonth, sysTime.wYear);
				SetWindowTextW(m_hCreatedUntil, wstr);
			}
		}
	}
	else
	{
		SetWindowTextW(m_hCreatedSince, L"Anytime");
		SetWindowTextW(m_hCreatedUntil, L"Anytime");
	}

	if (m_pCondition->ftModifyTime)
	{
		//since
		if (*(m_pCondition->ftModifyTime) == 0)
			SetWindowTextW(m_hWrittenSince, L"Anytime");
		else
		{
			FILETIME* pFT = (FILETIME*)m_pCondition->ftModifyTime;
			SYSTEMTIME sysTime;
			FileTimeToSystemTime(pFT, &sysTime);

			WCHAR wstr[25];
			StringCchPrintf(wstr, 25, L"%02d.%02d.%04d", sysTime.wDay, sysTime.wMonth, sysTime.wYear);
			SetWindowTextW(m_hWrittenSince, wstr);
		}

		//until
		if (*(m_pCondition->ftModifyTime + 1) == 0)
		{
			SetWindowTextW(m_hWrittenUntil, L"Anytime");
		}
		else
		{
			FILETIME* pFT = (FILETIME*)(m_pCondition->ftModifyTime + 1);
			if (pFT->dwHighDateTime == (DWORD)-1 && pFT->dwLowDateTime == (DWORD)-1)
			{
				SetWindowTextW(m_hWrittenUntil, L"Anytime");
			}

			else
			{
				SYSTEMTIME sysTime;
				FileTimeToSystemTime(pFT, &sysTime);

				WCHAR wstr[25];
				StringCchPrintf(wstr, 25, L"%02d.%02d.%04d", sysTime.wDay, sysTime.wMonth, sysTime.wYear);
				SetWindowTextW(m_hWrittenUntil, wstr);
			}
		}
	}
	else
	{
		SetWindowTextW(m_hWrittenSince, L"Anytime");
		SetWindowTextW(m_hWrittenUntil, L"Anytime");
	}
	
}