#ifndef _CDR_REC_H_
#define _CDR_REC_H_
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <netdb.h>
#include <sys/socket.h>
//#include <syswait.h>
#include <sys/ioctl.h>
#include <fcntl.h> 
#include <pthread.h>
#include <sys/ipc.h> 
#include <sys/msg.h>
#include <netinet/if_ether.h>
#include <net/if.h>

#include <linux/if_ether.h>
#include <linux/sockios.h>
#include <netinet/in.h> 
#include <arpa/inet.h> 
#include <dirent.h>


#define STREAM_VIDEO 0
#define STREAM_AUDIO 1



typedef int (*cdr_rec_data_callback)(int pStreamType,char* pData,unsigned int nSize,int isIFrame);
extern unsigned long long g_rec_u64PTS;	
extern int g_rec_flag;
extern int g_rec_pasueflag;
extern cdr_rec_data_callback g_rec_data_callbak;

int cdr_rec_rtsp_init(void);
void cdr_rec_start(void);
int cdr_rec_settime(char *pTime);
void cdr_rec_realease(void);
void cdr_rec_start(void);
void cdr_rec_stop(void);
int rec_stream_callbak(int pStreamType,char* pData,unsigned int nSize,int isIFrame);
int cdr_rec_mp4_read(void);

#endif
