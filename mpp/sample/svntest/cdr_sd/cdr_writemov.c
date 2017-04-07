#include <sys/vfs.h>
#include <dirent.h>
#include "cdr_writemov.h"
#include "cdr_comm.h"
#include "mp4v2/mp4v2.h"
#include "sample_comm.h"
#include "cdr_config.h"
#include "cdr_mp4_api.h"
#include "cdr_audio.h"
#include "cdr_led.h"
#include "cdr_app_service.h"
#include "cdr_mpp.h"


short g_TF_flag = 0;
int g_mp4handle = -1;

#define SD_MOUNT_PATH "/mnt/mmc/"
SD_PARAM g_SdParam;
int g_sdIsOnline = 0;
#define VENC_REC_CHN 0
#define VENC_LIVE_CHN 1


static int GetMP4Name(char *pFileName);
static void cdr_load_sd(void);
int sd_update_file(void);
void cdr_video_index_file_synchronous(void);
int cdr_create_record_thread(void);
int CreateMp4File_ex(char *pFileName,int timelen);
int cdr_add_mp4_to_demuxlist(char *fileName);


unsigned int g_mp4filelen = 180*1000;

int cdr_init_record(void)
{
	cdr_load_sd();
	cdr_video_index_file_synchronous();

    TarGPSGrailLog();
	//cdr_sd_init();
	cdr_create_record_thread();
	return 0;
}
//Get sd status:1 mount OK.0 no sd..
int cdr_get_sd_status()
{
	return g_sdIsOnline;
}


//录相 处理线程，完成功能:将pool中的视频流 放入 mp4中
//0 sd 卡的前期处理
/////1 create mp4 file
///2 往mp4文件中放数据
int cdr_record_thread_pro(void)
{
	char fileName[256] = {0};    
	
	while(1){
		usleep(1);
		if(g_sdIsOnline == 0){
			printf("no sd....\n");
			cdr_load_sd();
			sleep(1);
			cdr_led_contr(LED_RED,LED_STATUS_FLICK);
			continue;
		}

		if(1 != sd_update_file()){
			continue;
		}

		//When power off. save mp4 to sd.
		if(0 == cdr_get_powerflag())	break;
		
		cdr_led_contr(LED_RED,LED_STATUS_DFLICK);	
		memset(fileName,0,sizeof(fileName));
		GetMP4Name(fileName);		
		CreateMp4File_ex(fileName,g_mp4filelen);		
		
	}
	
	return 1;

}

//create record thread
int cdr_create_record_thread(void)
{
	if (0 == g_TF_flag)
	{
		int ret = -1;
		g_TF_flag = 1;

		pthread_t tfid;
		ret = pthread_create(&tfid, NULL, (void *)cdr_record_thread_pro, NULL);
		if (0 != ret)
		{
			DEBUG_PRT("create TF record thread failed!\n");
			return -1;
		}
		pthread_detach(tfid);
	}
	return 1;
}

static int GetMP4Name(char *pFileName)
{
	if (NULL == pFileName) 	return -1;

	time_t curTime;
	struct tm *ptm = NULL;
	char tmp[256] = {0};
	time(&curTime);
	ptm = gmtime(&curTime);
	sprintf(tmp, "/mnt/mmc/VIDEO/%04d%02d%02d%02d%02d%02d.mp4", 
        ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday, 
        ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
	tmp[strlen(tmp)] = '\0';
	strcpy(pFileName, tmp);

	return 0;	
}


//check MMC module.
static int CheckSDStatus()
{
    struct stat st;
    if (0 == stat("/dev/mmcblk0", &st))
    {
        if (0 == stat("/dev/mmcblk0p1", &st))
        {
            printf("...load TF card success...\n");
            return 1;
        }
        else
        {
            printf("...load TF card failed...\n");
            cdr_play_audio(CDR_AUDIO_NOSD,0);
            return 2;
        }
    }
    return 0;
}

static void cdr_load_sd(void)
{
    g_sdIsOnline = CheckSDStatus();
    if (g_sdIsOnline == 1) //index sd card inserted.
    {
        mkdir(SD_MOUNT_PATH, 0755);
        //system("umount /mnt/mmc/");
        cdr_system("mount -t vfat -o rw /dev/mmcblk0p1 /mnt/mmc/"); //mount SD.
        usleep(1000000);
        GetStorageInfo();

		mkdir("/mnt/mmc/VIDEO", 0755);
        mkdir("/mnt/mmc/PHOTO", 0755);
        mkdir("/mnt/mmc/INDEX", 0755);
		mkdir("/mnt/mmc/GVIDEO", 0755);
		cdr_system("rm -rf /mnt/mmc/VIDEO/*mp4");//删除有问题的mp4文件.		
    }
	else
	{
		g_SdParam.leftSize  = 0; 
	}
	
}


void cdr_unload_sd(void)
{
	cdr_system("sync");
	cdr_system("umount /mnt/mmc/"); //unmount SD.
}


/*******************************************
 * func: calculate SD card storage size.
 ******************************************/
int GetStorageInfo(void)
{
    struct statfs statFS;

    if (statfs(SD_MOUNT_PATH, &statFS) == -1)
    {  
        printf("error, statfs failed !\n");
        return -1;
    }

    g_SdParam.allSize   = ((statFS.f_blocks/1024)*(statFS.f_bsize/1024));
    g_SdParam.leftSize  = (statFS.f_bfree/1024)*(statFS.f_bsize/1024); 
    g_SdParam.haveUse   = g_SdParam.allSize - g_SdParam.leftSize;
    printf("scc SD totalsize=%ld...freesize=%ld...usedsize=%ld......\n", g_SdParam.allSize, g_SdParam.leftSize, g_SdParam.haveUse);
    return 0;
}

//Return :0 No enough space.1:have enough space.
int sd_update_file()
{
	int nRet = 0;
	if(g_sdIsOnline == 1 && GetStorageInfo() == 0)
	{
		nRet = 1;
		//lef 1G.
		if(g_SdParam.leftSize < 1024)
		{		
			//每次删除3个视频文件，和视频对应的jpg大图.
			cdr_system("ls -1 /mnt/mmc/VIDEO/* | head -1 | xargs rm -fr");
			cdr_system("ls -1 /mnt/mmc/VIDEO/* | head -1 | xargs rm -fr");	
			cdr_system("ls -1 /mnt/mmc/VIDEO/* | head -1 | xargs rm -fr"); 
			cdr_system("ls -1 /mnt/mmc/VIDEO/* | head -1 | xargs rm -fr"); 
			cdr_system("ls -1 /mnt/mmc/VIDEO/* | head -1 | xargs rm -fr"); 
			cdr_system("ls -1 /mnt/mmc/VIDEO/* | head -1 | xargs rm -fr"); 
			cdr_system("sync");
			cdr_video_index_file_synchronous();
			nRet = 0;
			GetStorageInfo();
			if(g_SdParam.leftSize >= 1024) nRet = 1;
		}

	}	

	return nRet ;
}

int update_sps_pps(void)
{
	//跳过下面malloc内存的流程.
	if(recspslen>0 && recppslen> 0)
	{
		printf("recspslen %d\n",recspslen);
		printf("recppslen %d\n",recppslen);
		return 0;
	}
	
	HI_S32 s32Ret = 0;
	static int s_vencChn = 0;  //Venc Channel.
	static int s_vencFd = 0;   //Venc Stream File Descriptor..
	static int s_maxFd = 0;    //mac fd for select.

	fd_set read_fds;
	VENC_STREAM_S stStream; //captured stream data struct.	
	VENC_CHN_STAT_S stStat;

	s_vencFd  = HI_MPI_VENC_GetFd(VENC_REC_CHN);
	s_maxFd   = s_vencFd + 1; //for select.
	s_vencChn = VENC_REC_CHN; //current video encode channel.	

	//struct sched_param param;
	struct timeval TimeoutVal;
	VENC_PACK_S *pstPack = NULL;	
	pstPack = (VENC_PACK_S *)malloc(sizeof(VENC_PACK_S) * 128);
	if (NULL == pstPack)
	{
		pstPack = NULL;
		return -1;
	}	
	sleep(1);
	int i;

	while(recspslen <=0 || recppslen <= 0 )
	{
		FD_ZERO( &read_fds );
		FD_SET( s_vencFd, &read_fds );

		TimeoutVal.tv_sec  = 2;
		TimeoutVal.tv_usec = 0;
		s32Ret = select( s_maxFd, &read_fds, NULL, NULL, &TimeoutVal );
		if (s32Ret <= 0)
		{
			printf("%s select failed!\n",__FUNCTION__);
			sleep(1);
		    continue;
		}

		//Select sucess.
		if (FD_ISSET( s_vencFd, &read_fds ))
		{
			s32Ret = HI_MPI_VENC_Query( s_vencChn, &stStat );
			if (HI_SUCCESS != s32Ret)
			{
				printf("HI_MPI_VENC_Query chn[%d] failed with %#x!\n", s_vencChn, s32Ret);
	            usleep(1000);
	            continue;
	        }

			stStream.pstPack = pstPack;
			stStream.u32PackCount = stStat.u32CurPacks;
			s32Ret = HI_MPI_VENC_GetStream( s_vencChn, &stStream, HI_TRUE );
			if (HI_SUCCESS != s32Ret)
			{
				printf("HI_MPI_VENC_GetStream main.. failed with %#x!\n", s32Ret);
	            usleep(1000);
	            continue;
			}


			for (i = 0; i < stStream.u32PackCount; i++)
			{
				if(stStream.pstPack[i].DataType.enH264EType == H264E_NALU_SPS)
				{
					recspslen =  stStream.pstPack[i].u32Len-stStream.pstPack[i].u32Offset -4;
					memcpy(recspsdata, stStream.pstPack[i].pu8Addr+stStream.pstPack[i].u32Offset+4,recspslen);	
				}

				if(stStream.pstPack[i].DataType.enH264EType == H264E_NALU_PPS)
				{
					recppslen =  stStream.pstPack[i].u32Len-stStream.pstPack[i].u32Offset -4;
					memcpy(recppsdata, stStream.pstPack[i].pu8Addr+stStream.pstPack[i].u32Offset+4,recppslen);	
				}
			}	

			s32Ret = HI_MPI_VENC_ReleaseStream(s_vencChn, &stStream);
			if (HI_SUCCESS != s32Ret)
			{
	            SAMPLE_PRT("HI_MPI_VENC_ReleaseStream chn[%d] main.. failed with %#x!\n", s_vencChn, s32Ret);
				stStream.pstPack = NULL;
	            usleep(1000);
	            continue;
			}
		}

	}
	
	if(pstPack) free(pstPack);
    pstPack = NULL;

	printf("recspslen %d\n",recspslen);
	printf("recppslen %d\n",recppslen);
	return 0;
}

/*
录mp4完成时调用的函数
*/
int record_mp4_finish(void* info)
{
	Mp4Context *p = info;
	char *pStr = NULL;
	printf("Record:%s finish\n",p->sName);
	cdr_add_mp4_to_demuxlist(p->sName);
	
	if(p->oMp4ExtraInfo.sPrePicFlag)
	{
		char chPreName[256];
		sprintf(chPreName,"%s_%03d.jpg",p->oMp4ExtraInfo.sPreName,p->oMp4ExtraInfo.nCutLen+CDR_CUTMP4_EX_TIME);
		rename(p->oMp4ExtraInfo.sPreName,chPreName);

		//notify to app.
		pStr = strrchr(chPreName,'/');
		pStr++;
		ack_capture_update(pStr);	

		//G-sensor mp4.
		if(p->oMp4ExtraInfo.nOutputMp4flag)
		{
			//这个需要保证mp4demux 线程已经解析完成了这个文件.
			//sleep(3);
			int nRet = 0;
			pStr = strrchr(p->oMp4ExtraInfo.sPreName,'/');
			pStr++;
			while(1)
			{
				nRet = cdr_read_mp4_ex(pStr,p->oMp4ExtraInfo.nCutLen+CDR_CUTMP4_EX_TIME,MP4GCUTOUTPATH,(stream_out_cb)&cdr_mp4cut_finish,NULL);				
				if(nRet == 0) break;
				sleep(1);
			}
		}	

	}
	return 0;
}

int InitAccEncoder(Mp4Context *p)
{
	if(!p) return -1;

	p->oAudioCfg.nSampleRate = 16000;
	p->oAudioCfg.nChannels = 2;
	p->oAudioCfg.nPCMBitSize = 16;
	p->oAudioCfg.nInputSamples = 1024;
	p->oAudioCfg.nMaxOutputBytes = 0;

	return 0;
}


/*
根据文件名与时长创建一个新的mp4文件，
同时要指定其的与mp4相关的参数

除文件名与时长可设外其他为固定值

将与mp4相关的参数封装为一个结构pMp4Context 传给下一级mp4文件创建函数

*/
int cdr_create_mp4file(char *pFileName,int timelen)
{
	if(!pFileName || timelen <=0)	return -1;    
	int headle = -1;
    
	Mp4Context *pMp4Context = calloc(1,sizeof(Mp4Context));
    if(pMp4Context == NULL)return -1;
    
	strcpy(pMp4Context->sName,pFileName);
	pMp4Context->nlen = timelen/1000;   
	pMp4Context->nNeedAudio= 1;

	pMp4Context->oAudioCfg.nSampleRate = 16000;
	pMp4Context->oAudioCfg.nChannels = 2;
	pMp4Context->oAudioCfg.nPCMBitSize = 16;
	pMp4Context->oAudioCfg.nInputSamples = 1024;
	pMp4Context->oAudioCfg.nMaxOutputBytes = 0;
		
	pMp4Context->oVideoCfg.timeScale = 90000;
	pMp4Context->oVideoCfg.fps = 30;
	pMp4Context->oVideoCfg.width = 1920;
	pMp4Context->oVideoCfg.height = 1080;
    
	pMp4Context->spslen = recspslen;
	memcpy(pMp4Context->sps,recspsdata,recspslen);
	pMp4Context->ppslen = recppslen;
	memcpy(pMp4Context->pps,recppsdata,recppslen);
    
	pMp4Context->outcb = (newmp4_out_cb)&record_mp4_finish;
    
	headle = cdr_mp4_create(pMp4Context);	//创建mp4文件及句柄
    
	//free(pMp4Context);
	//pMp4Context = NULL;
	return headle;
	
}

//create mp4 file 
/*
指定文件名，指定时长

*/
int CreateMp4File_ex(char *pFileName,int timelen)
{
	if(pFileName == NULL)		return -1;

	printf("%s %s len %d\n",__FUNCTION__,pFileName,timelen);
	update_sps_pps();

	g_mp4handle = cdr_create_mp4file(pFileName,timelen);
	if(g_mp4handle <= 0)
		return -1;
    
	HI_S32 s32Ret = 0;
	int i = 0;

    static int s_RecvencChn = 0,s_RecvencFd=0; 
	static int s_RecAencChn = 0,s_RecAencFd=0; 	
	static int s_maxFd = 0;
	fd_set read_fds;
    
	VENC_STREAM_S stVStream;
	AUDIO_STREAM_S stAStream;
    VENC_CHN_STAT_S stStat;

	s_RecvencChn = VENC_REC_CHN;
	s_RecAencChn = 0;

	s_RecvencFd  = HI_MPI_VENC_GetFd(s_RecvencChn);
	s_RecAencFd = HI_MPI_AENC_GetFd(s_RecAencChn);
	
	s_maxFd   = s_maxFd > s_RecvencFd ? s_maxFd:s_RecvencFd;
	s_maxFd   = s_maxFd > s_RecAencFd ? s_maxFd:s_RecAencFd;
	s_maxFd = s_maxFd+1;
    
	int iFrameFlag = 0;
	int nMp4PrePicflag = 0;
		
	//struct sched_param param;
	struct timeval TimeoutVal;
    
	VENC_PACK_S *pstPack = NULL;	
	pstPack = (VENC_PACK_S *)malloc(sizeof(VENC_PACK_S) * 128);
	if (NULL == pstPack)
	{
		pstPack = NULL;
		return -1;
	}	

	MP4_FRAME pDataFrame;
	unsigned char *pAudioBuffer = NULL;		

	while(cdr_mp4_checklen(g_mp4handle) > 0)	
	{
		FD_ZERO( &read_fds );
		FD_SET( s_RecvencFd, &read_fds);//对应1080p的通道
		FD_SET( s_RecAencFd, &read_fds);//对应音频的通道
		
		TimeoutVal.tv_sec  = 2;
		TimeoutVal.tv_usec = 0;
		s32Ret = select( s_maxFd, &read_fds, NULL, NULL, &TimeoutVal );
		if (s32Ret <= 0)
		{
			printf("%s select failed!\n",__FUNCTION__);
			sleep(1);
			continue;
		}
	
		//Select sucess.
		if (FD_ISSET( s_RecvencFd, &read_fds ))
		{
			s32Ret = HI_MPI_VENC_Query( s_RecvencChn, &stStat );
			if (HI_SUCCESS != s32Ret)
			{
				printf("HI_MPI_VENC_Query chn[%d] failed with %#x!\n", s_RecvencChn, s32Ret);
				continue;
			}
	
			stVStream.pstPack = pstPack;
			stVStream.u32PackCount = stStat.u32CurPacks;
			s32Ret = HI_MPI_VENC_GetStream( s_RecvencChn, &stVStream, HI_TRUE );
			if (HI_SUCCESS != s32Ret)
			{
				printf("HI_MPI_VENC_GetStream .. failed with %#x!\n", s32Ret);
				continue;
			}
	
	
			for (i = 0; i < stVStream.u32PackCount; i++)
			{
				memset(&pDataFrame,0x00,sizeof(MP4_FRAME));
				pDataFrame.streamType = MP4STREAM_VIDEO;
                
				if(stVStream.pstPack[i].DataType.enH264EType == H264E_NALU_PSLICE && iFrameFlag == 1)
				{					
					pDataFrame.nFrameType = stVStream.pstPack[i].DataType.enH264EType;
					pDataFrame.uPTS = stVStream.pstPack[i].u64PTS;
					pDataFrame.nlen = stVStream.pstPack[i].u32Len-stVStream.pstPack[i].u32Offset;	
					pDataFrame.pData= (unsigned char*)stVStream.pstPack[i].pu8Addr+stVStream.pstPack[i].u32Offset;		
					cdr_mp4_write_frame(g_mp4handle,&pDataFrame);
				}

				if(stVStream.pstPack[i].DataType.enH264EType == H264E_NALU_ISLICE)
				{					
					pDataFrame.nFrameType = stVStream.pstPack[i].DataType.enH264EType;
					pDataFrame.uPTS = stVStream.pstPack[i].u64PTS;
					pDataFrame.nlen = stVStream.pstPack[i].u32Len-stVStream.pstPack[i].u32Offset;	
					pDataFrame.pData= (unsigned char*)stVStream.pstPack[i].pu8Addr+stVStream.pstPack[i].u32Offset;		
					cdr_mp4_write_frame(g_mp4handle,&pDataFrame);	
					iFrameFlag = 1;			
				}
		
			}	
	
			s32Ret = HI_MPI_VENC_ReleaseStream(s_RecvencChn, &stVStream);
			if (HI_SUCCESS != s32Ret)
			{
				SAMPLE_PRT("HI_MPI_VENC_ReleaseStream chn[%d] .. failed with %#x!\n", s_RecvencChn, s32Ret);
				stVStream.pstPack = NULL;
				continue;
			}
		}


		//Get aac stream.
		if (FD_ISSET(s_RecAencFd, &read_fds))
		{
			memset(&stAStream,0x00,sizeof(AUDIO_STREAM_S));
			/* get stream from aenc chn */
			s32Ret = HI_MPI_AENC_GetStream(s_RecAencChn, &stAStream, HI_FALSE);
			if (HI_SUCCESS != s32Ret )
			{
				printf("%s: HI_MPI_AENC_GetStream(%d), failed with %#x!\n",\
					   __FUNCTION__, s_RecAencChn, s32Ret);
							continue;
			}
				
			if(iFrameFlag == 1 && stAStream.u32Len > 7)
			{	
				//memcpy(pAudioBuffer,stAStream.pStream,stAStream.u32Len);	
				pAudioBuffer = stAStream.pStream;
				
				memset(&pDataFrame,0x00,sizeof(MP4_FRAME));
				pDataFrame.streamType = MP4STREAM_AUDIO;
				pDataFrame.nFrameType = 0;
				pDataFrame.uPTS = stAStream.u64TimeStamp;
				//Remove adts header.
				pDataFrame.nlen = stAStream.u32Len-7;	
				pDataFrame.pData= pAudioBuffer+7;
				cdr_mp4_write_frame(g_mp4handle,&pDataFrame);
			}

			/* finally you must release the stream */
			s32Ret = HI_MPI_AENC_ReleaseStream(s_RecAencChn, &stAStream);
			if (HI_SUCCESS != s32Ret )
			{
				printf("%s: HI_MPI_AENC_ReleaseStream(%d), failed with %#x!\n",\
					   __FUNCTION__,s_RecAencChn, s32Ret);
				continue;
			}
		}

		if(nMp4PrePicflag == 0)
		{
			char prefullJpgName[256];
			sprintf(prefullJpgName, "%s_pre",pFileName);
			prefullJpgName[strlen(prefullJpgName)] = '\0';
			cdr_save_jpg(gsqcif_stPara.s32VencChn,prefullJpgName);
			nMp4PrePicflag = 1;
		}

		if(0 == cdr_get_powerflag()) break;		
	}

	/*写完了指定长度的mp4文件后 关闭mp4文件*/
	cdr_mp4_close(g_mp4handle);
		
	if(pstPack){
        free(pstPack);
        pstPack = NULL;
	}	
	return 0;
}

void cdr_video_index_file_synchronous(void)
{
	// 1.get first file of video.
	DIR *dir = NULL;
	struct dirent *ptr = NULL;
	dir = opendir("/mnt/mmc/VIDEO/");	
	if(dir == NULL)
	{
		printf("%s cann't open dir :/mnt/mmc/VIDEO/ \n",__FUNCTION__);
		return;
	}
	
	char chFirstName[256];
	//这儿需要初始化为FF,保证第一个文件
	memset(chFirstName,0xFF,sizeof(chFirstName));
	while ((ptr = readdir(dir)) != NULL)
	{
		if (ptr->d_type != 8)	continue;
		if(strcmp(ptr->d_name,chFirstName) < 0)
		{
			memset(chFirstName,0x00,256);
			//printf("%s \n",ptr->d_name);			
			snprintf(chFirstName, 256, "%s",ptr->d_name);		
		}	
	}
	closedir(dir);
	dir = NULL;

	//没有mp4文件.
	if(chFirstName[0] == 0xFF)
	{
		cdr_system("rm -rf /mnt/mmc/INDEX/*");
		sync();
		return ;
	}
	
	//printf("%s,%s\n",__FUNCTION__,chFirstName);		
	// 2. rm index pic before first video file.
	dir = opendir("/mnt/mmc/INDEX/");
	if(dir == NULL)
	{
		printf("%s cann't open dir :/mnt/mmc/INDEX/ \n",__FUNCTION__);
		return;
	}
	
	char chIndexTmp[256] = {0};
	while ((ptr = readdir(dir)) != NULL)
	{
		if (ptr->d_type != 8)	continue;

		//printf("%s < %s\n",ptr->d_name,chFirstName);
		if(strcmp(ptr->d_name,chFirstName) < 0)
		{
			//printf("rm %s 99999999999999999999\n",ptr->d_name);
			memset(chIndexTmp,0x00,256);
			snprintf(chIndexTmp, 256, "/mnt/mmc/INDEX/%s",ptr->d_name);	
			remove(chIndexTmp);
		}	
	}
    closedir(dir);
    dir = NULL;
    sync();
}


#if(0)
int cdr_live_thread_pro(void)
{
	int nSize = 0;
	sVAFrameInfo vaFrame;		
	char *pStremData = NULL;
	HI_S32 s32Ret = 0;
	static int s_LivevencChn = 0,s_LivevencFd=0; 

	static int s_maxFd = 0;
	fd_set read_fds;

	VENC_STREAM_S stVStream;
	VENC_CHN_STAT_S stStat;

	s_LivevencChn = VENC_LIVE_CHN;
	s_LivevencFd  = HI_MPI_VENC_GetFd(s_LivevencChn);	
	s_maxFd   = s_maxFd > s_LivevencFd ? s_maxFd:s_LivevencFd;
	s_maxFd = s_maxFd+1;

	static int iFrameFlag = 0;
		
	//struct sched_param param;
	struct timeval TimeoutVal;

	VENC_PACK_S *pstPack = NULL;	
	pstPack = (VENC_PACK_S *)malloc(sizeof(VENC_PACK_S) * 128);
	if (NULL == pstPack)
	{
		return -1;
	}	
	int i;

	while(1)	
	{
		if(g_nLivePlayFlag != 1)
		{
			sleep(1);
			iFrameFlag = 0;
			continue;
		}
		
		FD_ZERO( &read_fds );
		FD_SET( s_LivevencFd, &read_fds);
		
		TimeoutVal.tv_sec  = 2;
		TimeoutVal.tv_usec = 0;
		s32Ret = select( s_maxFd, &read_fds, NULL, NULL, &TimeoutVal );
		if (s32Ret <= 0)
		{
			printf("%s select failed!\n",__FUNCTION__);
			sleep(1);
			continue;
		}

		//Live stream
		if (FD_ISSET( s_LivevencFd, &read_fds ))
		{
			s32Ret = HI_MPI_VENC_Query( s_LivevencChn, &stStat );
			if (HI_SUCCESS != s32Ret)
			{
				printf("HI_MPI_VENC_Query chn[%d] failed with %#x!\n", s_LivevencChn, s32Ret);
				continue;
			}

			stVStream.pstPack = pstPack;
			stVStream.u32PackCount = stStat.u32CurPacks;
			s32Ret = HI_MPI_VENC_GetStream( s_LivevencChn, &stVStream, HI_TRUE );
			if (HI_SUCCESS != s32Ret)
			{
				printf("HI_MPI_VENC_GetStream .. failed with %#x!\n", s32Ret);
				continue;
			}

			for (i = 0; i < stVStream.u32PackCount; i++)
			{
				//暂时去掉SEI帧
				//if(stVStream.pstPack[i].DataType.enH264EType == H264E_NALU_SEI) continue;	

				//从IDR帧开始发 SPS PPS SEI Islice
				if(iFrameFlag == 0 && stVStream.pstPack[i].DataType.enH264EType != H264E_NALU_SPS)
					continue;	

				iFrameFlag = 1;
				
				cdr_stream_callback _fcallbak = g_fun_callbak[CDR_LIVE_TYPE];	

				//if(iFrameFlag == 0 && stVStream.pstPack[i].DataType.enH264EType != H264E_NALU_SPS)
				//	continue;		
				
				pStremData = (char*)(stVStream.pstPack[i].pu8Addr+stVStream.pstPack[i].u32Offset);
				nSize = stVStream.pstPack[i].u32Len-stVStream.pstPack[i].u32Offset;

				memset(&vaFrame,0x00,sizeof(sVAFrameInfo));
				vaFrame.isIframe =stVStream.pstPack[i].DataType.enH264EType;
				vaFrame.u64PTS = stVStream.pstPack[i].u64PTS;	
#if 0
				static FILE *fp = NULL;
				static int count = 0;
				if(fp == NULL && count == 0)
				{
					fp = fopen("live.264", "w+");
				}

				if(count < 1000)
				{
					fwrite(pStremData,nSize,1,fp);
					
				}
				if(count == 1000)
				{
					fclose(fp);
				}
				else
					count++;
	
#endif
		
				if(pStremData && nSize > 4 && _fcallbak != NULL)
				{
					_fcallbak(STREAM_VIDEO,pStremData,nSize,vaFrame);	
				}
			}	

			s32Ret = HI_MPI_VENC_ReleaseStream(s_LivevencChn, &stVStream);
			if (HI_SUCCESS != s32Ret)
			{
				SAMPLE_PRT("HI_MPI_VENC_ReleaseStream chn[%d] .. failed with %#x!\n", s_LivevencChn, s32Ret);
				stVStream.pstPack = NULL;
				continue;
			}
		}		
	}
	if(pstPack){
	    free(pstPack);
	    pstPack = NULL;
	}
	return 0;
}

#else
extern unsigned char g_ucRealtimePushAVFlag;

int cdr_live_thread_pro(void)
{
	int nSize = 0;
	sVAFrameInfo vaFrame;		
	char *pStremData = NULL;
	HI_S32 s32Ret = 0;
	static int s_LivevencChn = 0,s_LivevencFd=0; 

	static int s_maxFd = 0;
	fd_set read_fds;

	VENC_STREAM_S stVStream;
	VENC_CHN_STAT_S stStat;

	s_LivevencChn = VENC_LIVE_CHN;
	s_LivevencFd  = HI_MPI_VENC_GetFd(s_LivevencChn);	
	s_maxFd   = s_maxFd > s_LivevencFd ? s_maxFd:s_LivevencFd;
	s_maxFd = s_maxFd+1;

	static int iFrameFlag = 0;
		
	//struct sched_param param;
	struct timeval TimeoutVal;

	VENC_PACK_S *pstPack = NULL;	
	pstPack = (VENC_PACK_S *)malloc(sizeof(VENC_PACK_S) * 128);
	if (NULL == pstPack)
	{
		return -1;
	}	
	int i;

    //cdr_stream_callback _fcallbak = g_fun_callbak[CDR_LIVE_TYPE];	
    
	while(1){     
		if((g_nLivePlayFlag != 1) &&(g_ucRealtimePushAVFlag != 0x01))
		{
			sleep(1);
			iFrameFlag = 0;
			continue;
		}
		
		FD_ZERO( &read_fds );
		FD_SET( s_LivevencFd, &read_fds);
		
		TimeoutVal.tv_sec  = 2;
		TimeoutVal.tv_usec = 0;
		s32Ret = select( s_maxFd, &read_fds, NULL, NULL, &TimeoutVal );
		if (s32Ret <= 0)
		{
			printf("%s select failed!\n",__FUNCTION__);
			sleep(1);
			continue;
		}

		if (FD_ISSET( s_LivevencFd, &read_fds ))
		{
			s32Ret = HI_MPI_VENC_Query( s_LivevencChn, &stStat );
			if (HI_SUCCESS != s32Ret)
			{
				printf("HI_MPI_VENC_Query chn[%d] failed with %#x!\n", s_LivevencChn, s32Ret);
				continue;
			}

			stVStream.pstPack = pstPack;
			stVStream.u32PackCount = stStat.u32CurPacks;
			s32Ret = HI_MPI_VENC_GetStream( s_LivevencChn, &stVStream, HI_TRUE );
			if (HI_SUCCESS != s32Ret)
			{
				printf("HI_MPI_VENC_GetStream .. failed with %#x!\n", s32Ret);
				continue;
			}

			for (i = 0; i < stVStream.u32PackCount; i++)
			{

				if(iFrameFlag == 0 && stVStream.pstPack[i].DataType.enH264EType != H264E_NALU_SPS) continue;	
				iFrameFlag = 1;
				
				cdr_stream_callback _fcallbak_live = g_fun_callbak[CDR_LIVE_TYPE];	
                cdr_stream_callback _fcallbak_push = g_fun_callbak[CDR_PHUSH_TYPE];	

                pStremData = (char*)(stVStream.pstPack[i].pu8Addr+stVStream.pstPack[i].u32Offset);
				nSize = stVStream.pstPack[i].u32Len-stVStream.pstPack[i].u32Offset;
				memset(&vaFrame,0x00,sizeof(sVAFrameInfo));
				vaFrame.isIframe =stVStream.pstPack[i].DataType.enH264EType;
				vaFrame.u64PTS = stVStream.pstPack[i].u64PTS;	
		
				if(pStremData && nSize > 4 && _fcallbak_live != NULL)
				{
					_fcallbak_live(STREAM_VIDEO,pStremData,nSize,vaFrame);	
				}
				
                if(g_ucRealtimePushAVFlag == 1 && pStremData && nSize > 4 && _fcallbak_push != NULL)
				{
					_fcallbak_push(STREAM_VIDEO,pStremData,nSize,vaFrame);	
				}
			}	

			s32Ret = HI_MPI_VENC_ReleaseStream(s_LivevencChn, &stVStream);
			if (HI_SUCCESS != s32Ret)
			{
				SAMPLE_PRT("HI_MPI_VENC_ReleaseStream chn[%d] .. failed with %#x!\n", s_LivevencChn, s32Ret);
				stVStream.pstPack = NULL;
				continue;
			}
		}		
	}
	if(pstPack){
	    free(pstPack);
	    pstPack = NULL;
	}
	return 0;
}
#endif


//create live thread
int cdr_create_live_thread(void)
{
	int ret = -1;
	
	pthread_t tfid;
	ret = pthread_create(&tfid, NULL, (void *)cdr_live_thread_pro, NULL);
	if (0 != ret)
	{
		DEBUG_PRT("cdr_create_live_thread failed!\n");
		return -1;
	}
	pthread_detach(tfid);
	return 0;
}

