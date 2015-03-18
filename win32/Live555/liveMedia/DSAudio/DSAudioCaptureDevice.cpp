#include "General.h"
#include "DSAudioCaptureDevice.h"

DSAudioCaptureDevice::DSAudioCaptureDevice()
{
    sams_per_sec_ = 0;
    bits_per_sam_ = 0;
    channels_ = 0;

    preferred_sams_per_sec_ = 0;
    preferred_bits_per_sam_ = 0;
    preferred_channels_ = 0;
}

DSAudioCaptureDevice::~DSAudioCaptureDevice()
{

}

HRESULT DSAudioCaptureDevice::Create(const CString& comObjID)
{
    return DSCaptureDevice::Create(comObjID);
}
