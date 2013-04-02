#pragma once
#include <windows.h>

#ifndef ABOUTDLG_H
#define ABOUTDLG_H

class CAboutDlg
{
private:
	HWND	m_hWnd;

	static INT_PTR CALLBACK DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	INT_PTR OnInitDialog();

public:
	void DoModal(HWND hParent);
	CAboutDlg(void);
	~CAboutDlg(void);
};

#endif//ABOUTDLG_H