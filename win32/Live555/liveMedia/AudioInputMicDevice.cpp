/**********
This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 2.1 of the License, or (at your
option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
**********/
// Copyright (c) 2001-2003 Live Networks, Inc.  All rights reserved.
// Generic audio input device (such as a microphone, or an input sound card)
// Implementation

#include <AudioInputMicDevice.hh>

#include "DSAudio/FAACEncoder.h"

#include "GroupsockHelper.hh"


AudioInputMicDevice* AudioInputMicDevice::createNew(UsageEnvironment& env,
	    unsigned char bitsPerSample, unsigned char numChannels,
	    unsigned samplingFrequency, unsigned granularityInMS)
{
    AudioInputMicDevice* fr = new AudioInputMicDevice(env, bitsPerSample, numChannels, samplingFrequency,granularityInMS);
    return fr;
}



AudioInputMicDevice
::AudioInputMicDevice(UsageEnvironment& env, unsigned char bitsPerSample,
		   unsigned char numChannels,
		   unsigned samplingFrequency, unsigned granularityInMS)
  : FramedSource(env), fBitsPerSample(bitsPerSample),
    fNumChannels(numChannels), fSamplingFrequency(samplingFrequency),
    fGranularityInMS(granularityInMS), faac_encoder_(new FAACEncoder) {

	// 3?¨º??¡¥1¡è¡Á¡Â
    faac_encoder_->Init(samplingFrequency,numChannels,fBitsPerSample);

	ds_capture_ = new DSCapture();

	std::map<CString, CString> a_devices = ds_capture_->DShowGraph()->AudioCapDevices();

	DSAudioFormat audio_fmt;
    audio_fmt.samples_per_sec = fSamplingFrequency;
    audio_fmt.channels = fNumChannels;
    audio_fmt.bits_per_sample = fBitsPerSample;

	CString audio_device_id;
	for (std::map<CString, CString>::iterator it = a_devices.begin(); it != a_devices.end(); ++it)
    {
		audio_device_id = it->first;
    }

    ds_capture_->Create(audio_device_id, audio_fmt, NULL);
   
    ds_capture_->StartAudio();

	max_out_bytes = faac_encoder_->MaxOutBytes();
    outbuf = (unsigned char*)malloc(max_out_bytes);

	//_file = ::fopen("c:\\test.acc","wb");
}


void AudioInputMicDevice::doGetNextFrame()
{
	int len = 0;
	char* buf = ds_capture_->GetBuffer(len);
	if(len>0)
	{
		unsigned int sample_count = (len << 3)/fBitsPerSample;
		unsigned int buf_size;
		faac_encoder_->Encode((unsigned char*)buf, sample_count,
		(unsigned char*)outbuf, buf_size);
		//::fwrite(outbuf,1,buf_size,_file);
	    
		if(buf_size < fMaxSize)        
		{            
		  memcpy(fTo, outbuf, buf_size);
		  fFrameSize = buf_size;
		}        
		else        
		{           
		  memcpy(fTo, outbuf, fMaxSize);            
		  fNumTruncatedBytes = buf_size - fMaxSize;   
		  fFrameSize = fMaxSize;
		}

		//ds_capture_->ReleaseBuffer(buf);
	
	}
	else
		fFrameSize = 0; 

    //fDurationInMicroseconds = 20000;    
    //afterGetting(this);  

	  // Set the 'presentation time':
  if (fPresentationTime.tv_sec == 0 && fPresentationTime.tv_usec == 0) {
    // This is the first frame, so use the current time:
    gettimeofday(&fPresentationTime, NULL);
  } else {
    // Increment by the play time of the previous frame (20 ms)
    unsigned uSeconds	= fPresentationTime.tv_usec + 130000;
    fPresentationTime.tv_sec += uSeconds/1000000;
    fPresentationTime.tv_usec = uSeconds%1000000;
  }

  fDurationInMicroseconds = 130000; // each frame is 20 ms

  // Switch to another task, and inform the reader that he has data:
  nextTask() = envir().taskScheduler().scheduleDelayedTask(0,
				(TaskFunc*)FramedSource::afterGetting, this);


}

void AudioInputMicDevice::doStopGettingFrames() {
}


Boolean AudioInputMicDevice::setInputPort(int /*portIndex*/) {
  return True;
}

double AudioInputMicDevice::getAverageLevel() const {
  return 0.0;//##### fix this later
}


AudioInputMicDevice::~AudioInputMicDevice() {
	delete faac_encoder_;
}

char** AudioInputMicDevice::allowedDeviceNames = NULL;
