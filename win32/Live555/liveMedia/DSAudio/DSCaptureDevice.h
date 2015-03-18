#ifndef _DS_CAPTURE_DEVICE_H_
#define _DS_CAPTURE_DEVICE_H_

class DSCaptureDevice
{
public:
    DSCaptureDevice();

    virtual ~DSCaptureDevice();

    HRESULT Create(const CString& comObjID);

    // -----------------------------------------------------------------------
    // seto¨ªget¨ºy?Y3¨¦?¡À

    void SetDeviceID(const CString& deviceID) { com_obj_id_ = deviceID; }
    const CString& GetID() { return com_obj_id_; }

    void SetDeviceName(const CString& deviceName) { device_name_ = deviceName; }
    const CString& GetDeviceName() { return device_name_; }

    void SetBaseFilter(const CComPtr<IBaseFilter>& baseFilter) { base_filter_ = baseFilter; }
    CComPtr<IBaseFilter>& GetBaseFilter() { return base_filter_; }

    void SetOutPin(const CComPtr<IPin>& outPin) { out_pin_ = outPin; }
    CComPtr<IPin>& GetOutPin() { return out_pin_; }

private:
    CString com_obj_id_;
    CString device_name_;

    CComPtr<IPin> out_pin_;
    CComPtr<IBaseFilter> base_filter_;
};

#endif // _DS_CAPTURE_DEVICE_H_
