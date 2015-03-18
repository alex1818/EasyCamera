#ifndef _H264DECWRAPPER_H_
#define _H264DECWRAPPER_H_

#include "DllManager.h"

struct AVCodec;
struct AVCodecContext;
struct AVFrame;
struct H264Context;
struct MpegEncContext;
struct DSPContext;

class DLL_EXPORT H264DecWrapper
{
public:
    H264DecWrapper();
    virtual ~H264DecWrapper();

    int Initialize();

    int Decode(unsigned char* szNal, int iSize, unsigned char* szOutImage, int& iOutSize, bool& bGetFrame);

    int Destroy();
    
private:
    
    AVCodec *codec;
    AVCodecContext *c;
    int frame, size, got_picture, len;
    AVFrame *picture;
    DSPContext* dsp;
    H264Context *h;
    MpegEncContext *s;
};

#endif

