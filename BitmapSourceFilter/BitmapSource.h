#pragma once

#include "IBitmapSource.h"

class CBitmapSource : public CSource, public IBitmapSource
{
public:
    DECLARE_IUNKNOWN;
    static CUnknown * WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);

    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

	// IBitmapSource method
	STDMETHODIMP GetBuffer(BYTE** ppBuffer, int* pWidth, int* pHeight, int* pStride);
	STDMETHODIMP SwapBuffer();

	// internal method
	BYTE* GetCurrentBuffer();

	CCritSec m_cSharedState;

private:
    CBitmapSource(LPUNKNOWN lpunk, HRESULT *phr);
	~CBitmapSource();

private:
	BYTE*	m_pBuffer[2];
	int		m_iWidth;
	int		m_iHeight;
	int		m_iStride;
	long	m_iCurrentBuffer;
};
