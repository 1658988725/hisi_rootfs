#ifndef _CDR_MPP_H_
#define _CDR_MPP_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>

#include "sample_comm.h"
#include "cdr_audio.h"
#include "cdr_writemov.h"
#include "sample_comm.h"
#include "acodec.h"
#include "cdr_vedio.h"
#include "cdr_comm.h"


extern VENC_GETSTREAM_CH_PARA_S gsjpeg_stPara;
extern VENC_GETSTREAM_CH_PARA_S gsqcif_stPara;

int cdr_mpp_init(void);
int cdr_mpp_release(void);
void cdr_stream_setevent_callbak(int ntype,cdr_stream_callback callback);
void cdr_capture_jpg(int type);
HI_S32 cdr_save_jpg(VENC_CHN VencChn,char *pJpgName);
int get_jpg_tm_name(char*path,char *pfullFileName,char *pName);
#endif 