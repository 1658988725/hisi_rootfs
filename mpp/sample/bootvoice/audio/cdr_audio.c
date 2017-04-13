/******************************************************************************
  A simple program of Hisilicon HI3518 audio input/output/encoder/decoder implementation.
  Copyright (C), 2010-2012, Hisilicon Tech. Co., Ltd.
 ******************************************************************************
    Modification:  2012-7-3 Created
******************************************************************************/

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
#include "acodec.h"
//#include "aacenc.h"
//#include "aacdec.h"
#include "cdr_audio.h"
#include "cdr_config.h"


extern int playflag;

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


#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */

#define SAMPLE_AUDIO_PTNUMPERFRM   1024//320

static PAYLOAD_TYPE_E gs_enPayloadType = PT_AAC;

static HI_BOOL gs_bMicIn = HI_FALSE;

static HI_BOOL gs_bAiAnr = HI_FALSE;
static HI_BOOL gs_bAioReSample = HI_FALSE;
static HI_BOOL gs_bUserGetMode = HI_FALSE;
static AUDIO_RESAMPLE_ATTR_S *gs_pstAiReSmpAttr = NULL;
static AUDIO_RESAMPLE_ATTR_S *gs_pstAoReSmpAttr = NULL;

static AUDIO_SAMPLE_RATE_E enInSampleRate  = AUDIO_SAMPLE_RATE_BUTT;
static AUDIO_SAMPLE_RATE_E enOutSampleRate = AUDIO_SAMPLE_RATE_BUTT;
static HI_U32 u32AencPtNumPerFrm = 0;
/* 0: close, 1: open*/
static HI_U32 u32AiVqeType = 1;  
/* 0: close, 1: open*/
static HI_U32 u32AoVqeType = 1;  


#define CDR_AUDIO_FILE 0

typedef struct tagSAMPLE_AENC_S
{
    HI_BOOL bStart;
    pthread_t stAencPid;
    HI_S32  AeChn;
    HI_S32  AdChn;
    FILE    *pfd;
    HI_BOOL bSendAdChn;
} SAMPLE_AENC_S;


typedef struct sAudioFileList
{
	struct sAudioFileList *pNext;
	char audioFile[256];
}AudioFileList;

typedef struct tagADEC_S
{
    HI_BOOL bStart;
    HI_S32 AdChn; 
	int nAudioPlayFlag;
    AudioFileList *stPlayList;
    pthread_t stAdPid;
} ADEC_S;

static ADEC_S gs_stAdec;

static SAMPLE_AENC_S gs_stSampleAenc[AENC_MAX_CHN_NUM];


#define SAMPLE_DBG(s32Ret)\
do{\
    printf("s32Ret=%#x,fuc:%s,line:%d\n", s32Ret, __FUNCTION__, __LINE__);\
}while(0)

#if 1
/******************************************************************************
* function : PT Number to String
******************************************************************************/
static char* SAMPLE_AUDIO_Pt2Str(PAYLOAD_TYPE_E enType)
{
    if (PT_G711A == enType)  
    {
        return "g711a";
    }
    else if (PT_G711U == enType)  
    {
        return "g711u";
    }
    else if (PT_ADPCMA == enType)  
    {
        return "adpcm";
    }
    else if (PT_G726 == enType) 
    {
        return "g726";
    }
    else if (PT_LPCM == enType)  
    {
        return "pcm";
    }
	else if(PT_AAC == enType)
	{
		return "aac";
	}
    else 
    {
        return "data";
    }
}

/******************************************************************************
* function : Open Aenc File
******************************************************************************/
static FILE * SAMPLE_AUDIO_OpenAencFile(AENC_CHN AeChn, PAYLOAD_TYPE_E enType)
{
    FILE* pfd;
    HI_CHAR aszFileName[FILE_NAME_LEN];

    /* create file for save stream*/
    snprintf(aszFileName, FILE_NAME_LEN, "audio_chn%d.%s", AeChn, SAMPLE_AUDIO_Pt2Str(enType));
    pfd = fopen(aszFileName, "w+");
    if (NULL == pfd)
    {
        printf("%s: open file %s failed\n", __FUNCTION__, aszFileName);
        return NULL;
    }
    printf("open stream file:\"%s\" for aenc ok\n", aszFileName);
    return pfd;
}

/******************************************************************************
* function : Open Adec File
******************************************************************************/
static FILE *SAMPLE_AUDIO_OpenAdecFile(ADEC_CHN AdChn, PAYLOAD_TYPE_E enType)
{
    FILE* pfd;
    HI_CHAR aszFileName[FILE_NAME_LEN];

    /* create file for save stream*/
    snprintf(aszFileName, FILE_NAME_LEN ,"audio_chn%d.%s", AdChn, SAMPLE_AUDIO_Pt2Str(enType));
    pfd = fopen(aszFileName, "rb");
    if (NULL == pfd)
    {
        printf("%s: open file %s failed\n", __FUNCTION__, aszFileName);
        return NULL;
    }
    printf("open stream file:\"%s\" for adec ok\n", aszFileName);
    return pfd;
}

/******************************************************************************
* function : Ai -> Aenc -> file
*                       -> Adec -> Ao
******************************************************************************/
HI_S32 cdr_AUDIO_AiAencAo(HI_VOID)
{
    HI_S32 i, s32Ret;
    AUDIO_DEV   AiDev = SAMPLE_AUDIO_AI_DEV;
    AI_CHN      AiChn;
    AUDIO_DEV   AoDev = SAMPLE_AUDIO_AO_DEV;
    AO_CHN      AoChn = 0;
    ADEC_CHN    AdChn = 0;
    HI_S32      s32AiChnCnt;
	HI_S32      s32AoChnCnt;
    HI_S32      s32AencChnCnt;
    AENC_CHN    AeChn;
    //HI_BOOL     bSendAdec = HI_FALSE;
    FILE        *pfd = NULL;
    AIO_ATTR_S stAioAttr;
	AI_VQE_CONFIG_S stAiVqeAttr;	
	HI_VOID     *pAiVqeAttr = NULL;

    stAioAttr.enSamplerate   = AUDIO_SAMPLE_RATE_8000;
    stAioAttr.enBitwidth     = AUDIO_BIT_WIDTH_16;
    stAioAttr.enWorkmode     = AIO_MODE_I2S_MASTER;
    stAioAttr.enSoundmode    = AUDIO_SOUND_MODE_MONO;
    stAioAttr.u32EXFlag      = 0;
    stAioAttr.u32FrmNum      = 30;
    stAioAttr.u32PtNumPerFrm = SAMPLE_AUDIO_PTNUMPERFRM;
    stAioAttr.u32ChnCnt      = 1;
    stAioAttr.u32ClkSel      = 0;

    gs_bAioReSample = HI_FALSE;
    enInSampleRate  = AUDIO_SAMPLE_RATE_BUTT;
    enOutSampleRate = AUDIO_SAMPLE_RATE_BUTT;
	u32AencPtNumPerFrm = stAioAttr.u32PtNumPerFrm;

 
	if (1 == u32AiVqeType)
    {
	    stAiVqeAttr.s32WorkSampleRate    = AUDIO_SAMPLE_RATE_8000;
    	stAiVqeAttr.s32FrameSample       = SAMPLE_AUDIO_PTNUMPERFRM;
    	stAiVqeAttr.enWorkstate          = VQE_WORKSTATE_NOISY;
    	stAiVqeAttr.bAecOpen             = HI_TRUE;
    	stAiVqeAttr.stAecCfg.bUsrMode    = HI_FALSE;
    	stAiVqeAttr.stAecCfg.s8CngMode   = 0;
    	stAiVqeAttr.bAgcOpen             = HI_TRUE;
    	stAiVqeAttr.stAgcCfg.bUsrMode    = HI_FALSE;
    	stAiVqeAttr.bAnrOpen             = HI_TRUE;
    	stAiVqeAttr.stAnrCfg.bUsrMode    = HI_FALSE;
    	stAiVqeAttr.bHpfOpen             = HI_TRUE;
    	stAiVqeAttr.stHpfCfg.bUsrMode    = HI_TRUE;
    	stAiVqeAttr.stHpfCfg.enHpfFreq   = AUDIO_HPF_FREQ_150;
    	stAiVqeAttr.bRnrOpen             = HI_FALSE;
    	stAiVqeAttr.bEqOpen              = HI_FALSE;
    	stAiVqeAttr.bHdrOpen             = HI_FALSE;
		
		pAiVqeAttr = (HI_VOID *)&stAiVqeAttr;
    }	
	else
	{
		pAiVqeAttr = HI_NULL;
	}

		
    /********************************************
      step 1: config audio codec
    ********************************************/
    s32Ret = SAMPLE_COMM_AUDIO_CfgAcodec(&stAioAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_DBG(s32Ret);
        return HI_FAILURE;
    }

#if 0

    /********************************************
      step 2: start Ai
    ********************************************/
    s32AiChnCnt = stAioAttr.u32ChnCnt; 
    s32Ret = SAMPLE_COMM_AUDIO_StartAi(AiDev, s32AiChnCnt, &stAioAttr, enOutSampleRate, gs_bAioReSample, pAiVqeAttr,u32AiVqeType);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_DBG(s32Ret);
        return HI_FAILURE;
    }

    /********************************************
      step 3: start Aenc
    ********************************************/
    s32AencChnCnt = 1;
    s32Ret = SAMPLE_COMM_AUDIO_StartAenc(s32AencChnCnt, u32AencPtNumPerFrm, gs_enPayloadType);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_DBG(s32Ret);
        return HI_FAILURE;
    }

    /********************************************
      step 4: Aenc bind Ai Chn
    ********************************************/
    for (i=0; i<s32AencChnCnt; i++)
    {
        AeChn = i;
        AiChn = i;

        if (HI_TRUE == gs_bUserGetMode)
        {
            s32Ret = SAMPLE_COMM_AUDIO_CreatTrdAiAenc(AiDev, AiChn, AeChn);
            if (s32Ret != HI_SUCCESS)
            {
                SAMPLE_DBG(s32Ret);
                return HI_FAILURE;
            }
        }
        else
        {        
            s32Ret = SAMPLE_COMM_AUDIO_AencBindAi(AiDev, AiChn, AeChn);
            if (s32Ret != HI_SUCCESS)
            {
                SAMPLE_DBG(s32Ret);
                return s32Ret;
            }
        }
        printf("Ai(%d,%d) bind to AencChn:%d ok!\n",AiDev , AiChn, AeChn);
    }
#endif

#if 1
    /********************************************
      step 5: start Adec & Ao. ( if you want )
    ********************************************/
    stAioAttr.enSamplerate   = AUDIO_SAMPLE_RATE_8000;
    stAioAttr.enBitwidth     = AUDIO_BIT_WIDTH_16;
    stAioAttr.enWorkmode     = AIO_MODE_I2S_MASTER;
    stAioAttr.enSoundmode    = AUDIO_SOUND_MODE_MONO;
    stAioAttr.u32EXFlag      = 0;
    stAioAttr.u32FrmNum      = 30;
    stAioAttr.u32PtNumPerFrm = 320;
    stAioAttr.u32ChnCnt      = 1;
    stAioAttr.u32ClkSel      = 0;

    s32Ret = SAMPLE_COMM_AUDIO_StartAdec(AdChn, PT_LPCM);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_DBG(s32Ret);
        return HI_FAILURE;
    }

	s32AoChnCnt = stAioAttr.u32ChnCnt;
    s32Ret = SAMPLE_COMM_AUDIO_StartAo(AoDev, s32AoChnCnt, &stAioAttr, AUDIO_SAMPLE_RATE_BUTT, AUDIO_SAMPLE_RATE_BUTT, NULL, 0);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_DBG(s32Ret);
        return HI_FAILURE;
    }

    s32Ret = SAMPLE_COMM_AUDIO_AoBindAdec(AoDev, AoChn, AdChn);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_DBG(s32Ret);
        return HI_FAILURE;
    }

    printf("bind adec:%d to ao(%d,%d) ok \n", AdChn, AoDev, AoChn);
#if 1
	s32Ret = cdr_audio_play_thread_init();
	if (s32Ret != HI_SUCCESS)
	{
		SAMPLE_DBG(s32Ret);
		return HI_FAILURE;
	}
#endif

#endif
 
    return HI_SUCCESS;
}
HI_VOID SAMPLE_AUDIO_Usage(HI_VOID)
{
    printf("\n\n/************************************/\n");
    printf("please choose the case which you want to run:\n");
    printf("\t0:  start AI to AO loop\n");
    printf("\t1:  send audio frame to AENC channel from AI, save them\n");
    printf("\t2:  read audio stream from file, decode and send AO\n");
    printf("\t3:  start AI(AEC/ANR/ALC process), then send to AO\n");
    printf("\tq:  quit whole audio sample\n\n");
    printf("sample command:");
}

/******************************************************************************
* function : to process abnormal case
******************************************************************************/
void SAMPLE_AUDIO_HandleSig(HI_S32 signo)
{
    if (SIGINT == signo || SIGTERM == signo)
    {

        SAMPLE_COMM_AUDIO_DestoryAllTrd();
        SAMPLE_COMM_SYS_Exit();
        printf("\033[0;31mprogram exit abnormally!\033[0;39m\n");
    }

    exit(0);
}

#endif

int Set_VolumeCtrl(int iValueIndex)
{

    g_cdr_systemconfig.volume = iValueIndex;
    
     //理论范围[-121, 6] 
     //测试有效范围  [40-93, 99-93]
    if(iValueIndex != 0x00){
       iValueIndex = (iValueIndex+1)/2 + 49 ;// [0 99]
       iValueIndex = iValueIndex - 93;//[-93 6]
    }else{
       iValueIndex = -121;
    }
    
    int   fdAcodec = open(ACODEC_FILE, O_RDWR);
    if (fdAcodec < 0)
    {
        printf("%s: can't open Acodec,%s\n", __FUNCTION__, ACODEC_FILE);
        return HI_FAILURE;
    }
 
    	if (ioctl(fdAcodec, ACODEC_SET_OUTPUT_VOL, &iValueIndex))
	{
		printf("ioctl err!\n");
	} 
    
    close(fdAcodec);

    //Set_AI_MuteVolume(0x01);
    
   return 0;
}


//参数ai ucVolMuteFlag 0表示静音，为1开启音频
int Set_AI_MuteVolume(unsigned char ucVolMuteFlag)
{
    
	ACODEC_VOL_CTRL vol_ctrl;
    
    int   fdAcodec = open(ACODEC_FILE, O_RDWR);
    if (fdAcodec < 0)
    {
        printf("%s: can't open Acodec,%s\n", __FUNCTION__, ACODEC_FILE);
        return HI_FAILURE;
    }
    
    vol_ctrl.vol_ctrl_mute = 0x0;
    if(ucVolMuteFlag == 0x00)	vol_ctrl.vol_ctrl_mute = 0x1;//静音
	vol_ctrl.vol_ctrl = 0x00;
	if (ioctl(fdAcodec, ACODEC_SET_ADCL_VOL, &vol_ctrl))//左声道输出音量控制
	{
		printf("ioctl err!\n");
	}
	
	if (ioctl(fdAcodec, ACODEC_SET_ADCR_VOL, &vol_ctrl))
	{
		printf("ioctl err!\n");
	}

    close(fdAcodec);
    return 0;
}

int Set_VolumeRecordingSensitivity(int iIndex)
{
    int fdAcodec = open(ACODEC_FILE, O_RDWR);
    if (fdAcodec < 0)
    {
        printf("%s: can't open Acodec,%s\n", __FUNCTION__, ACODEC_FILE);
        return HI_FAILURE;
    }
    
    g_cdr_systemconfig.volumeRecordingSensitivity = iIndex;		 

	//Hi3518EV200/Hi3519V100 的模拟增益范围为[0, 16]，其中 0 到 15 按 2db 递增，0
	//对应 0db，15 对应最大增益 30db，而 16 对应的是-1.5db。
	
	unsigned int gain_mic = 16;
     
    if (ioctl(fdAcodec, ACODEC_SET_GAIN_MICL, &gain_mic))
	{
		printf("%s %d ioctl err!\n",__FUNCTION__,__LINE__);
	}

	if (ioctl(fdAcodec, ACODEC_SET_GAIN_MICR, &gain_mic))
	{
		printf("%s %d ioctl err!\n",__FUNCTION__,__LINE__);
	} 

	unsigned int adc_hpf;
	adc_hpf = 0x1;
	if (ioctl(fdAcodec, ACODEC_SET_ADC_HP_FILTER, &adc_hpf))
	{
		printf("%s %d ioctl err!\n",__FUNCTION__,__LINE__);
	}


    close(fdAcodec);

	gain_mic = g_cdr_systemconfig.volumeRecordingSensitivity*5;
	if(gain_mic == 0x00)Set_AI_MuteVolume(0x00);
	else Set_AI_MuteVolume(0x01);
  
	return 0;
}

/******************************************************************************
* function : main
******************************************************************************/
HI_S32 cdr_audioInit(void)
{
    int ret = 0;
    signal(SIGINT, SAMPLE_AUDIO_HandleSig);
    signal(SIGTERM, SAMPLE_AUDIO_HandleSig);

	VB_CONF_S stVbConf;    
	memset(&stVbConf,0,sizeof(VB_CONF_S));	
	SAMPLE_COMM_SYS_Init(&stVbConf);
	

    ret = cdr_AUDIO_AiAencAo();

    printf("g_cdr_systemconfig.volume:%d\n",g_cdr_systemconfig.volume);
    Set_VolumeCtrl(g_cdr_systemconfig.volume);
    //Set_VolumeRecordingSensitivity(g_cdr_systemconfig.volumeRecordingSensitivity);
    //sleep(2);

	return ret;   
}


HI_S32 cdr_audio_release(void)
{
	//SAMPLE_COMM_SYS_Exit();
	SAMPLE_COMM_AUDIO_DestoryAllTrd();
	SAMPLE_COMM_SYS_Exit();
	return HI_SUCCESS;
}


/******************************************************************************
* function : file -> Adec -> Ao
******************************************************************************/
HI_S32 _audio_play_file_pro()
{
	//printf("%s %d\n",__FUNCTION__,__LINE__);
	HI_S32 s32Ret;
    AUDIO_STREAM_S stAudioStream;    
    HI_U32 u32Len = 640;
    HI_U32 u32ReadLen;
    HI_S32 s32AdecChn = gs_stAdec.AdChn;
    HI_U8 *pu8AudioStream = NULL;
    AudioFileList *pList = NULL;
	FILE *pfd = NULL;
	//printf("%s %d\n",__FUNCTION__,__LINE__);
    pu8AudioStream = (HI_U8*)malloc(sizeof(HI_U8)*MAX_AUDIO_STREAM_LEN);
    if (NULL == pu8AudioStream)
    {
        printf("%s: malloc failed!\n", __FUNCTION__);
        return -1;
    }
	//printf("%s %d\n",__FUNCTION__,__LINE__);	
	while (HI_TRUE == gs_stAdec.bStart)
	{
		//printf("%s %d\n",__FUNCTION__,__LINE__);
		pList = gs_stAdec.stPlayList;
		if(pList == NULL)
		{
			usleep(300000);
			continue;
		}
		//GPIO6_1 功放PA引脚
		HI_MPI_SYS_SetReg(0x200F00C4,0x00);
		cdr_set_gpio_mode(0x201A0400,1,1);
		HI_MPI_SYS_SetReg(0x201A0008,0xFF);

		pfd = fopen(pList->audioFile,"rb");
		if(pfd == NULL)
		{
			//打开音频文件失败.需要更新为下一个音频文件.
			pList = gs_stAdec.stPlayList->pNext;
			free(gs_stAdec.stPlayList);
			gs_stAdec.stPlayList = pList;
			continue;
		}
		while(1)
		{
			/* read from file */
			stAudioStream.pStream = pu8AudioStream;
			u32ReadLen = fread(stAudioStream.pStream, 1, u32Len, pfd);
			if (u32ReadLen <= 0)
			{

				pList = gs_stAdec.stPlayList->pNext;
				free(gs_stAdec.stPlayList);
				gs_stAdec.stPlayList = pList;
				fclose(pfd);
				pfd = NULL;
				
				//列表里面没有音频文件了.
				if(gs_stAdec.stPlayList == NULL)
				{
			        s32Ret = HI_MPI_ADEC_SendEndOfStream(s32AdecChn, HI_FALSE);
			        if (HI_SUCCESS != s32Ret)
			        {
			            printf("%s: HI_MPI_ADEC_SendEndOfStream failed!\n", __FUNCTION__);
			        }
					sleep(2);	
					//或者关闭功放.
					HI_MPI_SYS_SetReg(0x201A0008,0x00);

					//推出系统
					playflag = 0;
				}					
				//继续播放下一个音频文件				
				 break;
			}

			/* here only demo adec streaming sending mode, but pack sending mode is commended */
			stAudioStream.u32Len = u32ReadLen;
			s32Ret = HI_MPI_ADEC_SendStream(s32AdecChn, &stAudioStream, HI_TRUE);
			if(HI_SUCCESS != s32Ret)
			{
			    printf("%s: HI_MPI_ADEC_SendStream(%d) failed with %#x!\n",\
			           __FUNCTION__, s32AdecChn, s32Ret);
			    break;
			}
		}
	}
    
    free(pu8AudioStream);
    pu8AudioStream = NULL;
    fclose(pfd);

    return HI_SUCCESS;
}

int cdr_audio_play_file(char *cAudioFile)
{	
	//printf("%s %s\n",__FUNCTION__,cAudioFile);
	
	AudioFileList *p,*pList = NULL;
	p = (AudioFileList*)malloc(sizeof(AudioFileList));
	if(p)
	{
		p->pNext = NULL;
		memset(p->audioFile,0x00,256);
		memcpy(p->audioFile,cAudioFile,strlen(cAudioFile));


		if(gs_stAdec.stPlayList == NULL)
		{
			HI_MPI_ADEC_ClearChnBuf(gs_stAdec.AdChn);
			gs_stAdec.stPlayList = p;
		}
		else
		{
			pList = gs_stAdec.stPlayList;
			while(pList->pNext)
				pList = pList->pNext;
			pList->pNext = p;	
		}
		return 0;
	}
	return -1;	
}


//eValue 是播放的附加文件.比如蓝牙开启关闭.FM频率数据等.
int cdr_play_audio(int type,int eValue)
{
	if(type == CDR_AUDIO_BOOTVOICE)	{
		cdr_audio_play_file("/home/audiofile/poweron.pcm");
	}
	else if(type == CDR_AUDIO_IMAGECAPUTER){
		cdr_audio_play_file("/home/audiofile/imagecapture.pcm");
	}
	else if(type == CDR_AUDIO_NOSD){
		cdr_audio_play_file("/home/audiofile/nosd.pcm");
	}
	else if(type == CDR_AUDIO_UPDATE){
		cdr_audio_play_file("/home/audiofile/systemupdate.pcm");
	}
	else if(type == CDR_AUDIO_AssociatedVideo){
		cdr_audio_play_file("/home/audiofile/photoassociatedvideo.pcm");
		if(eValue == 0x00)
			cdr_audio_play_file("/home/audiofile/close.pcm");
		else
			cdr_audio_play_file("/home/audiofile/open.pcm");
	}
	else if(type == CDR_AUDIO_FM && eValue > 0){
		
		cdr_audio_play_file("/home/audiofile/adjusttheradioto.pcm");

		int bai = eValue/1000;
		int shi = (eValue%1000)/100;
		int ge = (eValue%100)/10;
		char chTmp[256];
		//是否需要小数点后面的.
		int nNeedDot = eValue%10;
		if(bai>0)
		{
			cdr_audio_play_file("/home/audiofile/1.pcm");			
			cdr_audio_play_file("/home/audiofile/bai.pcm");	

			//105mHz
			if(ge>0)
				cdr_audio_play_file("/home/audiofile/0.pcm");	
		}
		
		if(shi >0)
		{
			sprintf(chTmp,"/home/audiofile/%d.pcm",shi);
			cdr_audio_play_file(chTmp); 
			cdr_audio_play_file("/home/audiofile/shi.pcm");		
		}

		if(ge>0)
		{
			//90 和100mHz都不需要播
			sprintf(chTmp,"/home/audiofile/%d.pcm",ge);
			cdr_audio_play_file(chTmp); 		
		}

		if(nNeedDot)
		{
			cdr_audio_play_file("/home/audiofile/dot.pcm"); 
			sprintf(chTmp,"/home/audiofile/%d.pcm",nNeedDot);
			cdr_audio_play_file(chTmp); 	
		}
		cdr_audio_play_file("/home/audiofile/mhz.pcm"); 	
		
	}
	else if(type == CDR_AUDIO_RESETSYSTEM){
		cdr_audio_play_file("/home/audiofile/reset.pcm");
	}
	else if(type == CDR_AUDIO_FORMATSD){
		cdr_audio_play_file("/home/audiofile/formatsd.pcm");
	}
	else if(type == CDR_AUDIO_SETTIMEOK){
		cdr_audio_play_file("/home/audiofile/settimeok.pcm");
	}
	else if(type == CDR_AUDIO_SYSTEMUPDATE){
		cdr_audio_play_file("/home/audiofile/systemupdate.pcm");
	}
	else if(type == CDR_AUDIO_LT8900PAIR){				
		if(eValue == 0x02)cdr_audio_play_file("/home/audiofile/startpairing.pcm");//蓝牙按键配对
	}
	else if(type == CDR_AUDIO_STARTREC){				
		cdr_audio_play_file("/home/audiofile/startrecord.pcm");//蓝牙按键配对
	}
	else if(type == CDR_AUDIO_USBOUT){				
		cdr_audio_play_file("/home/audiofile/usbout.pcm");//蓝牙按键配对
	}
	else if(type == CDR_AUDIO_DIDI){				
		cdr_audio_play_file("/home/audiofile/didi.pcm");//遥控强制录制视频hold状态
	}
}

int cdr_audio_play_thread_init(void)
{	
	gs_stAdec.stAdPid = 0;
	gs_stAdec.stPlayList = NULL;
	gs_stAdec.bStart = HI_TRUE;
	int status = pthread_create(&gs_stAdec.stAdPid, NULL, _audio_play_file_pro,NULL);
	if(status != 0)
	{
		printf("cdr_audio_play_thread_init pthread_create error\n");
	}
	pthread_detach(gs_stAdec.stAdPid);		
	return 0;	
}


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
