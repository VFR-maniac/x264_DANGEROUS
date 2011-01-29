
#include <zlib.h>

typedef struct {
    unsigned char   c_flag1;
    unsigned char   c_flag2;
    float           f_energy1;
    float           f_energy2;
    float           f_qpm;
} AQ_LOG_MB;

typedef struct {
    AQ_LOG_MB   *p_mb;
} AQ_LOG_BODY;



#ifdef X264_BUILD

void x264_write_aqlog_header( x264_t *h, char* p_format, int i_aq_metric,
                              float *f_aq1_strength, float *f_aq2_strength, float f_aq1_sensitivity, float f_aq2_sensitivity,
                              int *i_aq_boundary )
{
    fwrite( p_format, sizeof(char), 8, h->rc->p_aq_debug );
    fwrite( &i_aq_metric, sizeof(int), 1, h->rc->p_aq_debug );
#if defined(X264_AQ_MODE_ORE04) || defined(X264_AQ_MODE_ORE05)
    fwrite( f_aq1_strength, sizeof(float), 4, h->rc->p_aq_debug );
    fwrite( f_aq2_strength, sizeof(float), 4, h->rc->p_aq_debug );
#else
    fwrite( f_aq1_strength, sizeof(float), 1, h->rc->p_aq_debug );
    fwrite( f_aq2_strength, sizeof(float), 1, h->rc->p_aq_debug );
#endif
    fwrite( &f_aq1_sensitivity, sizeof(float), 1, h->rc->p_aq_debug );
    fwrite( &f_aq2_sensitivity, sizeof(float), 1, h->rc->p_aq_debug );
#if defined(X264_AQ_MODE_ORE04) || defined(X264_AQ_MODE_ORE05)
    fwrite( i_aq_boundary, sizeof(int), 3, h->rc->p_aq_debug );
#endif
}

void x264_write_aqlog_body( x264_t *h, int i_mb_xy, x264_frame_t *frame, int i_flag1, int i_flag2, float f_energy1, float f_energy2)
{
#define COMP_LEVEL 9
    AQ_LOG_BODY *body = (AQ_LOG_BODY*)h->rc->aqlog;

    int ret;
    int i_mb_count = h->mb.i_mb_count;
    unsigned int mb_size = sizeof(AQ_LOG_MB) * i_mb_count;
    unsigned int buf_size = mb_size * 1.5;
    unsigned int cmp_size;

    static x264_pthread_mutex_t hMutex = NULL;
    if( hMutex == NULL )
        x264_pthread_mutex_init( &hMutex, NULL );

    if( body == NULL )
    {
        body            = (AQ_LOG_BODY*)malloc(sizeof(AQ_LOG_BODY));
        body->p_mb      = (AQ_LOG_MB*)malloc(mb_size);
        h->rc->aqlog    = body;
    }

    body->p_mb[i_mb_xy].c_flag1     = (unsigned char)i_flag1;
    body->p_mb[i_mb_xy].c_flag2     = (unsigned char)i_flag2;
    body->p_mb[i_mb_xy].f_energy1   = f_energy1;
    body->p_mb[i_mb_xy].f_energy2   = f_energy2;
    body->p_mb[i_mb_xy].f_qpm       = h->rc->qpm;

    if( i_mb_xy == (i_mb_count-1) )
    {
        unsigned char *buf;
        int i_frame;

        // threadロック
        x264_pthread_mutex_lock( &hMutex );

        buf = (unsigned char*)malloc(buf_size);
        i_frame = frame->i_frame;
        cmp_size = buf_size;
        ret = compress2( buf, &cmp_size, body->p_mb, mb_size, COMP_LEVEL);
        if ( ret == Z_OK && cmp_size <= mb_size )
        {
            fwrite( &i_frame, sizeof(i_frame), 1, h->rc->p_aq_debug );
            fwrite( &cmp_size, sizeof(cmp_size), 1, h->rc->p_aq_debug );
            fwrite( buf, cmp_size, 1, h->rc->p_aq_debug );
        }
        else
        {
            fwrite( &i_frame, sizeof(i_frame), 1, h->rc->p_aq_debug );
            fwrite( &mb_size, sizeof(mb_size), 1, h->rc->p_aq_debug );
            fwrite( body->p_mb, mb_size, 1, h->rc->p_aq_debug );
        }
        free( buf );
        free( body->p_mb );
        free( body );
        h->rc->aqlog = NULL;

        // threadロック開放
        x264_pthread_mutex_unlock( &hMutex );
    }
}

#endif // X264_BUILD
