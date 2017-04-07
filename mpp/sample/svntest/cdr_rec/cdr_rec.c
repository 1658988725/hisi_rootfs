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

#include "cdr_rtsp_server.h"
#include "cdr_comm.h"
#include "rtspd_api.h"
#include "cdr_mp4_api.h"


cdr_rec_data_callback g_rec_data_callbak = NULL;
int g_rec_pasueflag = 1;
int g_rec_flag = 0;
unsigned long long g_rec_u64PTS = 0;	
int g_nRecSession = -1;
int g_nRecSessionStatus = 0;
void cdr_rec_registre_data_callbak(cdr_rec_data_callback callback);
	

int cdr_rec_rtsp_cb(int nStatus)
{
	g_nRecSessionStatus = nStatus;
	printf("*************************************\r\n");
	printf("%s nStatus:%d\r\n",__FUNCTION__,nStatus);	
	if(g_nRecSessionStatus == 4)
	{
		cdr_rec_start();
		g_nLivePlayFlag = 0;//防止冲突.
	}
	else if(g_nRecSessionStatus == 5)
	{
		cdr_rec_stop();
	}
	return 0;
}


int cdr_rec_rtsp_init(void)
{
	g_rec_flag = 1;
	g_rec_u64PTS = 0;
	g_rec_pasueflag = 1;

	//创建MP4读取线程
	cdr_rec_mp4_read();

	//for test
	cdr_rec_registre_data_callbak(rec_stream_callbak);	
	g_nRecSession = cdr_rtsp_create_mediasession("rec.sdp",1920*1080,cdr_rec_rtsp_cb);

	if(g_nRecSession >= 0) return 0;
	
	return -1;
}

//释放所有的资源.
void cdr_rec_realease()
{
	g_rec_flag = 0;
}

void cdr_rec_start(void)
{
	g_rec_pasueflag = 0;
}

void cdr_rec_stop(void)
{
	g_rec_pasueflag = 1;
}


//播放切换到某个时间点.
//Return:0 设置OK
//Return:-1 参数错误.文件不存在.
int cdr_rec_settime(char *pTime)
{
	int nRet = 0;
	cdr_rec_stop();
	printf("%s %s\n",__FUNCTION__,pTime);
	nRet = cdr_mp4ex_seek(pTime);	
	cdr_rec_start();
	return nRet;
}

/*
params:
	pStreamType	:0 video,1 audio.
	pData		:buffer
	nSize		:length of buffer
	freamType		:fame freamType.
return:
单独线程调用，等待执行完毕才会继续执行.
	阻塞函数.	
*/
void cdr_rec_registre_data_callbak(cdr_rec_data_callback callback)
{
	if(callback == NULL)
	{
		printf("[%s->%d] callbak is NULL\r\n",__FUNCTION__,__LINE__);
		return;
	}
	g_rec_data_callbak = callback;		
}

//char spspps[40];
/*********************************for test**************************************/
int rec_stream_callbak(int pStreamType,char* pData,unsigned int nSize,int isIFrame)
{
	//printf("%s nSize:%d,isIFrame:%d\n",__FUNCTION__,nSize,isIFrame);
	
	static unsigned long long pts = 0;
	static char exData[64];
	int len = 0;	
	if(isIFrame)
	{	
		memset(exData,0x00,sizeof(exData));
		exData[0] = 0x00;
		exData[1] = 0x00;
		exData[2] = 0x00;
		exData[3] = 0x01;
		memcpy(exData+4,recspsdata,recspslen);
		len = recspslen + 4;
		while(0 != cdr_rtsp_send_frame(g_nRecSession,exData,len,pts))
		{
			usleep(100);
		}
		
		memset(exData,0x00,sizeof(exData));
		exData[0] = 0x00;
		exData[1] = 0x00;
		exData[2] = 0x00;
		exData[3] = 0x01;
		memcpy(exData+4,recppsdata,recppslen);
		len = recppslen + 4;
		while(0 != cdr_rtsp_send_frame(g_nRecSession,exData,len,pts))
		{
			usleep(100);
		}

	}	
		while(0 != cdr_rtsp_send_frame(g_nRecSession,pData,nSize,pts));
		{
			usleep(100);		
		}
	
	pts += 33333;

	usleep(300);
	return 0;
}

int rec_mp4_read_pro(void)
{
	char *pData = NULL;
	int len;
	int iFrame;
	while (g_rec_flag)
	{
		if(g_rec_pasueflag == 1)
		{
			usleep(300);
			continue;
		}
		usleep(10);
		len = 0;
		iFrame = 0;
		pData = NULL;
		if(g_rec_data_callbak != NULL && 0 == cdr_mp4ex_read_vframe(&pData,&len,&iFrame))
		{	
		
			if(iFrame == CDR_H264_NALU_ISLICE)
				iFrame = 1;
			else
				iFrame = 0;
				
			g_rec_data_callbak(STREAM_VIDEO,pData,len,iFrame);
		}	
		
		if(pData){
            free(pData);
            pData = NULL;
		}
		//usleep(5);	
	}
	if(pData!=NULL){
        free(pData);
        pData = NULL;
	}
	
	return 0;
}


//Read from sd.
int cdr_rec_mp4_read(void)
{
	int ret = -1;
	
	pthread_attr_t attr;
	struct sched_param param;			
	pthread_attr_init(&attr);
	pthread_attr_setschedpolicy(&attr, SCHED_RR);
	param.sched_priority = sched_get_priority_max(SCHED_RR);
	pthread_attr_setschedparam(&attr, &param);		
	pthread_t tfid;
	ret = pthread_create(&tfid, &attr, (void *)rec_mp4_read_pro, NULL);
	if (0 != ret)
	{
		//DEBUG_PRT("create TF record thread failed!\n");
		return -1;
	}
	pthread_detach(tfid);	
	pthread_attr_destroy(&attr);
	
	return 0;
}

