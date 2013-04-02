#include "DataObject.h"
#include <shlobj.h>


CDataObject::CDataObject(void)
{
	m_cRef = 0;
	m_nNumFormats = 0;
}


CDataObject::~CDataObject(void)
{
}


HRESULT CDataObject::QueryInterface(REFIID riid, void **ppvObject)
{
	// Always set out parameter to NULL, validating it first.
    if (!ppvObject)
        return E_INVALIDARG;
    *ppvObject = NULL;

	if (riid == IID_IUnknown || riid == IID_IDropSource)
    {
        // Increment the reference count and return the pointer.
        *ppvObject = (LPVOID)this;
        AddRef();
        return NOERROR;
    }
    return E_NOINTERFACE;
}

ULONG CDataObject::AddRef()
{
	InterlockedIncrement(&m_cRef);
    return m_cRef;
}

ULONG CDataObject::Release()
{
	// Decrement the object's internal counter.
    ULONG ulRefCount = InterlockedDecrement(&m_cRef);

    if (0 == m_cRef)
    {
        delete this;
    }
    return ulRefCount;
}

HGLOBAL DupGlobalMem(HGLOBAL hMem)
{
    DWORD   len    = GlobalSize(hMem);
    PVOID   source = GlobalLock(hMem);
    PVOID   dest   = GlobalAlloc(GMEM_FIXED, len);

    memcpy_s(dest, len, source, len);
    GlobalUnlock(hMem);

    return dest;
}

HRESULT CDataObject::GetData(FORMATETC *pFormatEtc, STGMEDIUM *pMedium)
{
	int idx;

    // try to match the specified FORMATETC with one of our supported formats
    if((idx = LookupFormatEtc(pFormatEtc)) == -1)
        return DV_E_FORMATETC;

    // found a match - transfer data into supplied storage medium
    pMedium->tymed           = m_FormatEtc[idx].tymed;
	pMedium->pUnkForRelease  = NULL;

    // copy the data into the caller's storage medium
    switch(m_FormatEtc[idx].tymed)
    {
    case TYMED_HGLOBAL:
		//there must be a copy of it, not itself.
        pMedium->hGlobal     = DupGlobalMem(m_StgMedium[idx].hGlobal);
        break;

    default:
        return DV_E_FORMATETC;
    }

    return S_OK;
}

HRESULT CDataObject::GetDataHere(FORMATETC *pFormatEtc, STGMEDIUM *pmedium)
{
	UNREFERENCED_PARAMETER(pFormatEtc);
	UNREFERENCED_PARAMETER(pmedium);

	return DATA_E_FORMATETC;
}

HRESULT CDataObject::QueryGetData(FORMATETC *pFormatEtc)
{
	return (LookupFormatEtc(pFormatEtc) == -1) ? DV_E_FORMATETC : S_OK;
}

HRESULT CDataObject::GetCanonicalFormatEtc(FORMATETC *pFormatEct, FORMATETC *pFormatEtcOut)
{
	*pFormatEtcOut = *pFormatEct;
	pFormatEtcOut->ptd = NULL;
	
	return DATA_S_SAMEFORMATETC;
}

HRESULT CDataObject::SetData(FORMATETC *pFormatEtc, STGMEDIUM *pMedium, BOOL fRelease)
{
	if (pFormatEtc->tymed != TYMED_HGLOBAL)
		return DV_E_TYMED;

	if (pFormatEtc->tymed != pMedium->tymed) return E_INVALIDARG;
	if (fRelease == FALSE) return E_INVALIDARG;

	int nIndex = LookupFormatEtc(pFormatEtc);
	if (nIndex == -1)
	{
		m_FormatEtc[m_nNumFormats] = *pFormatEtc;
		m_StgMedium[m_nNumFormats] = *pMedium;
		m_nNumFormats++;
	}
	else
	{
		m_FormatEtc[nIndex] = *pFormatEtc;
		ReleaseStgMedium(&m_StgMedium[nIndex]);
		m_StgMedium[nIndex] = *pMedium;
	}

	return S_OK;
}

HRESULT CDataObject::EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC **ppEnumFormatEtc)
{
	// only the get direction is supported for OLE
    if(dwDirection == DATADIR_GET)
    {
        // for Win2k+ you can use the SHCreateStdEnumFmtEtc API call, however
        // to support all Windows platforms we need to implement IEnumFormatEtc ourselves.
		return SHCreateStdEnumFmtEtc(m_nNumFormats, m_FormatEtc, ppEnumFormatEtc);
    }
    else
    {
        // the direction specified is not supported for drag+drop
        return E_NOTIMPL;
    }
}

HRESULT CDataObject::DAdvise(FORMATETC *pFormatEtc, DWORD advf, IAdviseSink *pAdviseSink, DWORD *pdwConnection)
{
	UNREFERENCED_PARAMETER(pFormatEtc);
	UNREFERENCED_PARAMETER(advf);
	UNREFERENCED_PARAMETER(pAdviseSink);
	UNREFERENCED_PARAMETER(pdwConnection);

	return OLE_E_ADVISENOTSUPPORTED;
}

HRESULT CDataObject::DUnadvise(DWORD dwConnection)
{
	UNREFERENCED_PARAMETER(dwConnection);
	return OLE_E_ADVISENOTSUPPORTED;
}

HRESULT CDataObject::EnumDAdvise(IEnumSTATDATA **ppEnumAdvise)
{
	UNREFERENCED_PARAMETER(ppEnumAdvise);
	return OLE_E_ADVISENOTSUPPORTED;
}

int CDataObject::LookupFormatEtc(FORMATETC *pFormatEtc)

{
    // check each of our formats in turn to see if one matches
    for(int i = 0; i < m_nNumFormats; i++)
    {
        if((m_FormatEtc[i].tymed    &  pFormatEtc->tymed)   &&
            m_FormatEtc[i].cfFormat == pFormatEtc->cfFormat &&
            m_FormatEtc[i].dwAspect == pFormatEtc->dwAspect)
        {
            // return index of stored format
            return i;
        }
    }

    // error, format not found
    return -1;
}