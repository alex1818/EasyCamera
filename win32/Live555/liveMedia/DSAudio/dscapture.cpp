#include "General.h"
#include "DSCapture.h"

#include <map>

#include "DSAudioGraph.h"
#include "DSAudioCaptureDevice.h"
//#include "AudioEncoderThread.h"

DSCapture::DSCapture()
{
    // initializing a directshow filtergraph
    //
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    ds_graph_ = new DSGraph;

    ds_audio_graph_ = NULL;
    ds_audio_cap_device_ = NULL;
    //audio_encoder_thread_ = NULL;
}

DSCapture::~DSCapture()
{
    Destroy();
    SAFE_DELETE( ds_graph_ );
    CoUninitialize();
}

void DSCapture::Destroy()
{
    SAFE_DELETE( ds_audio_graph_ );
    SAFE_DELETE( ds_audio_cap_device_ );
    //SAFE_DELETE( audio_encoder_thread_ );
}

void DSCapture::Create(const CString& audioDeviceID,
    const DSAudioFormat& audioFormat, const CString& audioOutname)
{
    HRESULT hr;

    // audio

    ds_audio_graph_ = new DSAudioGraph;
    ds_audio_cap_device_ = new DSAudioCaptureDevice;
    //audio_encoder_thread_ = new AudioEncoderThread(ds_audio_graph_);

    hr = ds_audio_cap_device_->Create(audioDeviceID);
    if( FAILED( hr ) )
    {
        SAFE_DELETE( ds_audio_cap_device_ );
        SAFE_DELETE( ds_audio_graph_ );
    }
    if( ds_audio_cap_device_ )
    {
        ds_audio_cap_device_->SetPreferredSamplesPerSec(audioFormat.samples_per_sec);
        ds_audio_cap_device_->SetPreferredBitsPerSample(audioFormat.bits_per_sample);
        ds_audio_cap_device_->SetPreferredChannels(audioFormat.channels);

        HRESULT hr = ds_audio_graph_->Create( ds_audio_cap_device_ );
        if( FAILED( hr ) )
        {
            
        }
        //else
        //{
        //    audio_encoder_thread_->SetOutputFilename(CStringToString(audioOutname));
        //}
    }
}

void DSCapture::Create(const CString& audioDeviceID, const CString& videoDeviceID)
{
    HRESULT hr;

    // audio

    ds_audio_graph_ = new DSAudioGraph;
    ds_audio_cap_device_ = new DSAudioCaptureDevice;

    hr = ds_audio_cap_device_->Create(audioDeviceID);
    if( FAILED( hr ) )
    {
        SAFE_DELETE( ds_audio_cap_device_ );
        SAFE_DELETE( ds_audio_graph_ );
    }
    if( ds_audio_cap_device_ )
    {
        HRESULT hr = ds_audio_graph_->Create( ds_audio_cap_device_ );
        if( FAILED( hr ) )
        {

        }
    }
}

void DSCapture::StartAudio()
{
    //audio_encoder_thread_->Start();
    ds_audio_graph_->Start();
}

void DSCapture::StopAudio()
{
    ds_audio_graph_->Stop();
    //audio_encoder_thread_->Stop();
    //audio_encoder_thread_->Join();
}

char* DSCapture::GetBuffer(int& bufLen)
{
	return ds_audio_graph_->GetBuffer(bufLen);
}

void DSCapture::ReleaseBuffer(char* buf)
{
	return ds_audio_graph_->ReleaseBuffer(buf);
}

HRESULT DSCapture::SetAudioFormat( DWORD dwPreferredSamplesPerSec, WORD wPreferredBitsPerSample, WORD nPreferredChannels )
{
    StopAudio();

    ds_audio_graph_->Destroy();

    ds_audio_cap_device_->SetPreferredSamplesPerSec( dwPreferredSamplesPerSec );
    ds_audio_cap_device_->SetPreferredBitsPerSample( wPreferredBitsPerSample );
    ds_audio_cap_device_->SetPreferredChannels( nPreferredChannels );

    HRESULT hr = ds_audio_graph_->Create( ds_audio_cap_device_ );
    if( FAILED( hr ) )
    {
        return hr;
    }

    StartAudio();

    return 0;
}

std::string DSCapture::CStringToString(const CString& mfcStr)
{
    CT2CA pszConvertedAnsiString(mfcStr);
    return (pszConvertedAnsiString);
}
