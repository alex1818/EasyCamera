#include "General.h"
#include "DSAudioGraph.h"
#include "baseclasses/mtype.h"

DSAudioGraph::DSAudioGraph()
    : audio_cap_device_(NULL)
{

}

DSAudioGraph::~DSAudioGraph()
{
    Destroy();
}

int DSAudioGraph::Create(DSAudioCaptureDevice* audioCapDevice)
{
    audio_cap_device_ = audioCapDevice;

    grabber_.CoCreateInstance( CLSID_SampleGrabber );
    if( !grabber_ )
    {
        return -1;
    }
    CComQIPtr<IBaseFilter, &IID_IBaseFilter> grabber_filter(grabber_);
    graph_builder_->AddFilter(grabber_filter, _T("Grabber"));

    AM_MEDIA_TYPE mt;
    ZeroMemory(&mt, sizeof(AM_MEDIA_TYPE));
    mt.majortype = MEDIATYPE_Audio;
    mt.subtype = MEDIASUBTYPE_PCM;
    grabber_->SetMediaType(&mt);

    // source filter

    source_filter_ = audio_cap_device_->GetBaseFilter();

    CComQIPtr<IBaseFilter, &IID_IBaseFilter> cap_filter(source_filter_);
    graph_builder_->AddFilter(cap_filter, _T("Source"));
    audio_cap_device_->SetOutPin(DSGraph::GetOutPin(cap_filter, 0 ));
    cap_filter.Release();

    HRESULT hr = SetAudioFormat( 
        audio_cap_device_->GetPreferredSamplesPerSec(),
        audio_cap_device_->GetPreferredBitsPerSample(), 
        audio_cap_device_->GetPreferredChannels() 
        );
    if(FAILED(hr))
    {
        grabber_filter.Release();
        grabber_.Release();
        return -2;
    }

    // connect the capture device out pin to the grabber

    CComPtr<IPin> grabber_pin = DSGraph::GetInPin(grabber_filter, 0);
    grabber_filter.Release();

    hr = graph_builder_->Connect(audio_cap_device_->GetOutPin(), grabber_pin);
    if(FAILED(hr))
    {
        grabber_pin.Release();
        grabber_.Release();
        return -3;
    }
    grabber_pin.Release();

    // buffering samples as they pass through
    grabber_->SetBufferSamples(TRUE);

    // not grabbing just one frame
    grabber_->SetOneShot(FALSE);

    // setting callback
    grabber_->SetCallback((ISampleGrabberCB*)(&grabber_callback_), 1);

    return 0;
}

void DSAudioGraph::Destroy()
{
    grabber_.Release();
}

HRESULT DSAudioGraph::SetAudioFormat(DWORD samsPerSec, WORD bitsPerSam, WORD channels)
{
    IAMBufferNegotiation *pNeg = NULL;
    WORD wBytesPerSample = bitsPerSam/8;
    DWORD dwBytesPerSecond = wBytesPerSample * samsPerSec * channels;
    //DWORD dwBufferSize = (DWORD)(0.5*dwBytesPerSecond);
    DWORD dwBufferSize;

    // setting buffer size according to the aac frame size
//     // (in narrow-band: 160*2 bytes)
//     switch( samsPerSec )
//     {
//     case 8000: { dwBufferSize = 320; break; }
//     case 11025: { dwBufferSize = 1280; break; } // AUDIO STREAM LAG DEPENDS ON THIS
//     case 22050: { dwBufferSize = 1280; break; }
//     case 44100: { dwBufferSize = 4096; break; }
//     default: dwBufferSize = 320;
//     }

    // dwBufferSize = aac_frame_len * channels * wBytesPerSample
    //              = 1024 * channels * wByesPerSample
    dwBufferSize = 1024 * channels * wBytesPerSample;
    //dwBufferSize = 2048 * channels;

    // get the nearest, or exact audio format the user wants
    //
    IEnumMediaTypes *pMedia = NULL;
    AM_MEDIA_TYPE *pmt = NULL;

    HRESULT hr = audio_cap_device_->GetOutPin()->EnumMediaTypes( &pMedia );

    if( SUCCEEDED(hr) )
    {
        while( pMedia->Next( 1, &pmt, 0 ) == S_OK )
        {
            if ( ( pmt->formattype == FORMAT_WaveFormatEx) && 
                ( pmt->cbFormat == sizeof(WAVEFORMATEX) ) )
            {
                WAVEFORMATEX *wf = (WAVEFORMATEX *)pmt->pbFormat;

                if( ( wf->nSamplesPerSec == samsPerSec ) &&
                    ( wf->wBitsPerSample == bitsPerSam ) &&
                    ( wf->nChannels == channels ) )
                {
                    // found correct audio format
                    //
                    CComPtr<IAMStreamConfig> pConfig;
                    hr = audio_cap_device_->GetOutPin()->QueryInterface( IID_IAMStreamConfig, (void **) &pConfig );
                    if( SUCCEEDED( hr ) )
                    {
                        // get buffer negotiation interface
                        audio_cap_device_->GetOutPin()->QueryInterface(IID_IAMBufferNegotiation, (void **)&pNeg);

                        // set the buffer size based on selected settings
                        ALLOCATOR_PROPERTIES prop={0};
                        prop.cbBuffer = dwBufferSize;
                        prop.cBuffers = 6; // AUDIO STREAM LAG DEPENDS ON THIS
                        prop.cbAlign = wBytesPerSample * channels;
                        pNeg->SuggestAllocatorProperties(&prop);
                        SAFE_RELEASE( pNeg );

                        WAVEFORMATEX *wf = (WAVEFORMATEX *)pmt->pbFormat;

                        // setting additional audio parameters
                        wf->nAvgBytesPerSec = dwBytesPerSecond;
                        wf->nBlockAlign = wBytesPerSample * channels;
                        wf->nChannels = channels;
                        wf->nSamplesPerSec = samsPerSec;
                        wf->wBitsPerSample = bitsPerSam;

                        pConfig->SetFormat( pmt );

                    }
                    else
                    {
                        pConfig.Release();
                        SAFE_RELEASE( pMedia );
                        DeleteMediaType( pmt );
                        // can't set given audio format!
                        return -1;
                    }

                    DeleteMediaType( pmt );

                    hr = pConfig->GetFormat( &pmt );
                    if( SUCCEEDED( hr ) )
                    {
                        WAVEFORMATEX *wf = (WAVEFORMATEX *)pmt->pbFormat;

                        audio_cap_device_->SetSamplesPerSec( wf->nSamplesPerSec );
                        audio_cap_device_->SetBitsPerSample( wf->wBitsPerSample );
                        audio_cap_device_->SetChannels( wf->nChannels );

                        // audio is now initialized
                        DeleteMediaType( pmt );
                        pConfig.Release();
                        SAFE_RELEASE( pMedia );
                        return 0;
                    }

                    // error initializing audio
                    DeleteMediaType( pmt );
                    pConfig.Release();
                    SAFE_RELEASE( pMedia );
                    return -1;
                }
            }
            DeleteMediaType( pmt );
        }
        SAFE_RELEASE( pMedia );
    }

    {

    }

    return -2;
}
