#pragma once

#include "guids.h"

extern "C" {

	DECLARE_INTERFACE_(IBitmapSource, IUnknown)
	{
		STDMETHOD(GetBuffer)(THIS_
								BYTE** ppBuffer,
								int* pWidth,
								int* pHeight,
								int* pStride
							) PURE;

		STDMETHOD(SwapBuffer)(THIS_
							) PURE;
	};
}