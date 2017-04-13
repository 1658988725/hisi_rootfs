
#ifndef CDR_COMM_H
#define CDR_COMM_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "cdr_rec.h"
#include "cdr_rtsp_server.h"
#include "cdr_device.h"
#include "cdr_audio.h"
#include "cdr_app_service.h"


#define SOCKET_ERROR    (-1)
#define INVALID_SOCKET  (-1)
#define SK_EAGAIN   (EAGAIN)
#define SK_EINTR    (EINTR)
typedef int SOCKET;
typedef socklen_t SOCKLEN;



#define PRINT_ENABLE    1
#if PRINT_ENABLE
    #define DEBUG_PRT(fmt...)  \
        do {                 \
            printf("[%s - %d]: ", __FUNCTION__, __LINE__);\
            printf(fmt);     \
        }while(0)
#else
    #define DEBUG_PRT(fmt...)  \
        do { ; } while(0)
#endif



#ifndef SAFE_CLOSE
#define SAFE_CLOSE(fd)    do {  \
  if (fd >= 0) {                  \
    fclose(fd);                  \
    fd = NULL;                      \
  }                               \
} while (0)
#endif

#ifndef SAFE_FCLOSE
#define SAFE_FCLOSE(fd)    do {  \
  if (fd >= 0) {                  \
    fclose(fd);                  \
    fd = NULL;                      \
  }                               \
} while (0)
#endif

#ifndef SAFE_CLOSE1
#define SAFE_CLOSE1(fd)    do {  \
  if (fd >= 0) {                  \
    close(fd);                  \
    fd = NULL;                      \
  }                               \
} while (0)
#endif

#ifndef SAFE_FREE
#define SAFE_FREE(p)    do {  \
  if (p != NULL) {                  \
    free(p);                  \
    p = NULL;                      \
  }                               \
} while (0)
#endif


#define SD_MOUNT_PATH "/mnt/mmc/"
#define CDR_CUTMP4_EX_TIME 5

#define CDR_FW_VERSION "vs.2.1-18"
#define CDR_HW_VERSION "vh.1.1"

#define AUDIO_SAMPLE_RATE  8000 
#define CDR_AUDIO_BOOTVOICE		0
#define CDR_AUDIO_IMAGECAPUTER 	1
#define CDR_AUDIO_NOSD 			3
#define CDR_AUDIO_UPDATE 		4
#define CDR_AUDIO_AssociatedVideo 		5 //关联视频.
#define CDR_AUDIO_FM 			6 //FM
#define CDR_AUDIO_RESETSYSTEM 	7 //恢复出厂设置
#define CDR_AUDIO_FORMATSD 		8 //格式化SD卡
#define CDR_AUDIO_SETTIMEOK 	9//设置系统时间OK
#define CDR_AUDIO_SYSTEMUPDATE 	10//系统升级
#define CDR_AUDIO_LT8900PAIR 	11//蓝牙按键配对
#define CDR_AUDIO_STARTREC 		12//开始录像
#define CDR_AUDIO_USBOUT 		13//USB out power off.
#define CDR_AUDIO_DIDI 			14//强制录制视频连续hold声音

#define CDR_GPS_OK 	15//gps 导航ok

#define MAKEWORD(b, a)  ((unsigned short) (((unsigned char) (a)) | ((unsigned short) ((unsigned char) (b))) << 8))
#define LOBYTE(w)   ((unsigned char) (w))
#define HIBYTE(w)   ((unsigned char) (((unsigned short) (w) >> 8) & 0xFF))

extern int recspslen;
extern int recppslen;

extern unsigned char recspsdata[64];
extern unsigned char recppsdata[64];

//vaframe info
typedef struct{
	short isIframe;//帧类型而非I帧类型
	short iEnc;
	short iStreamtype;//video / audio
	unsigned long long u64PTS;	
}sVAFrameInfo;

enum CDR_FUNCALLBAK_TYPE
{
	CDR_REC_TYPE     = 0,
	CDR_LIVE_TYPE    = 1,
	CDR_SNAP_TYPE    = 2,
	CDR_INDEX_TYPE   = 3,
	CDR_PHUSH_TYPE   = 4,
	CDR_CALLBACK_MAX   = 5
};

typedef struct venc_getstream_ch_s
{
     int bThreadStart;
	 int  s32Cnt;
     int  s32VencChn;
	 int  framerate;
	 unsigned char  nsnapflag;
 	 unsigned char  exsnapflag;//snap 快照 ，对应不同位置的照片
}VENC_GETSTREAM_CH_PARA_S;


#define STREAM_VIDEO 0
#define STREAM_AUDIO 1

extern VENC_GETSTREAM_CH_PARA_S gsrec_stPara;
extern VENC_GETSTREAM_CH_PARA_S gslive_stPara;

#define QUEUE_SIZE 64
typedef struct _kfifo
{
    void * data[ QUEUE_SIZE ];
    unsigned int in; 
    unsigned int out;
}kfifo;


int kfifo_put(kfifo *fifo, void * element );
int kfifo_get(kfifo *fifo, void **element );


typedef int (*cdr_stream_callback)(int pStreamType,char* pData,unsigned int nSize,sVAFrameInfo vaFrame);

cdr_stream_callback g_fun_callbak[CDR_CALLBACK_MAX];

time_t _strtotime(char *pstr);
void cdr_log_get_time(char * ucTimeTemp);
int set_cdr_start_time(void);
//int cdr_system_new(const char * cmd);
void cdr_get_curr_time(char * ucTimeTemp);

void PrintTime();
void PrintfString2Hex(unsigned char *pSrc,int iLen);
void PrintfString2Hex2(char *pSrc,int len);
void TarGPSGrailLog(void);

void TimeStr2BCD(const char *sTimeStr,char *sBCDBuf);
char* substring(char* ch,int pos,int length,char* subch);

#endif



