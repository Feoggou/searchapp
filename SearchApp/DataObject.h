#pragma once

#ifndef DATAOBJECT_H
#define DATAOBJECT_H

#include "objidl.h"
class CDataObject :
	public IDataObject
{
public:
	CDataObject(void);
	~CDataObject(void);

	HRESULT __stdcall QueryInterface(REFIID riid, void **ppvObject);
	ULONG __stdcall AddRef();
	ULONG __stdcall Release();

	HRESULT __stdcall GetData(FORMATETC *pFormatEtc, STGMEDIUM *pMedium);
	HRESULT __stdcall GetDataHere(FORMATETC *pFormatEtc, STGMEDIUM *pmedium);
	HRESULT __stdcall QueryGetData(FORMATETC *pFormatEtc);
	HRESULT __stdcall GetCanonicalFormatEtc(FORMATETC *pFormatEct, FORMATETC *pFormatEtcOut);
	HRESULT __stdcall SetData(FORMATETC *pFormatEtc, STGMEDIUM *pMedium, BOOL fRelease);
	HRESULT __stdcall EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC **ppEnumFormatEtc);
	HRESULT __stdcall DAdvise(FORMATETC *pFormatEtc, DWORD advf, IAdviseSink *pAdviseSink, DWORD *pdwConnection);
	HRESULT __stdcall DUnadvise(DWORD dwConnection);
	HRESULT __stdcall EnumDAdvise(IEnumSTATDATA **ppEnumAdvise);

private:
	ULONG m_cRef;

	int m_nNumFormats;
	FORMATETC	m_FormatEtc[4];
	STGMEDIUM	m_StgMedium[4];

	int LookupFormatEtc(FORMATETC *pFormatEtc);
};

#endif//DATAOBJECT_H