/*
	Copyright (c) 2013-2014 EasyDarwin.ORG.  All rights reserved.
	Github: https://github.com/EasyDarwin
	WEChat: EasyDarwin
	Website: http://www.easydarwin.org
*/
#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"
#include "RTSPCommon.hh"
#include "Genera.h"

char timeStr[60] = {0};		//print time format
UsageEnvironment* env;		//live555 global environment
FramedSource* audioSource;	//live555 audio source
FramedSource* videoSource;	//live555 video source
RTPSink* audioSink;			//live555 audio sink
RTPSink* videoSink;			//live555 video sink
DarwinInjector* injector;	//Darwin Injector For EasyDarwin
Boolean isPlaying = false;	//playing flag
char loopMutex = ~0;		//for live555 msg waiting,http://blog.csdn.net/xiejiashu/article/details/8463752
char eventLoopWatchVariable = 0;
char fIp[MAX_PARAMETER_LEN] = {0};
unsigned int fPort = 0;
char fId[MAX_PARAMETER_LEN] = {0};

bool RedirectStream(char const* ip, unsigned port, char const* id);	// forward,strat streaming
char* printTime();	// format time
bool play();		// forward source->sink
void pause(void* clientData);// forward pause streaming
void stop();
void EasyIpCameraRun(char const* ip, unsigned int port, char const* id);

int main(int argc, char** argv)
{
	EasyIpCameraRun("127.0.0.1",554,"live");
	return 0;
}

void EasyIpCameraRun(char const* ip, unsigned int port, char const* id)
{
	TaskScheduler* scheduler = BasicTaskScheduler::createNew();
	env = BasicUsageEnvironment::createNew(*scheduler);
	isPlaying = RedirectStream(ip, port, id);
	eventLoopWatchVariable = 0;
	env->taskScheduler().doEventLoop1(&eventLoopWatchVariable);
}

bool RedirectStream(char const* ip, unsigned port, char const* id)
{
	// Create a 'Darwin injector' object:
	injector = DarwinInjector::createNew(*env);

	////////// AUDIO //////////
	struct in_addr dummyDestAddress;
	dummyDestAddress.s_addr = 0;
	printf("in RedirectStream\n");
	Groupsock rtpGroupsockAudio(*env, dummyDestAddress, 0, 0);
	Groupsock rtcpGroupsockAudio(*env, dummyDestAddress, 0, 0);

	// AudioSink
	audioSink = MPEG4GenericRTPSink::createNew(*env, &rtpGroupsockAudio,
										97,
										8000,
										"audio", "AAC-hbr", "",
										2);
	const unsigned estimatedSessionBandwidthAudio = 160; // in kbps; for RTCP b/w share
	const unsigned maxCNAMElen = 100;
	unsigned char CNAME[maxCNAMElen+1];
	gethostname((char*)CNAME, maxCNAMElen);
	CNAME[maxCNAMElen] = '\0'; // just in case
	RTCPInstance* audioRTCP = NULL;
	//RTCPInstance::createNew(*env, &rtcpGroupsockAudio,
			 //   estimatedSessionBandwidthAudio, CNAME,
			 //   audioSink, NULL /* we're a server */);
	injector->addStream(audioSink, audioRTCP);
	////////// END AUDIO //////////

	////////// VIDEO //////////
	Groupsock rtpGroupsockVideo(*env, dummyDestAddress, 0, 0);
	Groupsock rtcpGroupsockVideo(*env, dummyDestAddress, 0, 0);
	printf("H264VideoRTPSink::createNew\n");
	videoSink = H264VideoRTPSink::createNew(*env,&rtpGroupsockVideo,96);
	const unsigned estimatedSessionBandwidthVideo = 4500; // in kbps; for RTCP b/w share
	RTCPInstance* videoRTCP = NULL;
	//RTCPInstance::createNew(*env, &rtcpGroupsockVideo,
				//     estimatedSessionBandwidthVideo, CNAME,
				//     videoSink, NULL /* we're a server */);
	injector->addStream(videoSink, videoRTCP);
	////////// END VIDEO //////////

	char remoteFilename[MAX_PARAMETER_LEN];
	sprintf(remoteFilename,"%s.%s", id, "sdp"); 
	// Next, specify the destination Darwin Streaming Server:
	if (!injector->setDestination(ip, remoteFilename,
				"liveUSBCamera", "LIVE555 Streaming Media", port)) {
		*env << printTime() << "injector->setDestination() failed: " << env->getResultMsg() << "\n";
		return false;
	}
	*env << printTime() << "LIVE555 Streaming Media" << "\n";

	if(play())
	{
		return true;
	}
	else
	{
		*env << printTime() << "streaming to server error\n";

		audioSink->stopPlaying();
		Medium::close(audioSource);

		videoSink->stopPlaying();
		Medium::close(videoSource);

		Medium::close(*env, injector->name());
		injector == NULL;
		return false;
	}
}


void afterPlaying(void* clientData) {
	if( audioSource != NULL )
	{
		if (audioSource->isCurrentlyAwaitingData()) return;
	}
	
	if( videoSource != NULL )
	{
		if (videoSource->isCurrentlyAwaitingData()) return;
	}

	if( audioSource != NULL )
	{
		audioSink->stopPlaying();
		Medium::close(audioSource);
		audioSource = NULL;
	}

	if( videoSource != NULL )
	{
		videoSink->stopPlaying();
		Medium::close(videoSource);
		videoSource = NULL;
	}

	if(!play())
		isPlaying = false;
}

bool play() 
{
	FramedSource* audioES = AudioInputMicDevice::createNew(*env,16,2,8000);
	audioSource = audioES;

	FramedSource* videoES = CamH264VideoStreamFramer::createNew(*env, NULL);
	videoSource = videoES;

	if(videoSource == NULL && audioSource == NULL)
	{
		*env << printTime() << env->getResultMsg() << "\n";
		return false;
	}

	// Finally, start playing each sink.
	if(videoSource != NULL)
	{
		*env << printTime() << "Beginning to get camera video...\n";
		videoSink->startPlaying(*videoSource, afterPlaying, videoSink);
		videoSink->SetOnDarwinClosure(pause, (void*)videoSink);
	}

	if(audioSource != NULL)
	{
		audioSink->startPlaying(*audioSource, afterPlaying, audioSink);
		audioSink->SetOnDarwinClosure(pause, (void*)audioSink);
	}
	return true;
}

void pause(void* clientData)
{
	*env << printTime() << "restreaming to server\n";

	if(audioSource != NULL)
	{
		audioSink->stopPlaying();
		Medium::close(audioSource);
	}

	if(videoSource != NULL)
	{
		videoSink->stopPlaying();
		Medium::close(videoSource);
	}
	Medium::close(*env, injector->name());
	injector == NULL;
	isPlaying = false;

	EasyIpCameraRun(fIp,fPort,fId);	
}

void stop()
{
	*env << printTime() << "stop streaming to server\n";

	if(audioSource != NULL)
	{
		audioSink->stopPlaying();
		Medium::close(audioSource);
	}

	if(videoSource != NULL)
	{
		videoSink->stopPlaying();
		Medium::close(videoSource);
	}
	Medium::close(*env, injector->name());
	injector == NULL;
	isPlaying = false;
}

char* printTime()
{
	SYSTEMTIME sys; 
	GetLocalTime( &sys ); 
	sprintf(timeStr,"%4d/%02d/%02d-%02d:%02d:%02d %03d: \0",sys.wYear,sys.wMonth,sys.wDay,sys.wHour,sys.wMinute,sys.wSecond,sys.wMilliseconds); 
	return timeStr;
}

