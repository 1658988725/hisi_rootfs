#include <string.h>
#include <stdio.h>
#include "cdr_mpp.h"
#include "cdr_vedio.h"
#include "cdr_app_service.h"
#include "cdr_XmlLog.h"
#include "cdr_config.h"
#include "cdr_vedio.h"

#include "cdr_exif.h"
#include "hi_comm_isp.h"
#include "mpi_isp.h"
#include "cdr_comm.h"
#include "cdr_rtsp_push.h"
#include "cdr_bubiao_analyze.h"
#include "cdr_upload.h"

/***********************Data flow:***************************
	Sersor--> bufferpool -->Callbac function --> net or sd.
************************************************************/


VENC_GETSTREAM_CH_PARA_S gsjpeg_stPara;
VENC_GETSTREAM_CH_PARA_S gsqcif_stPara;


//static VENC_CHN	g_jepgchn = 2;
//static VENC_CHN	g_indexjepgchn = 3;

#define NORMAL_JPG 1
#define GSENSOR_JPG 2
#define NORMAL_QCIF 3
#define AVIDEO_QCIF 4
#define JPGPRE_QCIF 5

extern GPS_INFO g_GpsInfo;

//mv gcif to zip.
void cdr_zip_qcif(void)
{
	time_t curTime;
	struct tm *ptm = NULL;
	char tmp[256] = {0};			
	time(&curTime);
	ptm = gmtime(&curTime);

	memset(tmp,0x00,256);
	//don't need path in the zip file.
	sprintf(tmp, "zip -qj /mnt/mmc/INDEX/%04d%02d%02d%02d%02d%02d.zip /mnt/mmc/INDEX/*.jpg", ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
	cdr_system(tmp);
	cdr_system("rm -rf /mnt/mmc/INDEX/*jpg");
	printf("%s zip qcif ok..\n",__FUNCTION__);
	
}

void cdr_init_gps_info()
{
	if(g_GpsInfo.ucDataValidFlag != 0)
		return;

	time_t curTime;
	struct tm *ptm = NULL;

	time(&curTime);
	ptm = gmtime(&curTime);

	g_GpsInfo.D.year  = ptm->tm_year+1900;
	g_GpsInfo.D.month = ptm->tm_mon+1;
	g_GpsInfo.D.day = ptm->tm_mday;
	g_GpsInfo.D.hour = ptm->tm_hour;
	g_GpsInfo.D.minute = ptm->tm_min;
	g_GpsInfo.D.second = ptm->tm_sec;

	g_GpsInfo.latitude = 0;
	g_GpsInfo.longitude = 0;
	
	g_GpsInfo.latitude_Degree = 113;//纬度
	g_GpsInfo.latitude_Cent = 46;
	g_GpsInfo.latitude_Second = 10;
	
	g_GpsInfo.longitude_Degree = 22;    //经度
	g_GpsInfo.longitude_Cent = 27;    //经度
	g_GpsInfo.longitude_Second = 11;    //经度
	g_GpsInfo.speed = 0;    //经度
	g_GpsInfo.direction = 0;    //经度
	g_GpsInfo.height = 0;    //经度
	g_GpsInfo.satellite = 0;    //经度
	g_GpsInfo.ucDataValidFlag = 0;    //经度		
	g_GpsInfo.NS = 0;	 //经度
	g_GpsInfo.EW = 0;    //经度

}
void cdr_captue_finish(int type,char *fullname,char *name)
{
	//printf("%s %d %s %s \r\n",__FUNCTION__,__LINE__,fullname,name);
    char url[200] = {0};	
	cdr_init_gps_info();				    
	switch(type)
	{
		case NORMAL_JPG:
			cdr_add_log(ePHOTO,"media",name);
			cdr_write_exif_to_jpg(fullname,g_GpsInfo);
			if(GetCapturePictureFlag()==0x01){   //上传照片            
                GetUrlBuff(url);
                printf("url %s,fullname:%s\n",url,fullname);
                cdr_upload(url,fullname);
                SetCapturePictureFlag(0x00);                
            }
		break;
		case GSENSOR_JPG:			
			cdr_add_log(eGPHOTO,"media",name);
	         cdr_write_exif_to_jpg(fullname,g_GpsInfo);            
	        //memcpy(g_cdr_systemconfig.recInfo.strAVPicName,fullname,strlen(fullname));
		break;
#if 0			
		case NORMAL_QCIF:
		case AVIDEO_QCIF:
		break;
#endif		
		default:
		break;
	}
}


HI_S32 save_stream_to_jepg(VENC_STREAM_S *pstStream, char *pJepgName)
{
	FILE *pFile = NULL;
	VENC_PACK_S*  pstData;
	HI_U32 i;
	
    pFile = fopen(pJepgName, "wb");
    if (pFile == NULL)
    {
        SAMPLE_PRT("open file err\n");
        return HI_FAILURE;
    }
	
    for (i = 0; i < pstStream->u32PackCount; i++)
    {
        pstData = &pstStream->pstPack[i];
        fwrite(pstData->pu8Addr+pstData->u32Offset, pstData->u32Len-pstData->u32Offset, 1, pFile);
        fflush(pFile);
    }
	
    fclose(pFile);           
    return HI_SUCCESS;
}

HI_S32 cdr_save_pre_jpg(char *pJpgpreName)
{
	//printf("pJpgpreName:%s\r\n",pJpgpreName);
	char *p = NULL;	
	char chPreName[256],tmp[256];
	memcpy(tmp,pJpgpreName,256);
	p = strchr(tmp,'.');
	*p = '\0';
	sprintf(chPreName,"%s_pre.jpg",tmp);
	return cdr_save_jpg(gsqcif_stPara.s32VencChn,chPreName);	

}

HI_S32 cdr_save_jpg(VENC_CHN VencChn,char *pJpgName)
{
    struct timeval TimeoutVal;
    fd_set read_fds;
    HI_S32 s32VencFd;
    VENC_CHN_STAT_S stStat;
    VENC_STREAM_S stStream;
    HI_S32 s32Ret;
	VENC_RECV_PIC_PARAM_S stRecvParam;
    
    /******************************************
     step 2:  Start Recv Venc Pictures
    ******************************************/
    stRecvParam.s32RecvPicNum = 1;
    s32Ret = HI_MPI_VENC_StartRecvPicEx(VencChn,&stRecvParam);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_VENC_StartRecvPic faild with%#x!\n", s32Ret);
        return HI_FAILURE;
    }
    /******************************************
     step 3:  recv picture
    ******************************************/
    s32VencFd = HI_MPI_VENC_GetFd(VencChn);
    if (s32VencFd < 0)
    {
    	 SAMPLE_PRT("HI_MPI_VENC_GetFd faild with%#x!\n", s32VencFd);
        return HI_FAILURE;
    }

    FD_ZERO(&read_fds);
    FD_SET(s32VencFd, &read_fds);
    
    TimeoutVal.tv_sec  = 2;
    TimeoutVal.tv_usec = 0;
    s32Ret = select(s32VencFd+1, &read_fds, NULL, NULL, &TimeoutVal);
    if (s32Ret < 0) 
    {
        SAMPLE_PRT("snap select failed!\n");
        return HI_FAILURE;
    }
    else if (0 == s32Ret) 
    {
        SAMPLE_PRT("snap time out!\n");
        return HI_FAILURE;
    }
    else
    {
        if (FD_ISSET(s32VencFd, &read_fds))
        {
            s32Ret = HI_MPI_VENC_Query(VencChn, &stStat);
            if (s32Ret != HI_SUCCESS)
            {
                SAMPLE_PRT("HI_MPI_VENC_Query failed with %#x!\n", s32Ret);
                return HI_FAILURE;
            }
			
			/*******************************************************
			 suggest to check both u32CurPacks and u32LeftStreamFrames at the same time,for example:
			 if(0 == stStat.u32CurPacks || 0 == stStat.u32LeftStreamFrames)
			 {
				SAMPLE_PRT("NOTE: Current  frame is NULL!\n");
				return HI_SUCCESS;
			 }
			*******************************************************/
			if(0 == stStat.u32CurPacks)
			{
				  SAMPLE_PRT("NOTE: Current  frame is NULL!\n");
				  return HI_SUCCESS;
			}
            stStream.pstPack = (VENC_PACK_S*)malloc(sizeof(VENC_PACK_S) * stStat.u32CurPacks);
            if (NULL == stStream.pstPack)
            {
                SAMPLE_PRT("malloc memory failed!\n");
                return HI_FAILURE;
            }

            stStream.u32PackCount = stStat.u32CurPacks;
            s32Ret = HI_MPI_VENC_GetStream(VencChn, &stStream, -1);
            if (HI_SUCCESS != s32Ret)
            {
                SAMPLE_PRT("HI_MPI_VENC_GetStream failed with %#x!\n", s32Ret);
                free(stStream.pstPack);
                stStream.pstPack = NULL;
                return HI_FAILURE;
            }
			s32Ret = save_stream_to_jepg(&stStream,pJpgName);
            //s32Ret = SAMPLE_COMM_VENC_SaveSnap(&stStream, bSaveJpg, bSaveThm);
            if (HI_SUCCESS != s32Ret)
            {
                SAMPLE_PRT("HI_MPI_VENC_GetStream failed with %#x!\n", s32Ret);
                free(stStream.pstPack);
                stStream.pstPack = NULL;
                return HI_FAILURE;
            }

            s32Ret = HI_MPI_VENC_ReleaseStream(VencChn, &stStream);
            if (s32Ret)
            {
                SAMPLE_PRT("HI_MPI_VENC_ReleaseStream failed with %#x!\n", s32Ret);
                free(stStream.pstPack);
                stStream.pstPack = NULL;
                return HI_FAILURE;
            }

            free(stStream.pstPack);
            stStream.pstPack = NULL;
	    }
    }
    /******************************************
     step 4:  stop recv picture
    ******************************************/
    s32Ret = HI_MPI_VENC_StopRecvPic(VencChn);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_PRT("HI_MPI_VENC_StopRecvPic failed with %#x!\n",  s32Ret);
        return HI_FAILURE;
    }


    return HI_SUCCESS;
}


int cdr_qcif_thread(void)
{
	char fullJpgName[256];//全路径名字
	char qcifName[256];	  //名字	
	while( gsqcif_stPara.bThreadStart)
	{
		sleep(20);
		memset(fullJpgName,0x00,256);
		get_jpg_tm_name("/mnt/mmc/INDEX/",fullJpgName,qcifName);
		cdr_save_jpg(gsqcif_stPara.s32VencChn,fullJpgName);		
	}	 
	return 1;
}

int get_Gjpg_tm_name(char*path,char *pfullFileName,char *pName)
{
    
	if (NULL == path || NULL == pfullFileName || pName == NULL) return -1;

	time_t curTime;
	struct tm *ptm = NULL;
	char tmp[256] = {0};

	
	time(&curTime);
	ptm = gmtime(&curTime);
	
	memset(tmp,0x00,256);
	sprintf(tmp, "%sG%04d%02d%02d%02d%02d%02d.jpg", path,ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
	tmp[strlen(tmp)] = '\0';
	strcpy(pfullFileName, tmp);


	memset(tmp,0x00,256);
	sprintf(tmp, "G%04d%02d%02d%02d%02d%02d.jpg", ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
	tmp[strlen(tmp)] = '\0';
	strcpy(pName, tmp);
	
	return 0;	

}
int get_jpg_tm_name(char*path,char *pfullFileName,char *pName)
{
	if (NULL == path || NULL == pfullFileName || pName == NULL)
	{
		return -1;
	}

	time_t curTime;
	struct tm *ptm = NULL;
	char tmp[256] = {0};

	
	time(&curTime);
	ptm = gmtime(&curTime);
	
	memset(tmp,0x00,256);
	sprintf(tmp, "%s%04d%02d%02d%02d%02d%02d.jpg", path,ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
	tmp[strlen(tmp)] = '\0';
	strcpy(pfullFileName, tmp);


	memset(tmp,0x00,256);
	sprintf(tmp, "%04d%02d%02d%02d%02d%02d.jpg", ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
	tmp[strlen(tmp)] = '\0';
	strcpy(pName, tmp);
	
	return 0;	
}


void cdr_capture_jpg(int type)
{
	char fullJpgName[256];//全路径名字
	char jpgName[256];	  //名字
	gsjpeg_stPara.s32VencChn = 2;

	int jpgtype = NORMAL_JPG;

	cdr_play_audio(CDR_AUDIO_IMAGECAPUTER,0);	
	if(type == 0)
	{		
		get_jpg_tm_name("/mnt/mmc/PHOTO/",fullJpgName,jpgName);
		jpgtype = NORMAL_JPG;
	}
	else
	{
		get_Gjpg_tm_name("/mnt/mmc/PHOTO/",fullJpgName,jpgName);
		jpgtype = GSENSOR_JPG;
	}
	if (0 != access(fullJpgName, W_OK))
	{
		cdr_save_pre_jpg(fullJpgName);
		cdr_save_jpg(gsjpeg_stPara.s32VencChn,fullJpgName);			
	}
	
	cdr_captue_finish(jpgtype,fullJpgName,jpgName);	
	ack_capture_update(jpgName);
	
}


//每20秒生成一个index图片。
int cdr_init_indexqcif_thread()
{
	pthread_t tfid;
	int ret = 0;

	gsqcif_stPara.bThreadStart = HI_TRUE;
	gsqcif_stPara.s32VencChn = 3;

	ret = pthread_create(&tfid, NULL, (void *)cdr_qcif_thread, NULL);
	if (ret != 0)
	{
		printf("pthread_create failed, %d, %s\n", errno, strerror(errno));
		return -1;
	}
	pthread_detach(tfid);
	return 0;

}


/*
function:cdr_mpp_init(void)
1.create bufferpool.
2.init hisi mpp.(sersor isp ..)
3.init region thread.
4.create read buffer thread.and write to bufferpool.
5.create read buffer from bufferpool thread.
return:0 OK;-1 failed.
*/
int cdr_mpp_init(void)
{
	int nRet = 0,i;
	for(i = 0;i<CDR_CALLBACK_MAX;i++)
	{
		g_fun_callbak[i] = NULL;
	}
		
	cdr_videoInit();//video and osd
	cdr_audioInit();	

	cdr_isp_init();	

	gsrec_stPara.s32VencChn = 0;
	gsrec_stPara.framerate = 30;
	gslive_stPara.s32VencChn = 1;
	gslive_stPara.framerate = 30;

	//init_jpeg_thread();
	cdr_init_indexqcif_thread();
	
	return nRet;
}

/*
function:cdr_mpp_release(void)
1.stop thread (read & send thread.)
2.release bufferpool.
2.uninit hisi mpp.
return:0 OK;-1 failed.
*/
int cdr_mpp_release(void)
{
	int nRet = 0;
	
	//cdr_audio_release();
	cdr_video_release();		
	
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
void cdr_stream_setevent_callbak(int ntype,cdr_stream_callback callback)
{	
	if(callback == NULL)
	{
		printf("[%s->%d] callbak is NULL\r\n",__FUNCTION__,__LINE__);
		return;
	}
	g_fun_callbak[ntype] = callback;	
	
	int i = 0;
	for(i = 0;i<CDR_CALLBACK_MAX;i++)
	{
		if(g_fun_callbak[i] != NULL){
            printf("%s g_fun_callbak[%d] is register.\n",__FUNCTION__,i);
		}
	}
}
