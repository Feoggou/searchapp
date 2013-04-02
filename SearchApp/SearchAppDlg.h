#pragma once

#include "General.h"

#ifndef SEARCHAPPDLG_H
#define SEARCHAPPDLG_H

struct FILEITEM
{
	WCHAR* wsFullName;
	WCHAR* wsDisplayName;
	//including the dot:
	WCHAR* wsExt;
	WIN32_FILE_ATTRIBUTE_DATA data;
	int dIcon;

	FILEITEM()
	{
		dIcon = -1;
		wsFullName = NULL;
		wsDisplayName = NULL;
		wsExt = NULL;
	}
	~FILEITEM()
	{
		//sFileName should be destroyed by the deque!
	}
};

struct CONDITION
{
	DWORD dwAttribON, dwAttribOFF;
	QWORD* ftCreationTime, *ftModifyTime, *ftAccessTime;
	QWORD* Size;
	WCHAR* wsName;
	//case sensitive search for FileName, Contents?
	BOOL m_bCSFileName, m_bCSContents;

	CONDITION()
	{
		ftCreationTime = NULL;
		ftModifyTime = NULL;
		ftAccessTime = NULL;
		Size = NULL;
		wsName = NULL;
	}

	~CONDITION()
	{
		if (ftCreationTime)
		{
			delete[] ftCreationTime;
			ftCreationTime = NULL;
		}
		if (ftModifyTime)
		{
			delete[] ftModifyTime;
			ftModifyTime = NULL;
		}
		if (ftAccessTime)
		{
			delete[] ftAccessTime;
			ftAccessTime = NULL;
		}
		if (Size)
		{
			delete[] Size;
			Size = NULL;
		}
		if (wsName)
		{
			delete[] wsName;
			wsName = NULL;
		}
	}
};

struct ITEM
{
	//the index from the list
	int nIndex;
	//pointer to the string that represents the full name of the item.
	WCHAR* wsFullName;
	//pointer to the string that represents the display name of the item.
	WCHAR* wsDisplayName;
	//the length of the folder path (wsFullName), in chars.
	int len;
};

struct FILTER
{
	WCHAR* wsExt;
	IClassFactory* pClassFactory;
};

struct FILTERLIST
{
	CDoubleList<FILTER>* pFilterList;
	WCHAR letter;
};

class CSearchAppDlg
{
public:
	HWND		m_hWnd;

	enum KEYSIZE { Bytes, KBytes, MBytes, GBytes};

private:
	typedef BOOL (*CHARCHECKENC) (const BYTE* wcContents, const BYTE* wcSearch, DWORD& dwEnc, int* nr, int nrMaxSearch);
	typedef BOOL (*SEARCHUSINGFILTER) (const WCHAR* wsFullName, const WCHAR* wsSearchContents, IClassFactory* pClassFactory, WIN32_FILE_ATTRIBUTE_DATA& data);
	CHARCHECKENC CharCheckEnc;
	SEARCHUSINGFILTER SearchUsingFilter;

//PRIVATE DATA
private:
	//SEARCH
	//the condition set by the user (i.e. options for search)
	CONDITION m_Condition;
	//the current item index, used for inserting/setting items.
	int m_nCurrentItem;
	//should I search fast? (ignore item date, size, type)
	BOOL m_bSearchFast;
	//the list of fileitems currently in the ListCtrl.
	CDoubleList<FILEITEM> m_FileItems;
	//the list of filters:
	CDoubleList<FILTERLIST> m_FilterList;
	//flags for enumeration.
	SHCONTF m_ulEnumFlags;
	QWORD m_SizeMin, m_SizeMax;
	static WNDPROC		OldEditSizeProc;

	static BOOL m_bStopSearch;

	//UI
	//the rectangles of controls: used for reposition/re-sizing.
	RECT m_DefDlgRect, m_DefButBrowseRect, m_DefEditBrowseRect, m_DefListCtrlRect;
	//the image lists. I only need m_hImageListSmall. Anyway, they are not allocated in my application. only retrieved.
	HIMAGELIST m_hImageListSmall, m_hImageList;

	//SORTING
	//how is the list currently sorted?
	enum Sorted{SortedUnknown, AscByName, DescByName, AscByPath, DescByPath, AscByDate, DescByDate, AscByType, DescByType} m_dSorted;
	//compare the display names of 2 items. Used for sorting when the user clicks a header item.
	void CompareDisplayNames(CDoubleList<FILEITEM>::Iterator I, CDoubleList<FILEITEM>::Iterator J);
	//compare the path names of 2 items. Used for sorting when the user clicks a header item.
	void ComparePathNames(CDoubleList<FILEITEM>::Iterator I, CDoubleList<FILEITEM>::Iterator J);
	//compare the filetypes of 2 items. Used for sorting when the user clicks a header item.
	void CompareFileTypes(CDoubleList<FILEITEM>::Iterator I, CDoubleList<FILEITEM>::Iterator J);

	//strings:
	WCHAR* m_wsSearchIn;
	WCHAR* m_wsSearchFileName;
	WCHAR* m_wsSearchContents;
	//uses the same buffer as m_wsSearchFileName
	WCHAR* m_wsSearchFileExt;
	WCHAR m_wsDesktopPath[MAX_PATH];
	WCHAR m_wsTempFolder[288];
	int m_nTempFolderLen;

	//handles to controls
	HWND m_hSearchFileName;
	HWND m_hSearchContents;
	HWND m_hSearchIn;
	HWND m_hListCtrl;
	HWND m_hBrowse;
	HICON m_hIcon;
	static HWND m_hTaskWnd;

	static HANDLE m_hSecondThread;

	HWND		m_hSizeMin, m_hSizeMax;

	ULONG m_uRegisteredValue;

//PRIVATE FUNCTIONS
private:
	static INT_PTR CALLBACK DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static DWORD WINAPI SecondThread(CSearchAppDlg* pDlg);
	static LRESULT CALLBACK SecondWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	//MESSAGE HANDLERS
	BOOL OnInitDialog(void);
	//the user or something has resized the window
	void OnSize(UINT nType, WORD cx, WORD cy);
	//the user has pressed the "Search" button
	inline void OnSearch();
	//the user has clicked an item in the Header Control
	void OnHeaderItemClicked(int iItem);
	//the user has right-clicked an item - is it an item??
	void OnListItemContextMenu(int nItem, POINT& ptAction);
	static LRESULT CALLBACK MessageFilter(int nCode, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK NewEditSizeProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	
	//UTILITY FUNCTIONS
	//initialization purposes: saves the client rects (position + size) to help for sizing
	inline void SetClientRects(void);
	//searches the folder pSearchFolder and all its contents
	void SearchFolder(IShellFolder* pSearchFolder);
	//deletes the items specified in pidlChildren
	void OnDeleteItems(ITEMIDLIST** pidlChildren, int nSize);
	//deletes permanently the items specified in pidlChildren
	void OnPermanentDeleteItems(ITEMIDLIST** pidlChildren, int nSize);
	//cuts the specified items
	void CutItems(CDoubleList<ITEM>& Items);
	//copies the specified items:
	void CopyItems(CDoubleList<ITEM>& Items);
	//eliminates files & folders if the parent folder is also selected
	void FilterSelectedItems(IShellFolder* pDesktopFolder, CDoubleList<ITEM>* Items, int& nSize, ITEMIDLIST*** Children);
	//gets either or both desktop folder and current folder, when the folder path is specified.
	BOOL GetShellFolder(WCHAR* wsFolderName, IShellFolder** pDesktopFolder, IShellFolder** pCurrentFolder, ITEMIDLIST** pidlCurFolder);
	//updates the m_Condition from the changed values.
	void UpdateCondition();
	void InitializeSizeBoxes(void);
	BOOL GetNextStringToSize(WCHAR*& wPos, KEYSIZE& key);
	BOOL GetNextQWORD(QWORD& nr, WCHAR** wPos);
	BOOL CheckSizeText(WCHAR* wsText, int len, QWORD& qwSize, BOOL bIsMin);
	BOOL CheckSizes(void);
	void WatchFolder(ITEMIDLIST* pidlFolder);
	void OnShellNotify(WPARAM wParam, LPARAM lParam);

	//shell notifications:
	//file deleted
	void OnNotifyDeleteFile(WCHAR* wsFileName);
	//folder deleted
	void OnNotifyDeleteFolder(WCHAR* wsFolderName);
	//file renamed
	void OnNotifyRenameFile(WCHAR* wsOldFileName, WCHAR* wsNewFileName);
	//folder renamed
	void OnNotifyRenameFolder(WCHAR* wsOldFolderName, WCHAR* wsNewFolderName);

	//DATA RETRIEVAL
	//retrieves the parent folder of the file wsFileName. It does not allocate new space. it only retrieves the first
	//character of the display name.
	inline void GetDisplayName(const WCHAR* wsFullName, WCHAR** wsDisplayName);
	//retrieves the file extension from the filename. optionally with the dot ('.')
	//the filetype of a folder is NULL.
	//It does not allocate new space. it only retrieves the first
	//character of the extension.
	inline void GetFileExt(const WCHAR* wsDisplayName, WCHAR** wsFileExt, BOOL bRetrieveDot = FALSE);
	void LoadFilters();
	void TryLoadFilter(const HKEY hParentKey, WCHAR* wsKeyName, WCHAR& lastchar);

	//CHECKS
	//checks an item: be it folder or file (pidlChild)
	void CheckItem(IShellFolder* pSearchFolder, ITEMIDLIST* pidlChild, ULONG& ulFlags);
	//check the flags retrieved from pSearchFolder->GetAttributesOf()
	inline BOOL CheckFlags(const ULONG& ulFlags);
	//checks an item's attributes to see if it matches attributes' condition
	BOOL CheckAttributes(const WIN32_FILE_ATTRIBUTE_DATA& data);
	//retrieve the next char available encodings, and the number of character it needs for each encoding.
	BOOL GetNextChar(const BYTE* wPos, DWORD& dwEnc, DWORD nMax, int nr[]);
	//retrieve the next char available encodings, and the number of character it needs for each encoding.
	//if it needed more than it has, it returns false and bNeedMore is set to TRUE.
	//nMaxFile = nr total of bytes left in the file. if there is still space in the file and bNeedMore is true,
	//we can afford to return false and change the buffer. else, we need to return ANY encodings.
	BOOL GetNextChar(const BYTE* wPos, DWORD& dwEnc, DWORD nMax, int nr[], BOOL& bNeedMore, const QWORD& nMaxFile);
	//this is for Case Sensitive:
	static BOOL CheckCharEncCS(const BYTE* wcContents, const BYTE* wcSearch, DWORD& dwEnc, int* nr, int nrMaxSearch);
	//we retrieve each char as its encodings and compare it to the current character in the search str.
	//the encodings that fail will have the "nr" set to 0.
	static BOOL CheckCharEnc(const BYTE* wcContents, const BYTE* wcSearch, DWORD& dwEnc, int* nr, int nrMaxSearch);

	//checks an item's extension name to see if it matches extension-name condition
//	bool CheckExtName(WCHAR* wsFileExt);
	//checks an item's title name to see if it matches title-name condition: case when there is a file extension
//	bool CheckTitleName(WCHAR* wsDisplayName, WCHAR* wsFileExt);
	//checks an item's title name to see if it matches title-name condition: case when there is no file extension
	bool CheckDisplayName(WCHAR* wsDisplayName);

	inline BOOL CheckFileContents(const WCHAR* wsFullName, const WCHAR* wsExtName, WIN32_FILE_ATTRIBUTE_DATA& data);
	BOOL CheckDefaultContents(HANDLE hFile, LARGE_INTEGER& liSize);
	IClassFactory* FindFilter(const WCHAR* wsExtName);
	static BOOL SearchUsingFilterCS(const WCHAR* wsFullName, const WCHAR* wsSearchContents, IClassFactory* pClassFactory, WIN32_FILE_ATTRIBUTE_DATA& data);
	static BOOL SearchUsingFilterNCS(const WCHAR* wsFullName, const WCHAR* wsSearchContents, IClassFactory* pClassFactory, WIN32_FILE_ATTRIBUTE_DATA& data);

	BOOL CheckUTF8Char(BYTE* wcContents, WCHAR* wcSearch, int& nrChars, int& nrWCHARs);
	BOOL GetNextUTF8Char(BYTE* wcContents, DWORD nMax, int& nr);

//PUBLIC FUNCTIONS
public:
	CSearchAppDlg(void);
	~CSearchAppDlg(void);

	INT_PTR DoModal(void);
	//similar to CWnd::ScreenToClient
	inline void ScreenToClient(RECT& rect);
	inline bool PathFileExistsEx(const WCHAR* wsPath, BOOL bMustBeDir = FALSE);
};

#endif//SEARCHAPPDLG_H