extern "C" 
{
#include "log.h"
}

#include "H264EndWrapper.h"

#include <stdio.h>
#include <string.h> // strerror() 
#include <stdlib.h>




void *my_malloc( int i_size )
{
#ifdef SYS_MACOSX
    /* Mac OS X always returns 16 bytes aligned memory */
    return malloc( i_size );
#elif defined( HAVE_MALLOC_H )
    return memalign( 16, i_size );
#else
    uint8_t * buf;
    uint8_t * align_buf;
    buf = (uint8_t *) malloc( i_size + 15 + sizeof( void ** ) +
              sizeof( int ) );
    align_buf = buf + 15 + sizeof( void ** ) + sizeof( int );
    align_buf -= (intptr_t) align_buf & 15;
    *( (void **) ( align_buf - sizeof( void ** ) ) ) = buf;
    *( (int *) ( align_buf - sizeof( void ** ) - sizeof( int ) ) ) = i_size;
    return align_buf;
#endif
}

void my_free( void *p )
{
    if( p )
    {
#if defined( HAVE_MALLOC_H ) || defined( SYS_MACOSX )
        free( p );
#else
        free( *( ( ( void **) p ) - 1 ) );
#endif
    p = NULL;
    }
}

H264EncWrapper::H264EncWrapper()
{
    m_h = NULL;
    m_pBuffer = NULL;
    m_iBufferSize = 0;
    m_iFrameNum = 0;
    x264_param_default(&m_param);
}

H264EncWrapper::~H264EncWrapper()
{
}

int H264EncWrapper::Initialize(int iWidth, int iHeight, int iRateBit, int iFps)
{
   x264_param_default_preset(&m_param, "veryfast", "zerolatency");

	m_param.i_width = iWidth;
	m_param.i_height = iHeight;
	m_param.i_fps_num = 25;
	m_param.i_fps_den = 1;
	m_param.i_keyint_max = 25;
	//m_param.b_intra_refresh = 1;
	m_param.b_annexb = 1;
	m_param.rc.i_bitrate = 96;
	x264_param_apply_profile(&m_param, "main");
	m_h = x264_encoder_open(&m_param);
	x264_picture_alloc(&m_pic, X264_CSP_I420, iWidth, iHeight);
    return 0;
}

int H264EncWrapper::Destroy()
{
    x264_picture_clean( &m_pic );

    // TODO: clean m_h
    //x264_encoder_close  (m_h); //?????
    
   return 0;
}

//FILE* ff1 ;
int H264EncWrapper::Encode(unsigned char* szYUVFrame, TNAL*& pNALArray, int& iNalNum)
{
    // 可以优化为m_pic中保存一个指针,直接执行szYUVFrame
   // memcpy(m_pic.img.plane[0], szYUVFrame, m_param.i_width * m_param.i_height*3 / 2);
	int frames = 0;
    int resolution = m_param.i_width * m_param.i_height;
	memcpy(m_pic.img.plane[0], szYUVFrame,resolution ); 
    memcpy(m_pic.img.plane[1], szYUVFrame + resolution, resolution >> 2); 
	memcpy(m_pic.img.plane[2], szYUVFrame + resolution + (resolution >> 2), resolution >> 2); 

   // m_pic.i_pts = (int64_t)m_iFrameNum * m_param.i_fps_den;

    x264_picture_t pic_out;
    x264_nal_t *nal;
    int i_nal, i; // nal的个数
	int frame_size = 0;
	frames++;
	

    if((frame_size = x264_encoder_encode( m_h, &nal, &i_nal, &m_pic, &pic_out )) < 0 )
    {
        fprintf( stderr, "x264 [error]: x264_encoder_encode failed\n" );
        return -1;
    }

	
    int i_size = 0;
    //pNALArray = new TNAL[i_nal];
    //memset(pNALArray, 0, i_nal+1);
    //
    //for( i = 0; i < i_nal; i++ )
    //{
    //    if( m_iBufferSize < nal[i].i_payload * 3/2 + 4 )
    //    {
    //        m_iBufferSize = nal[i].i_payload * 2 + 4;
    //        my_free( m_pBuffer );
    //        m_pBuffer = (uint8_t*)my_malloc( m_iBufferSize );
    //    }

    //    i_size = m_iBufferSize;
    //    x264_nal_encode( m_pBuffer, &i_size, &nal[i] );
    //    //DEBUG_LOG(INF, "Encode frame[%d], NAL[%d],  length = %d, ref_idc = %d, type = %d", 
    //    //    m_iFrameNum, i, i_size, nal[i].i_ref_idc, nal[i].i_type);
    //    //printf("Encode frame[%d], NAL[%d],  length = %d, ref_idc = %d, type = %d\n", 
    //    //    m_iFrameNum, i, i_size, nal[i].i_ref_idc, nal[i].i_type);
    //    
    //    //fwrite(m_pBuffer, 1, i_size, ff1);
    //    
    //    //去掉buffer中前面的 00 00 00 01 才是真正的nal unit
    //    pNALArray[i].size = i_size;
    //    pNALArray[i].data = new unsigned char[i_size];
    //    memcpy(pNALArray[i].data, m_pBuffer, i_size);
    //    
    //}
	pNALArray = new TNAL[i_nal];
    memset(pNALArray, 0, i_nal+1);
	for( i = 0; i < i_nal; i++ )
	{
		i_size = nal[i].i_payload;
		pNALArray[i].size = i_size;
        pNALArray[i].data = new unsigned char[i_size];
		memcpy(pNALArray[i].data, nal[i].p_payload, i_size);
	}
    iNalNum = i_nal;    
    m_iFrameNum++;
    return frame_size;
}

void H264EncWrapper::CleanNAL(TNAL* pNALArray, int iNalNum)
{
    for(int i = 0; i < iNalNum; i++)
    {
        delete []pNALArray[i].data;
        pNALArray[i].data = NULL;
    }
    delete []pNALArray;
    pNALArray = NULL;
}

