#include "General.h"
#include "DSCaptureDevice.h"

DSCaptureDevice::DSCaptureDevice()
{

}

DSCaptureDevice::~DSCaptureDevice()
{
    base_filter_.Release();
    out_pin_.Release();
}

HRESULT DSCaptureDevice::Create(const CString& comObjID)
{
    HRESULT hr;

    // create an enumerator
    CComPtr<ICreateDevEnum> create_dev_enum;
    create_dev_enum.CoCreateInstance(CLSID_SystemDeviceEnum);

    // enumerate video capture devices
    CComPtr<IEnumMoniker> enum_moniker;
    create_dev_enum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &enum_moniker, 0);

    if (!enum_moniker)
    {
        // video input device can't be found with the given id
        // trying to find an audio input device
        //
        create_dev_enum->CreateClassEnumerator(CLSID_AudioInputDeviceCategory, &enum_moniker, 0);

        if (!enum_moniker)
        {
            // no capture device found with the given id
            return -1;
        }
    }

    enum_moniker->Reset();

    IBindCtx* bind_ctx;
    hr = ::CreateBindCtx( 0, &bind_ctx );
    if( hr != S_OK )
    {
        // the given capture device ID is not valid
        return -2;
    }

    ULONG ulFetched;
    CComPtr<IMoniker> moniker;

    // getting device moniker from given id (comObjID)
    //
    hr = ::MkParseDisplayName( bind_ctx, comObjID, &ulFetched, &moniker );
    SAFE_RELEASE( bind_ctx );

    if( hr != S_OK )
    {
        // the given capture device ID is not valid
        return -2;
    }

    // ask for the actual filter
    hr = moniker->BindToObject(0,0,IID_IBaseFilter, (void **)&base_filter_);
    if( hr != S_OK )
    {
        // the given capture device ID is not valid
        return -2;
    }

    // get the property bag interface from the moniker
    CComPtr< IPropertyBag > pBag;
    hr = moniker->BindToStorage( 0, 0, IID_IPropertyBag, (void**) &pBag );
    if( hr != S_OK )
    {
        // the given capture device ID is not valid
        return -2;
    }

    // ask for the english-readable name
    CComVariant var;
    var.vt = VT_BSTR;
    pBag->Read( L"FriendlyName", &var, NULL );

    LPOLESTR szDeviceID;
    moniker->GetDisplayName( 0, NULL, &szDeviceID );

    com_obj_id_ = comObjID;
    device_name_ = var.bstrVal;

    return 0;
}
