#include "General.h"
#include "DSGraph.h"

DSGraph::DSGraph()
{
    graph_builder_.CoCreateInstance(CLSID_FilterGraph);
    if (!graph_builder_)
    {
        // failed
    }

    ListCapDevices(CLSID_AudioInputDeviceCategory, audio_cap_devices_);
}

DSGraph::~DSGraph()
{
    if (graph_builder_)
    {
        Stop();
        source_filter_.Release();
        grabber_.Release();
        graph_builder_.Release();
    }
}

void DSGraph::Start()
{
    CComQIPtr<IMediaControl, &IID_IMediaControl> control(graph_builder_);
    if (control)
    {
        control->Run();
        control.Release();
    }
}

void DSGraph::Stop()
{
    CComQIPtr<IMediaControl, &IID_IMediaControl> control(graph_builder_);
    if (control)
    {
        control->Stop();
        control.Release();
    }
}

void DSGraph::ListCapDevices(const IID& deviceIID, std::map<CString, CString>& deviceList)
{
    CComPtr<IEnumMoniker> enum_moniker;

    CComPtr<ICreateDevEnum> create_dev_enum;
    create_dev_enum.CoCreateInstance(CLSID_SystemDeviceEnum);
    create_dev_enum->CreateClassEnumerator(deviceIID, &enum_moniker, 0);
    create_dev_enum.Release();
    if (!enum_moniker)
    {
        return;
    }

    enum_moniker->Reset();

    UINT i = 0;
    while (true)
    {
        i++;
        CComPtr<IMoniker> moniker;

        ULONG ulFetched = 0;
        HRESULT hr = enum_moniker->Next(1, &moniker, &ulFetched);
        if(hr != S_OK)
        {
            break;
        }

        CComPtr< IBaseFilter >* ppCap;
        hr = moniker->BindToObject(0,0,IID_IBaseFilter, (void **) &ppCap);
        if (*ppCap)
        {
            CComPtr< IPropertyBag > pBag;
            hr = moniker->BindToStorage( 0, 0, IID_IPropertyBag, (void**) &pBag );
            if( hr != S_OK )
            {
                continue;
            }

            CComVariant var;
            var.vt = VT_BSTR;
            pBag->Read( L"FriendlyName", &var, NULL );

            LPOLESTR wszDeviceID;
            moniker->GetDisplayName( 0, NULL, &wszDeviceID );

            deviceList[wszDeviceID] = var.bstrVal;
        }
    }
}

IPin* DSGraph::GetInPin(IBaseFilter* filter, int pinIndex)
{
    return GetPin(filter, PINDIR_INPUT, pinIndex);
}

IPin* DSGraph::GetOutPin(IBaseFilter* filter, int pinIndex)
{
    return GetPin(filter, PINDIR_OUTPUT, pinIndex);
}

IPin* DSGraph::GetPin(IBaseFilter* filter, PIN_DIRECTION direction, int index)
{
    IPin* pin = NULL;

    CComPtr< IEnumPins > enum_pins;
    HRESULT hr = filter->EnumPins( &enum_pins );
    if(FAILED( hr )) 
    {
        return pin;
    }

    while (true)
    {
        ULONG ulFound;
        IPin *tmp_pin;
        hr = enum_pins->Next(1, &tmp_pin, &ulFound);
        if (hr != S_OK)
        {
            break;
        }

        PIN_DIRECTION pindir = (PIN_DIRECTION)3;
        tmp_pin->QueryDirection(&pindir);
        if (pindir == direction)
        {
            if (index == 0)
            {
                tmp_pin->AddRef();
                pin = tmp_pin;
                SAFE_RELEASE(tmp_pin);
                break;
            }
            index--;
        }

        SAFE_RELEASE(tmp_pin);
    }

    return pin;
}
