#pragma once

#ifndef FILEOPERATIONPROGRESSSINK_H
#define FILEOPERATIONPROGRESSSINK_H

#include "shobjidl.h"
class FileOperationProgressSink :
	public IFileOperationProgressSink
{
public:
	FileOperationProgressSink(void);
	~FileOperationProgressSink(void);

	ULONG __stdcall Release();
	HRESULT __stdcall QueryInterface(const IID &riid, void** ppvObject);
	ULONG __stdcall AddRef();

	HRESULT __stdcall PauseTimer();
	HRESULT __stdcall ResetTimer();
	HRESULT __stdcall ResumeTimer();
	HRESULT __stdcall FinishOperations(HRESULT hResult);
	HRESULT __stdcall PostCopyItem(DWORD dwFlags, IShellItem* psiItem, IShellItem* psiDestinationFolder, LPCWSTR pszNewName, HRESULT hrCopy, IShellItem* psiNewlyCreated);
	HRESULT __stdcall PostDeleteItem(DWORD dwFlags, IShellItem* psiItem, HRESULT hrDelete, IShellItem* psiNewlyCreated);
	HRESULT __stdcall PostMoveItem(DWORD dwFlags, IShellItem* psiItem, IShellItem* psiDestinationFolder, LPCWSTR pszNewName, HRESULT hrMove, IShellItem* psiNewlyCreated);
	HRESULT __stdcall PostNewItem(DWORD dwFlags, IShellItem* psiDestinationFolder, LPCWSTR pszNewName, LPCWSTR pszTemplateName, DWORD dwFileAttributes, HRESULT hrNew, IShellItem* psiNewItem);
	HRESULT __stdcall PostRenameItem(DWORD dwFlags, IShellItem* psiItem, LPCWSTR pszNewName, HRESULT hrRename, IShellItem* psiNewlyCreated);
	HRESULT __stdcall PreCopyItem(DWORD dwFlags, IShellItem* psiItem, IShellItem* psiDestinationFolder, LPCWSTR pszNewName);
	HRESULT __stdcall PreDeleteItem(DWORD dwFlags, IShellItem* psiItem);
	HRESULT __stdcall PreMoveItem(DWORD dwFlags, IShellItem* psiItem, IShellItem* psiDestinationFolder, LPCWSTR pszNewName);
	HRESULT __stdcall PreNewItem(DWORD dwFlags, IShellItem* psiDestinationFolder, LPCWSTR pszNewName);
	HRESULT __stdcall PreRenameItem(DWORD dwFlags, IShellItem* psiItem, LPCWSTR pszNewName);
	HRESULT __stdcall StartOperations(void);
	HRESULT __stdcall UpdateProgress(UINT iWorkTotal, UINT iWorkSoFar);

	ULONG m_cRef;
};

#endif// FILEOPERATIONPROGRESSSINK_H