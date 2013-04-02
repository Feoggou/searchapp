#pragma once

#ifndef OPTIONSDLG_H
#define OPTIONSDLG_H

#include <windows.h>
#include "SearchAppDlg.h"

class COptionsDlg
{
private:
	//private data:
	static WNDPROC		OldEditTimeProc;
	FILETIME m_ftCreatedSince, m_ftWrittenSince, m_ftAccessedSince;
	FILETIME m_ftCreatedUntil, m_ftWrittenUntil, m_ftAccessedUntil;

	HWND		m_hCreatedSince, m_hCreatedUntil;
	HWND		m_hWrittenSince, m_hWrittenUntil;
	HWND		m_hAccessedSince, m_hAccessedUntil;

	CONDITION*	m_pCondition;

	enum KEYWORD
	{
		Anytime,
		//months
		January, February, March, April, May, June, July, August, September, October, November, December,

		//days of the week:
		Monday, Tuesday, Wednesday, Thursday, Friday, Saturday, Sunday,

		//this:
		Today, ThisWeek, ThisMonth, ThisYear,

		//last
		Yesterday, LastWeek, LastMonth, LastYear,

		//counts
		DaysAgo, WeeksAgo, MonthsAgo, YearsAgo,
	};

	struct KEY
	{
		KEYWORD nID;
		WCHAR wsText[10];
	};

public:
	//public data:
	HWND		m_hWnd;

	//private functions
private:
	static INT_PTR CALLBACK DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	//the procedure of a time edit
	static LRESULT CALLBACK NewEditTimeProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	INT_PTR OnInitDialog();
	BOOL OnOK();
	//checks to see if every "time" is set correctly
	BOOL CheckTimes(void);
	//checks the time text.
	BOOL CheckTimeText(WCHAR* wsText, int len, FILETIME& fileTime, BOOL bIsSince);
	//transforms a day-of-the-week into a date
	BOOL CalcDay(int iChosenDay, SYSTEMTIME& localTime, FILETIME& fileTime);
	//decrease by a number of months
	void DecMonth(WORD nCountDec, SYSTEMTIME& localTime);
	//decrease by a number of days
	BOOL DecDays(WORD nCountDec, SYSTEMTIME& localTime, FILETIME& fileTime);
	//decrease by a number of weeks
	BOOL DecWeeks(WORD nCountDec, SYSTEMTIME& localTime, FILETIME& fileTime);
	//retrieves the next integer from the string and returns the position after the end of the integer
	BOOL GetNextInt(int& nr, WCHAR** wPos);
	//retrieves the keyword of the string that follows and returns the position after the end of that string.
	BOOL GetNextString(WCHAR*& wPos, KEYWORD& key);
	//initializes the time boxes with the data set before.
	void InitializeTimeBoxes(void);

public:
	//public functions:
	COptionsDlg(void);
	~COptionsDlg(void);

	INT_PTR DoModal(HWND hParent, CONDITION* pCondition);
};

#endif//OPTIONSDLG_H