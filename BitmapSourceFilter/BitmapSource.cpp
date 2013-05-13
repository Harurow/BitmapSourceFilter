#include <streams.h>
#include <olectl.h>
#include <initguid.h>

#include "guids.h"
#include "BitmapSource.h"
#include "BitmapStream.h"


CUnknown * WINAPI CBitmapSource::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr)
{
    ASSERT(phr);

    CUnknown *punk = new CBitmapSource(lpunk, phr);
    if(punk == NULL)
    {
        if(phr)
            *phr = E_OUTOFMEMORY;
    }

    return punk;
}


CBitmapSource::CBitmapSource(LPUNKNOWN lpunk, HRESULT *phr)
	: CSource(NAME("Bitmap Source"), lpunk, CLSID_BitmapSource),
	  m_iWidth(320), m_iHeight(240), m_iCurrentBuffer(0L)
{
    ASSERT(phr);
    CAutoLock cAutoLock(&m_cStateLock);

	// RGB24
	m_iStride = ((m_iWidth * 3) + 3) / 4 * 4;
	int bufSize = m_iStride * m_iHeight;

	m_pBuffer[0] = new BYTE[bufSize];
	if (m_pBuffer[0] == NULL)
	{
		if(phr)
			*phr = E_OUTOFMEMORY;

		return;
	}
	ZeroMemory(m_pBuffer[0], bufSize);

	m_pBuffer[1] = new BYTE[bufSize];
	if (m_pBuffer[1] == NULL)
	{
		delete [] m_pBuffer[0];
		m_pBuffer[0] = NULL;
		if(phr)
			*phr = E_OUTOFMEMORY;

		return;
	}
	ZeroMemory(m_pBuffer[1], bufSize);

    m_paStreams = (CSourceStream **) new CBitmapStream*[1];
    if(m_paStreams == NULL)
    {
        if(phr)
            *phr = E_OUTOFMEMORY;

        return;
    }

	m_paStreams[0] = new CBitmapStream(phr, this, m_iWidth, m_iHeight, L"out");
    if(m_paStreams[0] == NULL)
    {
        if(phr)
            *phr = E_OUTOFMEMORY;

        return;
    }
}


CBitmapSource::~CBitmapSource()
{
	for (int i = 0; i < 2; i++)
	{
		if (m_pBuffer[i])
		{
			delete [] m_pBuffer[i];
			m_pBuffer[i] = NULL;
		}
	}
}


STDMETHODIMP CBitmapSource::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
    CheckPointer(ppv,E_POINTER);

    if (riid == IID_BitmapSource) {
        return GetInterface((IBitmapSource *) this, ppv);
    } else {
        return CSource::NonDelegatingQueryInterface(riid, ppv);
    }
}


STDMETHODIMP CBitmapSource::GetBuffer(BYTE** ppBuffer, int* pWidth, int* pHeight, int* pStride)
{
    CheckPointer(ppBuffer,E_POINTER);
	CheckPointer(pWidth,E_POINTER);
	CheckPointer(pHeight,E_POINTER);
	CheckPointer(pStride,E_POINTER);

	long index = (m_iCurrentBuffer) ? 0 : 1;
	
	if (m_pBuffer[index] == NULL)
		return E_OUTOFMEMORY;

	*ppBuffer = m_pBuffer[index];
	*pWidth = m_iWidth;
	*pHeight = m_iHeight;
	*pStride = m_iStride;

	return S_OK;
}


STDMETHODIMP CBitmapSource::SwapBuffer()
{
	if (m_pBuffer[0] == NULL)
	{
		return E_OUTOFMEMORY;
	}

	CAutoLock lock(&m_cSharedState);
	long index = (m_iCurrentBuffer) ? 0L : 1L;
	m_iCurrentBuffer = index;
	return S_OK;
}


BYTE* CBitmapSource::GetCurrentBuffer()
{
	return m_pBuffer[m_iCurrentBuffer];
}
