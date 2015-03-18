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
// Copyright (c) 1996-2012 Live Networks, Inc.  All rights reserved.
// Generic audio input device (such as a microphone, or an input sound card)
// C++ header

#ifndef _AUDIO_INPUT_MIC_DEVICE_HH
#define _AUDIO_INPUT_MIC_DEVICE_HH

#include "../DSAudio/General.h"
#include<iostream>
#include<cstring>
#include<windows.h>
#include<conio.h>
#include "../DSAudio/dscapture.h"

#ifndef _FRAMED_SOURCE_HH
#include "FramedSource.hh"
#endif

class FAACEncoder;
class AudioInputMicDevice: public FramedSource {
public:
  unsigned char bitsPerSample() const { return fBitsPerSample; }
  unsigned char numChannels() const { return fNumChannels; }
  unsigned samplingFrequency() const { return fSamplingFrequency; }

  static AudioInputMicDevice* createNew(UsageEnvironment& env,
	    unsigned char bitsPerSample, unsigned char numChannels,
	    unsigned samplingFrequency, unsigned granularityInMS = 20);

  private:
  // redefined virtual functions:
  virtual void doGetNextFrame();
  virtual void doStopGettingFrames();
  virtual Boolean setInputPort(int portIndex);
  virtual double getAverageLevel() const;


  static char** allowedDeviceNames;
  // If this is set to non-NULL, then it's a NULL-terminated array of strings
  // of device names that we are allowed to access.

public:
  AudioInputMicDevice(UsageEnvironment& env,
		   unsigned char bitsPerSample,
		   unsigned char numChannels,
		   unsigned samplingFrequency,
		   unsigned granularityInMS);
	// we're an abstract base class

  virtual ~AudioInputMicDevice();

protected:
  unsigned char fBitsPerSample, fNumChannels;
  unsigned fSamplingFrequency;
  unsigned fGranularityInMS;

  DSCapture* ds_capture_;
  FAACEncoder* faac_encoder_;

  unsigned long max_out_bytes;
  unsigned char* outbuf;
  
  //FILE* _file;
};

#endif
