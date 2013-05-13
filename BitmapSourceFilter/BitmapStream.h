#pragma once

class CBitmapSource;

class CBitmapStream : public CSourceStream
{
public:
	CBitmapStream(HRESULT *phr, CBitmapSource *pParent, int iWidth, int iHeight, LPCWSTR pPinName);
	~CBitmapStream();

	// plots a ball into the supplied video frame
	HRESULT FillBuffer(IMediaSample *pms);

	// Ask for buffers of the size appropriate to the agreed media type
	HRESULT DecideBufferSize(IMemAllocator *pIMemAlloc,
							 ALLOCATOR_PROPERTIES *pProperties);

	HRESULT CheckMediaType(const CMediaType *pMediaType);
	HRESULT SetMediaType(const CMediaType *pMediaType);
	HRESULT GetMediaType(int iPosition, CMediaType *pmt);
	HRESULT GetMediaType(CMediaType *pmt) { return GetMediaType(0, pmt); };

	// Resets the stream time to zero
	HRESULT OnThreadCreate(void);

	// Quality control notifications sent to us
	STDMETHODIMP Notify(IBaseFilter * pSender, Quality q);

private:
	CBitmapSource* m_pParent;

	int m_iWidth;
	int m_iHeight;
	int m_iRepeatTime;
	const int m_iDefaultRepeatTime;

	CCritSec m_cSharedState;
	CRefTime m_rtSampleTime;
};
