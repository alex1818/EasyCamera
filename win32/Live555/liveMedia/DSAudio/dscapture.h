#ifndef _DS_CAPTURE_H_
#define _DS_CAPTURE_H_

#include "DSGraph.h"

class DSAudioGraph;
class DSAudioCaptureDevice;
//class AudioEncoderThread;

struct DSAudioFormat
{
    unsigned int samples_per_sec;
    unsigned int bits_per_sample;
    unsigned int channels;
};

class DSCapture
{
public:
    DSCapture();

    ~DSCapture();

    void Create(const CString& audioDeviceID, const DSAudioFormat& audioFormat, const CString& audioOutname);

    void Create(const CString& audioDeviceID, const CString& videoDeviceID);

    void Destroy();


    HRESULT	SetAudioFormat(DWORD dwPreferredSamplesPerSec,
        WORD wPreferredBitsPerSample, WORD nPreferredChannels);

    void StartAudio();

    void StopAudio();

	char* GetBuffer(int& bufLen);

	void ReleaseBuffer(char* buf);
	

    DSAudioGraph* GetAudioGraph() { return ds_audio_graph_; }

    DSGraph * DShowGraph() { return ds_graph_; }

private:
    std::string CStringToString(const CString& mfcStr);

private:
    DSGraph*		ds_graph_;

    DSAudioCaptureDevice*	ds_audio_cap_device_;

    DSAudioGraph*			ds_audio_graph_;

    //AudioEncoderThread* audio_encoder_thread_;
};

#endif // _DS_CAPTURE_H_
