#include <streams.h>
#include <initguid.h>

#include "guids.h"
#include "BitmapSource.h"

// Setup data

const AMOVIESETUP_MEDIATYPE sudOpPinTypes =
{
    &MEDIATYPE_Video,       // Major type
    &MEDIASUBTYPE_NULL      // Minor type
};


const AMOVIESETUP_PIN sudOpPin =
{
    L"Output",              // Pin string name
    FALSE,                  // Is it rendered
    TRUE,                   // Is it an output
    FALSE,                  // Can we have none
    FALSE,                  // Can we have many
    &CLSID_NULL,            // Connects to filter
    NULL,                   // Connects to pin
    1,                      // Number of types
    &sudOpPinTypes };       // Pin details


const AMOVIESETUP_FILTER sudFilter =
{
	&CLSID_BitmapSource,    // Filter CLSID
    L"Bitmap Source",       // String name
    MERIT_DO_NOT_USE,       // Filter merit
    1,                      // Number pins
    &sudOpPin               // Pin details
};


// COM global table of objects in this dll

CFactoryTemplate g_Templates[] = {
  { L"Bitmap Source"
  , &CLSID_BitmapSource
  , CBitmapSource::CreateInstance
  , NULL
  , &sudFilter }
};

int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);
