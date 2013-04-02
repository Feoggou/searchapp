#include "FileOperationProgressSink.h"


FileOperationProgressSink::FileOperationProgressSink(void)
{
	m_cRef = 0;//1;
}


FileOperationProgressSink::~FileOperationProgressSink(void)
{
}


HRESULT FileOperationProgressSink::FinishOperations(HRESULT hResult)
{
	UNREFERENCED_PARAMETER(hResult);
	return S_OK;
}


HRESULT FileOperationProgressSink::PostCopyItem(DWORD dwFlags, IShellItem* psiItem, IShellItem* psiDestinationFolder, LPCWSTR pszNewName, HRESULT hrCopy, IShellItem* psiNewlyCreated)
{
	UNREFERENCED_PARAMETER(dwFlags);
	UNREFERENCED_PARAMETER(psiItem);
	UNREFERENCED_PARAMETER(psiDestinationFolder);
	UNREFERENCED_PARAMETER(pszNewName);
	UNREFERENCED_PARAMETER(hrCopy);
	UNREFERENCED_PARAMETER(psiNewlyCreated);

	return E_NOTIMPL;
}


HRESULT FileOperationProgressSink::PostDeleteItem(DWORD dwFlags, IShellItem* psiItem, HRESULT hrDelete, IShellItem* psiNewlyCreated)
{
	UNREFERENCED_PARAMETER(dwFlags);
	UNREFERENCED_PARAMETER(psiItem);
	UNREFERENCED_PARAMETER(hrDelete);
	UNREFERENCED_PARAMETER(psiNewlyCreated);

	return S_OK;
}


HRESULT FileOperationProgressSink::PostMoveItem(DWORD dwFlags, IShellItem* psiItem, IShellItem* psiDestinationFolder, LPCWSTR pszNewName, HRESULT hrMove, IShellItem* psiNewlyCreated)
{
	UNREFERENCED_PARAMETER(dwFlags);
	UNREFERENCED_PARAMETER(psiItem);
	UNREFERENCED_PARAMETER(psiDestinationFolder);
	UNREFERENCED_PARAMETER(pszNewName);
	UNREFERENCED_PARAMETER(hrMove);
	UNREFERENCED_PARAMETER(psiNewlyCreated);

	return E_NOTIMPL;
}


HRESULT FileOperationProgressSink::PostNewItem(DWORD dwFlags, IShellItem* psiDestinationFolder, LPCWSTR pszNewName, LPCWSTR pszTemplateName, DWORD dwFileAttributes, HRESULT hrNew, IShellItem* psiNewItem)
{
	UNREFERENCED_PARAMETER(dwFlags);
	UNREFERENCED_PARAMETER(psiDestinationFolder);
	UNREFERENCED_PARAMETER(pszNewName);
	UNREFERENCED_PARAMETER(pszTemplateName);
	UNREFERENCED_PARAMETER(dwFileAttributes);
	UNREFERENCED_PARAMETER(hrNew);
	UNREFERENCED_PARAMETER(psiNewItem);

	return E_NOTIMPL;
}


HRESULT FileOperationProgressSink::PostRenameItem(DWORD dwFlags, IShellItem* psiItem, LPCWSTR pszNewName, HRESULT hrRename, IShellItem* psiNewlyCreated)
{
	UNREFERENCED_PARAMETER(psiNewlyCreated);
	UNREFERENCED_PARAMETER(dwFlags);
	UNREFERENCED_PARAMETER(psiItem);
	UNREFERENCED_PARAMETER(pszNewName);
	UNREFERENCED_PARAMETER(hrRename);

	return E_NOTIMPL;
}


HRESULT FileOperationProgressSink::PreCopyItem(DWORD dwFlags, IShellItem* psiItem, IShellItem* psiDestinationFolder, LPCWSTR pszNewName)
{
	UNREFERENCED_PARAMETER(dwFlags);
	UNREFERENCED_PARAMETER(psiDestinationFolder);
	UNREFERENCED_PARAMETER(pszNewName);
	UNREFERENCED_PARAMETER(psiItem);
	
	return E_NOTIMPL;
}


HRESULT FileOperationProgressSink::PreDeleteItem(DWORD dwFlags, IShellItem* psiItem)
{
	UNREFERENCED_PARAMETER(dwFlags);
	UNREFERENCED_PARAMETER(psiItem);

	return S_OK;
}


HRESULT FileOperationProgressSink::PreMoveItem(DWORD dwFlags, IShellItem* psiItem, IShellItem* psiDestinationFolder, LPCWSTR pszNewName)
{
	UNREFERENCED_PARAMETER(dwFlags);
	UNREFERENCED_PARAMETER(psiDestinationFolder);
	UNREFERENCED_PARAMETER(pszNewName);
	UNREFERENCED_PARAMETER(psiItem);
	
	return E_NOTIMPL;
}


HRESULT FileOperationProgressSink::PreNewItem(DWORD dwFlags, IShellItem* psiDestinationFolder, LPCWSTR pszNewName)
{
	UNREFERENCED_PARAMETER(dwFlags);
	UNREFERENCED_PARAMETER(psiDestinationFolder);
	UNREFERENCED_PARAMETER(pszNewName);
	
	return E_NOTIMPL;
}


HRESULT FileOperationProgressSink::PreRenameItem(DWORD dwFlags, IShellItem* psiItem, LPCWSTR pszNewName)
{
	UNREFERENCED_PARAMETER(dwFlags);
	UNREFERENCED_PARAMETER(pszNewName);
	UNREFERENCED_PARAMETER(psiItem);

	return E_NOTIMPL;
}


HRESULT FileOperationProgressSink::StartOperations(void)
{
	return S_OK;
}


HRESULT FileOperationProgressSink::UpdateProgress(UINT iWorkTotal, UINT iWorkSoFar)
{
	UNREFERENCED_PARAMETER(iWorkTotal);
	UNREFERENCED_PARAMETER(iWorkSoFar);

	return S_OK;
}

HRESULT FileOperationProgressSink::PauseTimer()
{
	return S_OK;
}

HRESULT FileOperationProgressSink::ResetTimer()
{
	return S_OK;
}

HRESULT FileOperationProgressSink::ResumeTimer()
{
	return S_OK;
}

ULONG FileOperationProgressSink::Release()
{
	// Decrement the object's internal counter.
    ULONG ulRefCount = InterlockedDecrement(&m_cRef);
    if (0 == m_cRef)
    {
        delete this;
    }
    return ulRefCount;
}

HRESULT FileOperationProgressSink::QueryInterface(const IID &riid, void** ppvObject)
{
	// Always set out parameter to NULL, validating it first.
    if (!ppvObject)
        return E_INVALIDARG;
    *ppvObject = NULL;

	if (riid == IID_IUnknown || riid == IID_IFileOperationProgressSink)
    {
        // Increment the reference count and return the pointer.
        *ppvObject = (LPVOID)this;
        AddRef();
        return NOERROR;
    }
    return E_NOINTERFACE;
}

ULONG FileOperationProgressSink::AddRef()
{
	InterlockedIncrement(&m_cRef);
    return m_cRef;
}