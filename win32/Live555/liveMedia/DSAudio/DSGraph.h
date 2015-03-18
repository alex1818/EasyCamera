#ifndef _DS_GRAPH_H_
#define _DS_GRAPH_H_

#include <map>

#include "DSGrabberCallback.h"

class DSGraph
{
public:
    DSGraph();

    virtual ~DSGraph();

    void Start();

    void Stop();

    bool IsBufferAvailable() { return grabber_callback_.IsBufferAvailable(); }

    char* GetBuffer(int& bufLen) { return grabber_callback_.GetBuffer(bufLen); }

    void ReleaseBuffer(char* buf) { grabber_callback_.ReleaseBuffer(buf); }

    int BufferSize() { return grabber_callback_.BufferSize(); }

    __int64 Timestamp() { return grabber_callback_.Timestamp(); }

    UINT FPSMeter() { return grabber_callback_.FPSMeter(); }

    void ResetFPSMeter() { return grabber_callback_.ResetFPSMeter(); }

    std::map<CString, CString>& AudioCapDevices() { return audio_cap_devices_; }

    static IPin* GetPin(IBaseFilter* filter, PIN_DIRECTION direction, int index);

    static IPin* GetInPin(IBaseFilter* filter, int pinIndex);

    static IPin* GetOutPin(IBaseFilter* filter, int pinIndex);

private:
    void ListCapDevices(const IID& deviceIID, std::map<CString, CString>& deviceList);

protected:
    std::map<CString, CString> audio_cap_devices_;

    CComPtr<IBaseFilter> source_filter_;
    CComPtr<IGraphBuilder> graph_builder_;
    CComPtr<ISampleGrabber> grabber_;

    DSGrabberCallback grabber_callback_;
};

#endif // _DS_GRAPH_H_
