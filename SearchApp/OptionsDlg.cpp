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

			else if (key == COptionsDlg::ThisWeek)
			{
				BOOL bResult = DecWeeks(0, localTime, fileTime);
				delete[] wsResult;
				return bResult;
			}

			else if (key == COptionsDlg::ThisMonth)
			{
				localTime.wDay = 1;
			}

			else if (key == COptionsDlg::ThisYear)
			{
				localTime.wDay = 1;
				localTime.wMonth = 1;
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

			else if (key == COptionsDlg::Yesterday)
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
		
		else if (Key >= COptionsDlg::DaysAgo && Key <= COptionsDlg::YearsAgo)
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
			case COptionsDlg::DaysAgo:
				{
					BOOL bResult = DecDays((WORD)nr, localTime, fileTime);
					delete[] wsResult;
					return bResult;
				}

			case COptionsDlg::WeeksAgo:
				{
					BOOL bResult = DecWeeks((WORD)nr, localTime, fileTime);
					delete[] wsResult;
					return bResult;
				}
				break;

			case COptionsDlg::MonthsAgo:
				{
					DecMonth(1, localTime);
				}
				break;

			case COptionsDlg::YearsAgo:
				{
					localTime.wDay = 1;
					localTime.wMonth = 1;
					localTime.wYear -= (WORD)nr;
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
	int nCount = localTime.wDayOfWeek + 7 * nCountDec - 1;
	if  (localTime.wDayOfWeek == 0) nCount += 7;

	return DecDays((WORD)nCount, localTime, fileTime);
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

	KEY item;

	CDoubleList<KEY>KeyList(NULL);

	//filling the list:
	//a. months
	item.nID = COptionsDlg::January;
	StringCchCopyW(item.wsText, 10, L"january");
	KeyList.push_back(item);

	item.nID = COptionsDlg::January;
	StringCchCopyW(item.wsText, 10, L"jan");
	KeyList.push_back(item);

	item.nID = COptionsDlg::February;
	StringCchCopyW(item.wsText, 10, L"february");
	KeyList.push_back(item);

	item.nID = COptionsDlg::February;
	StringCchCopyW(item.wsText, 10, L"feb");
	KeyList.push_back(item);

	item.nID = COptionsDlg::March;
	StringCchCopyW(item.wsText, 10, L"march");
	KeyList.push_back(item);

	item.nID = COptionsDlg::March;
	StringCchCopyW(item.wsText, 10, L"mar");
	KeyList.push_back(item);

	item.nID = COptionsDlg::April;
	StringCchCopyW(item.wsText, 10, L"april");
	KeyList.push_back(item);

	item.nID = COptionsDlg::April;
	StringCchCopyW(item.wsText, 10, L"apr");
	KeyList.push_back(item);

	item.nID = COptionsDlg::May;
	StringCchCopyW(item.wsText, 10, L"may");
	KeyList.push_back(item);

	item.nID = COptionsDlg::June;
	StringCchCopyW(item.wsText, 10, L"june");
	KeyList.push_back(item);

	item.nID = COptionsDlg::June;
	StringCchCopyW(item.wsText, 10, L"jun");
	KeyList.push_back(item);

	item.nID = COptionsDlg::July;
	StringCchCopyW(item.wsText, 10, L"july");
	KeyList.push_back(item);

	item.nID = COptionsDlg::July;
	StringCchCopyW(item.wsText, 10, L"jul");
	KeyList.push_back(item);

	item.nID = COptionsDlg::August;
	StringCchCopyW(item.wsText, 10, L"august");
	KeyList.push_back(item);

	item.nID = COptionsDlg::August;
	StringCchCopyW(item.wsText, 10, L"aug");
	KeyList.push_back(item);

	item.nID = COptionsDlg::September;
	StringCchCopyW(item.wsText, 10, L"september");
	KeyList.push_back(item);

	item.nID = COptionsDlg::September;
	StringCchCopyW(item.wsText, 10, L"sep");
	KeyList.push_back(item);

	item.nID = COptionsDlg::October;
	StringCchCopyW(item.wsText, 10, L"october");
	KeyList.push_back(item);

	item.nID = COptionsDlg::October;
	StringCchCopyW(item.wsText, 10, L"oct");
	KeyList.push_back(item);

	item.nID = COptionsDlg::November;
	StringCchCopyW(item.wsText, 10, L"november");
	KeyList.push_back(item);

	item.nID = COptionsDlg::November;
	StringCchCopyW(item.wsText, 10, L"nov");
	KeyList.push_back(item);

	item.nID = COptionsDlg::December;
	StringCchCopyW(item.wsText, 10, L"december");
	KeyList.push_back(item);

	item.nID = COptionsDlg::December;
	StringCchCopyW(item.wsText, 10, L"dec");
	KeyList.push_back(item);

	//b. days of the week
	item.nID = COptionsDlg::Monday;
	StringCchCopyW(item.wsText, 10, L"monday");
	KeyList.push_back(item);

	item.nID = COptionsDlg::Monday;
	StringCchCopyW(item.wsText, 10, L"mon");
	KeyList.push_back(item);

	item.nID = COptionsDlg::Tuesday;
	StringCchCopyW(item.wsText, 10, L"tuesday");
	KeyList.push_back(item);

	item.nID = COptionsDlg::Tuesday;
	StringCchCopyW(item.wsText, 10, L"tue");
	KeyList.push_back(item);

	item.nID = COptionsDlg::Wednesday;
	StringCchCopyW(item.wsText, 10, L"wednesday");
	KeyList.push_back(item);

	item.nID = COptionsDlg::Wednesday;
	StringCchCopyW(item.wsText, 10, L"wed");
	KeyList.push_back(item);

	item.nID = COptionsDlg::Thursday;
	StringCchCopyW(item.wsText, 10, L"thursday");
	KeyList.push_back(item);

	item.nID = COptionsDlg::Thursday;
	StringCchCopyW(item.wsText, 10, L"thu");
	KeyList.push_back(item);

	item.nID = COptionsDlg::Friday;
	StringCchCopyW(item.wsText, 10, L"friday");
	KeyList.push_back(item);

	item.nID = COptionsDlg::Friday;
	StringCchCopyW(item.wsText, 10, L"fri");
	KeyList.push_back(item);

	item.nID = COptionsDlg::Saturday;
	StringCchCopyW(item.wsText, 10, L"saturday");
	KeyList.push_back(item);

	item.nID = COptionsDlg::Saturday;
	StringCchCopyW(item.wsText, 10, L"sat");
	KeyList.push_back(item);

	item.nID = COptionsDlg::Sunday;
	StringCchCopyW(item.wsText, 10, L"sunday");
	KeyList.push_back(item);

	item.nID = COptionsDlg::Sunday;
	StringCchCopyW(item.wsText, 10, L"sun");
	KeyList.push_back(item);

	//this... (today,...)
	item.nID = COptionsDlg::Today;
	StringCchCopyW(item.wsText, 10, L"today");
	KeyList.push_back(item);

	item.nID = COptionsDlg::ThisWeek;
	StringCchCopyW(item.wsText, 10, L"thisweek");
	KeyList.push_back(item);

	item.nID = COptionsDlg::ThisMonth;
	StringCchCopyW(item.wsText, 10, L"thismonth");
	KeyList.push_back(item);

	item.nID = COptionsDlg::ThisYear;
	StringCchCopyW(item.wsText, 10, L"thisyear");
	KeyList.push_back(item);

	//last...
	item.nID = COptionsDlg::Yesterday;
	StringCchCopyW(item.wsText, 10, L"yesterday");
	KeyList.push_back(item);

	item.nID = COptionsDlg::Yesterday;
	StringCchCopyW(item.wsText, 10, L"lastday");
	KeyList.push_back(item);

	item.nID = COptionsDlg::LastWeek;
	StringCchCopyW(item.wsText, 10, L"lastweek");
	KeyList.push_back(item);

	item.nID = COptionsDlg::LastMonth;
	StringCchCopyW(item.wsText, 10, L"lastmonth");
	KeyList.push_back(item);

	item.nID = COptionsDlg::LastYear;
	StringCchCopyW(item.wsText, 10, L"lastyear");
	KeyList.push_back(item);

	//anytime:
	item.nID = COptionsDlg::Anytime;
	StringCchCopyW(item.wsText, 10, L"anytime");
	KeyList.push_back(item);

	item.nID = COptionsDlg::Anytime;
	StringCchCopyW(item.wsText, 10, L"any");
	KeyList.push_back(item);

	//days/weeks/months/years ago
	item.nID = COptionsDlg::DaysAgo;
	StringCchCopyW(item.wsText, 10, L"daysago");
	KeyList.push_back(item);

	item.nID = COptionsDlg::DaysAgo;
	StringCchCopyW(item.wsText, 10, L"days");
	KeyList.push_back(item);

	item.nID = COptionsDlg::WeeksAgo;
	StringCchCopyW(item.wsText, 10, L"weeksago");
	KeyList.push_back(item);

	item.nID = COptionsDlg::WeeksAgo;
	StringCchCopyW(item.wsText, 10, L"weeks");
	KeyList.push_back(item);

	item.nID = COptionsDlg::MonthsAgo;
	StringCchCopyW(item.wsText, 10, L"monthsago");
	KeyList.push_back(item);

	item.nID = COptionsDlg::MonthsAgo;
	StringCchCopyW(item.wsText, 10, L"months");
	KeyList.push_back(item);

	item.nID = COptionsDlg::YearsAgo;
	StringCchCopyW(item.wsText, 10, L"yearsago");
	KeyList.push_back(item);

	item.nID = COptionsDlg::YearsAgo;
	StringCchCopyW(item.wsText, 10, L"years");
	KeyList.push_back(item);

	//compare the string from wsStart until wPos, to see if it is one of the keywords:
	//i = position in the string
	int i = 0;
	for (WCHAR* wch = wsStart; wch != wPos; wch++, i++)
	{
		CDoubleList<KEY>::Iterator J;
		for (J = KeyList.begin(); J != NULL;)
		{
			if (*(J->m_Value.wsText + i) != *wch)
				KeyList.erase(J);
			else J = J->pNext;
		}

		if (KeyList.is_empty()) break;
	}

	while (!KeyList.is_empty())
	{
		if (i < (int)wcslen(KeyList.begin()->m_Value.wsText)) KeyList.pop_front();
		//else i == len (it cannot be > because it would have been removed already)
		else break;
	}

	if (KeyList.is_empty()) return false;

	key = KeyList.begin()->m_Value.nID;

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