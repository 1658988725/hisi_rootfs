/******************************************************************************
  A simple program of Hisilicon HI3531 video encode implementation.
  Copyright (C), 2010-2011, Hisilicon Tech. Co., Ltd.
 ******************************************************************************
    Modification:  2011-2 Created
******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <assert.h>
#include "cdr_mpp.h"
#include "cdr_app_service.h"
#include "cdr_device_discover.h"
#include "rtspd_api.h"
#include "cdr_XmlLog.h"
#include "cdr_mp4_api.h"
#include "cdr_config.h"
#include "cdr_comm.h"

#define CDR_RTSP_PORT 554

#if 0
void test_get_h264(void)
{
	sleep(10);
	char *pData = NULL;
	int len = 0;
    int iFrame = 0;
	int nCount = 1000;
	FILE *fp = fopen("ReadMp4.264", "w+");
	char Nal[5] = {0x00,0x00,0x00,0x01};
	int flag = 0;
	while(nCount)
	{
		if(0 == cdr_mp4ex_read_vframe(&pData,&len,&iFrame))
		{
			if(iFrame == CDR_H264_NALU_ISLICE)
			{
				fwrite(Nal,4,1,fp);
				fwrite(recspsdata,recspslen,1,fp);

				
				fwrite(Nal,4,1,fp);
				fwrite(recppsdata,recppslen,1,fp);
				flag = 1;
			}

			if(flag && pData && len > 4)
			{
				printf("Read nCount:%d\n",nCount);
				fwrite(pData,len,1,fp);
				fflush(fp);
				SAFE_FREE(pData);
				nCount --;
			}
		}
		//printf("Read nCount:%d\n",nCount);		
	}	
	fclose(fp);
	sync();
	printf("Get 264 finish ...\n");
	
}
#endif

int main(int argc, char *argv[])
{	
	HI_S32 s32Ret = HI_TRUE;

	printf("Software Compiled Time: %s %s\r\n",__DATE__, __TIME__);
	signal(SIGPIPE, SIG_IGN);

	cdr_config_init();

    set_cdr_start_time();    

	cdr_mpp_init();		

	//Init sd.and begin rec....
	cdr_init_record();//录相	

	cdr_init_mp4dir(MP4DIRPATH);
	//init protocol.
	cdr_app_service_init();
	
	cdr_device_discover_init();

	cdr_rtsp_init(CDR_RTSP_PORT);	
	cdr_rec_rtsp_init(); //回放 rtsp
	cdr_live_rtsp_init();//直播 rtsp

    cdr_device_init();    

	//test_get_h264();

	//Wait for finish.
	cdr_system_wait();

	//wait save all to SD.	
	cdr_device_deinit();	

	while(1)
	{
		//the second time system power up.
		if(cdr_get_powerflag())
		{
			printf("reboot system\n");
			cdr_system("reboot");
	     }
		printf("cdr wait for end..\r\n");
		usleep(1000000);
	}
    exit(s32Ret);

}

