
#include "H264DecWrapper.h"

extern "C" 
{
#include "avcodec.h"
#include "define.h"
#include "dsputil.h"
#include "h264.h"
#include "string.h"

extern AVCodec h264_decoder;
}

H264DecWrapper::H264DecWrapper()
{
    codec = NULL;
    c = NULL;
    picture = NULL;
    dsp = NULL;
    h = NULL;
    s = NULL;
    frame = size = got_picture = len = 0;
}

H264DecWrapper::~H264DecWrapper()
{
}

int H264DecWrapper::Initialize()
{
    codec = &h264_decoder;
    avcodec_init();


    c = avcodec_alloc_context();
    picture = avcodec_alloc_frame();
    if(codec->capabilities&CODEC_CAP_TRUNCATED)  
    {
        c->flags |= CODEC_FLAG_TRUNCATED; 
    }

    if (avcodec_open(c, codec) < 0) {
        fprintf(stderr, "could not open codec\n");
        return -1;
    }

    h = (H264Context*)c->priv_data;
    s = &h->s;
    s->dsp.idct_permutation_type = 1;
    dsputil_init(&s->dsp, c);    

    return 0;
}

int H264DecWrapper::Destroy()
{
    avcodec_close(c);
    av_free(c);
    av_free(picture);
    return 0;
}

unsigned output(unsigned char *buf,int wrap, int xsize,int ysize, unsigned char* outbuf)
{
    int i;
    for(i=0;i<ysize;i++)
    {
        memcpy(outbuf + i*xsize, buf + i * wrap, xsize);
    }
    return xsize*ysize;
}

//返回在szNal中已经处理的字节数
int H264DecWrapper::Decode(unsigned char* szNal, int iSize, 
    unsigned char* szOutImage, int& iOutSize, bool& bGetFrame)
{
    int got_picture_ptr = 0;
    int len = avcodec_decode_video(c, picture, &got_picture_ptr, szNal, iSize);
    if (len < 0) 
    {
        fprintf(stderr, "Error while decoding frame %d\n", frame);
        return -1;
    }
    bGetFrame = (got_picture_ptr == 0 ? false : true);
    
    if (bGetFrame) 
    {
        iOutSize = 0;
        iOutSize += output(picture->data[0], picture->linesize[0],c->width, c->height, szOutImage+iOutSize);
        iOutSize += output(picture->data[1], picture->linesize[1],c->width/2, c->height/2, szOutImage+iOutSize);
        iOutSize += output(picture->data[2], picture->linesize[2],c->width/2, c->height/2, szOutImage+iOutSize);
    }
    else
    {
        bGetFrame = false;
    }

    return len;
}


