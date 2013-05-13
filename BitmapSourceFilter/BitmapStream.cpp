#include <streams.h>
#include <olectl.h>
#include <initguid.h>

#include "BitmapStream.h"
#include "BitmapSource.h"


CBitmapStream::CBitmapStream(HRESULT *phr,
							 CBitmapSource *pParent,
							 int iWidth, int iHeight,
							 LPCWSTR pPinName)
	: CSourceStream(NAME("Bitmap Source"),phr, pParent, pPinName),
	  m_pParent(pParent),
	  m_iWidth(iWidth),
	  m_iHeight(iHeight),
	  m_iDefaultRepeatTime(20)
{
	ASSERT(phr);
	CAutoLock lock(&m_cSharedState);
}


CBitmapStream::~CBitmapStream()
{
	CAutoLock lock(&m_cSharedState);
}


HRESULT CBitmapStream::FillBuffer(IMediaSample *pSample)
{
	CheckPointer(pSample, E_POINTER);

	BYTE *pData;
	pSample->GetPointer(&pData);
	long lDataLen = pSample->GetSize();

	ZeroMemory(pData, lDataLen);
	{
		CAutoLock lock(&m_cSharedState);

		{
			CAutoLock lockParent(&m_pParent->m_cSharedState);
			CopyMemory(pData, m_pParent->GetCurrentBuffer(), lDataLen);
		}

		CRefTime rtStart = m_rtSampleTime;

		m_rtSampleTime += (LONG)m_iRepeatTime;

		pSample->SetTime((REFERENCE_TIME *) &rtStart,(REFERENCE_TIME *) &m_rtSampleTime);
	}

	pSample->SetSyncPoint(TRUE);
	
	return NOERROR;
}


STDMETHODIMP CBitmapStream::Notify(IBaseFilter * pSender, Quality q)
{
	if(q.Proportion<=0)
	{
		m_iRepeatTime = 1000;
	}
	else
	{
		m_iRepeatTime = m_iRepeatTime*1000 / q.Proportion;
		if(m_iRepeatTime>1000)
		{
			m_iRepeatTime = 1000;
		}
		else if(m_iRepeatTime<10)
		{
			m_iRepeatTime = 10;
		}
	}

	if(q.Late > 0)
		m_rtSampleTime += q.Late;

	return NOERROR;
}


HRESULT CBitmapStream::CheckMediaType(const CMediaType *pMediaType)
{
	CheckPointer(pMediaType,E_POINTER);

	if((*(pMediaType->Type()) != MEDIATYPE_Video) ||   // we only output video
		!(pMediaType->IsFixedSize()))                  // in fixed size samples
	{                                                  
		return E_INVALIDARG;
	}

	const GUID *SubType = pMediaType->Subtype();
	if (SubType == NULL)
		return E_INVALIDARG;

	if ((*SubType != MEDIASUBTYPE_RGB24))
	{
		return E_INVALIDARG;
	}

	VIDEOINFO *pvi = (VIDEOINFO *) pMediaType->Format();

	if(pvi == NULL)
		return E_INVALIDARG;

	if( pvi->bmiHeader.biWidth != m_iWidth || 
	   abs(pvi->bmiHeader.biHeight) != m_iHeight)
	{
		return E_INVALIDARG;
	}

	if (pvi->bmiHeader.biHeight < 0)
		return E_INVALIDARG;

	return S_OK;
}


HRESULT CBitmapStream::SetMediaType(const CMediaType *pMediaType)
{
	CAutoLock lock(m_pFilter->pStateLock());

	HRESULT hr = CSourceStream::SetMediaType(pMediaType);

	if (SUCCEEDED(hr))
	{
		VIDEOINFO * pvi = (VIDEOINFO *) m_mt.Format();
		if (pvi == NULL)
			return E_UNEXPECTED;

		switch(pvi->bmiHeader.biBitCount)
		{
			case 24:    // RGB24
				hr = S_OK;
				break;

			default:
				ASSERT(FALSE);
				hr = E_INVALIDARG;
				break;
		}
	} 

	return hr;
}


HRESULT CBitmapStream::GetMediaType(int iPosition, CMediaType *pmt)
{
	CheckPointer(pmt,E_POINTER);

	CAutoLock lock(m_pFilter->pStateLock());
	if(iPosition < 0)
	{
		return E_INVALIDARG;
	}

	if(iPosition > 1)
	{
		return VFW_S_NO_MORE_ITEMS;
	}

	VIDEOINFO *pvi = (VIDEOINFO *) pmt->AllocFormatBuffer(sizeof(VIDEOINFO));
	if(NULL == pvi)
		return(E_OUTOFMEMORY);

	ZeroMemory(pvi, sizeof(VIDEOINFO));

	switch(iPosition)
	{
		case 0:
		{
			pvi->bmiHeader.biCompression = BI_RGB;
			pvi->bmiHeader.biBitCount    = 24;
			break;
		}
	}

	// (Adjust the parameters common to all formats...)

	pvi->bmiHeader.biSize       = sizeof(BITMAPINFOHEADER);
	pvi->bmiHeader.biWidth      = m_iWidth;
	pvi->bmiHeader.biHeight     = m_iHeight;
	pvi->bmiHeader.biPlanes     = 1;
	pvi->bmiHeader.biSizeImage  = GetBitmapSize(&pvi->bmiHeader);
	pvi->bmiHeader.biClrImportant = 0;

	SetRectEmpty(&(pvi->rcSource)); // we want the whole image area rendered.
	SetRectEmpty(&(pvi->rcTarget)); // no particular destination rectangle

	pmt->SetType(&MEDIATYPE_Video);
	pmt->SetFormatType(&FORMAT_VideoInfo);
	pmt->SetTemporalCompression(FALSE);

	// Work out the GUID for the subtype from the header info.
	const GUID SubTypeGUID = GetBitmapSubtype(&pvi->bmiHeader);
	pmt->SetSubtype(&SubTypeGUID);
	pmt->SetSampleSize(pvi->bmiHeader.biSizeImage);

	return NOERROR;
}


HRESULT CBitmapStream::DecideBufferSize(IMemAllocator *pAlloc,
									  ALLOCATOR_PROPERTIES *pProperties)
{
	CheckPointer(pAlloc,E_POINTER);
	CheckPointer(pProperties,E_POINTER);

	CAutoLock lock(m_pFilter->pStateLock());
	HRESULT hr = NOERROR;

	VIDEOINFO *pvi = (VIDEOINFO *) m_mt.Format();
	pProperties->cBuffers = 1;
	pProperties->cbBuffer = pvi->bmiHeader.biSizeImage;

	ASSERT(pProperties->cbBuffer);

	// Ask the allocator to reserve us some sample memory, NOTE the function
	// can succeed (that is return NOERROR) but still not have allocated the
	// memory that we requested, so we must check we got whatever we wanted

	ALLOCATOR_PROPERTIES Actual;
	hr = pAlloc->SetProperties(pProperties,&Actual);
	if(FAILED(hr))
	{
		return hr;
	}

	// Is this allocator unsuitable

	if(Actual.cbBuffer < pProperties->cbBuffer)
	{
		return E_FAIL;
	}

	// Make sure that we have only 1 buffer (we erase the ball in the
	// old buffer to save having to zero a 200k+ buffer every time
	// we draw a frame)

	ASSERT(Actual.cBuffers == 1);
	return NOERROR;
}


HRESULT CBitmapStream::OnThreadCreate()
{
	CAutoLock lock(&m_cSharedState);

	m_rtSampleTime = 0;
	m_iRepeatTime = m_iDefaultRepeatTime;

	return NOERROR;
}
