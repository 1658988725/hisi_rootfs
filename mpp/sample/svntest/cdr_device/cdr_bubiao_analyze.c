/************************************************************************	
** Filename: 	
** Description:  
** Author: 	xjl
** Create Date: 
** Version: 	v1.0
   Copyright(C) 2016 e-eye CO.LTD. ShenZhen <www.e-eye.cn>

*************************************************************************/
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <memory.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>  
#include "type.h"
#include "cdr_bubiao_analyze.h"
#include "cdr_upload.h"
#include "cdr_ftp_process.h"
#include "cdr_mpp.h"
#include "cdr_rtsp_push.h"
#include "rtsp_client.h"
#include "cdr_comm.h"
#include "cdr_count_file.h"
#include "cdr_bubiao.h"
#include "cdr_bubiao_log.h"
#include "cdr_bubiao_driver.h"
#include "queue_bag.h"

unsigned char s_UrlBuff[200] = {0};

unsigned char g_ucRealtimePushAVFlag = 0;
unsigned short usResponseSerialNm = 0;

unsigned char g_ucSubPackFlag = 0;//�ְ����ر�־ 1:Ҫ���зְ�����  0:�����зְ�����
unsigned char ucCurrentBagIndex = 0;
unsigned char g_ucBBProcessFlag = 0;

extern char g_cUrlBuff[200];
extern unsigned char g_ucPushTimerOutFlag;
extern int g_RtpSessionUdpSock;

//volatile 
sBBMsgArr g_sBBMsgArrBag;

extern int g_iPictMediaTatoilCnt;
extern int g_iAVMediaTatoilCnt;

/*
ʵʱ����Ƶ�������������Ľ���
��ʼ�ֽ�	�ֶ� 	�������� 	������Ҫ�� 
0 	������IP��ַ���� 	BYTE 	����n 
1 	������IP��ַ 	STRING 	ʵʱ����Ƶ������IP��ַ 
1+n 	����������Ƶͨ�������˿ںţ�TCP��	WORD 	ʵʱ����Ƶ�������˿ں� 
3+n 	����������Ƶͨ�������˿ںţ�UDP��	WORD 	ʵʱ����Ƶ�������˿ں� 
5+n 	����Ƶͨ���� 	BYTE 	��1��ʼ 
6+n 	�Ƿ�Я����Ƶ 	BYTE 	0:Я����1:��Я�� 
7+n 	��/��������־ 	BYTE 	0:��������1:������ 
8+n 	����/�رձ�־ 	BYTE 	0:�رգ�1:���� 
*/
int RealTimeAVTransProcess(sBBMsgArr sBBMsgArrPack,sBBMsgHeaderPack sBBMsgHeadPack,s_BuBiaoTatoilResult *sTatoiResultPackage)
{
  int i = 0;
  int j = 0;
  unsigned char *pSrcBuf = sBBMsgArrPack.ucMsgArr;
  
  unsigned char * ucHeadTemp = pSrcBuf;
  s_RealTimeAVBody sRealTimeAVBody;  
  memset(&sRealTimeAVBody,0,sizeof(s_RealTimeAVBody));
  
  i = sBBMsgHeadPack.iMsgHeadLen + 1 ;//��ʶ+��Ϣͷ�� 
  sRealTimeAVBody.ucServerIPAddrLen = pSrcBuf[i];
  i++;

  pSrcBuf+=i;
  for(j=0;j<sRealTimeAVBody.ucServerIPAddrLen;j++)
  {
     sRealTimeAVBody.usServerIPAddr[j] = *pSrcBuf++;
  }  
  sRealTimeAVBody.usServerIPAddr[sRealTimeAVBody.ucServerIPAddrLen] = '\0';

  printf("sRealTimeAVBody.usServerIPAddr:%s\n",sRealTimeAVBody.usServerIPAddr);
  
  pSrcBuf = ucHeadTemp;
  
  i+=sRealTimeAVBody.ucServerIPAddrLen;
  sRealTimeAVBody.usServerTCPListenPort = MAKEWORD(pSrcBuf[i],pSrcBuf[i+1]);

  i+=2;
  sRealTimeAVBody.usServerUDPListenPort = MAKEWORD(pSrcBuf[i],pSrcBuf[i+1]);
  
  i+=2;
  sRealTimeAVBody.ucAVChanlNm = pSrcBuf[i];//0x02 ��ֵ ������ǰ��
  
  i+=1;
  sRealTimeAVBody.ucMediumType = pSrcBuf[i];//0x01 av 0x02: picture

  i+=1;
  sRealTimeAVBody.ucStreamType = pSrcBuf[i];  

  i+=1;
  sRealTimeAVBody.ucSwitchFlag = pSrcBuf[i];  

  switch(sRealTimeAVBody.ucMediumType){
   case 0x00:break;/*ʵʱ����Ƶ*/
   
   case 0x01:/*ʵʱ��Ƶ�ϴ�*/
    if(g_ucRealtimePushAVFlag == 0x00){
        InitRtspPush(sRealTimeAVBody.usServerIPAddr);    
        //InitRtspPush("rtsp://eyesrc:158Hostm7d@120.76.194.244:1935/live/KK_1234567890123");    
        //InitRtspPush("rtsp://192.168.200.48:10554/and_123456_s.sdp");    
        memcpy(&g_sRealTimeAVBody,&sRealTimeAVBody,sizeof(s_RealTimeAVBody));
    }
    break;    
   case 0x02:/*�·���������ָ��ϴ����պ��ͼ��*/
    SetCapturePictureFlag(0x01);
    SetUrlBuff(sRealTimeAVBody.usServerIPAddr,(int)(sRealTimeAVBody.ucServerIPAddrLen+1));
    cdr_capture_jpg(0);  
    break;
  }
  
  switch(sRealTimeAVBody.ucSwitchFlag){//0:�رգ�1:���� 2:����
   case 0x00:
    if(g_ucRealtimePushAVFlag == 0x01){
       DoTearDown((const char*)sRealTimeAVBody.usServerIPAddr);
       if(g_RtpSessionUdpSock != -1)close(g_RtpSessionUdpSock);//�費��Ҫclose��ȷ�ϲ���
       g_ucRealtimePushAVFlag = 0x00;
    }
    if(sRealTimeAVBody.ucMediumType == 0x02){
       sTatoiResultPackage->sKakaAckPack.ucResult = 0x00;
    }
    break;
   case 0x01:
    if(sRealTimeAVBody.ucMediumType == 0x01) {
        g_ucPushTimerOutFlag = 0;
    }
    break;
   case 0x02:
    if(sRealTimeAVBody.ucMediumType == 0x01) {//���֣���λ��ʱ��־
        g_ucPushTimerOutFlag = 0;
    }
    break;
  }  
  //ShowRealTimeAVAttr(sRealTimeAVBody);     
  return 0;
}

int RealTimeAVTransControl(sBBMsgArr sBBMsgArrPack,
    sBBMsgHeaderPack sBBMsgHeadPack,s_BuBiaoTatoilResult *sTatoiResultPackage)
{
  int i = 0;
  unsigned char *pSrcBuf = sBBMsgArrPack.ucMsgArr;  
    
  s_RealTimeAVControl sRealTimeAVControl;    
  memset(&sRealTimeAVControl,0,sizeof(s_RealTimeAVControl));
  
  i = sBBMsgHeadPack.iMsgHeadLen + 1 ;//��ʶ+��Ϣͷ��   
  sRealTimeAVControl.ucLogicChanlNm = pSrcBuf[i]; //0x02 ��ֵ ������ǰ��  
  
  i+=1;
  sRealTimeAVControl.ucControlID = pSrcBuf[i];

  i+=1;
  sRealTimeAVControl.ucCloseAVType = pSrcBuf[i];  

  i+=1;
  sRealTimeAVControl.ucSwitchStreamType = pSrcBuf[i];  

  switch(sRealTimeAVControl.ucControlID){    //0:�رգ�1:���� 0x7f:����
   case 0x00:
    if(g_ucRealtimePushAVFlag == 0x01){
       DoTearDown((const char*)g_sRealTimeAVBody.usServerIPAddr);
       if(g_RtpSessionUdpSock != -1)close(g_RtpSessionUdpSock);
       g_ucRealtimePushAVFlag = 0x00;
    }
    if(g_sRealTimeAVBody.ucMediumType == 0x02){
       sTatoiResultPackage->sKakaAckPack.ucResult = 0x00;
    }
    break;
   case 0x7F:   g_ucPushTimerOutFlag = 0;   break; //���֣���λ��ʱ��־
  }  
  return 0;
}

/*
��������/����͸��
*/
int DataPassthroughDownProcess(sBBMsgArr sBBMsgArrPack, sBBMsgHeaderPack sBBMsgHeadPack,
   s_BuBiaoTatoilResult *sTatoiResultPackage)
{
    unsigned char ucPassthroughMsgType = 0;
    int i = 0;
    char *pSrcBuf = (char*)sBBMsgArrPack.ucMsgArr;
    
    s_CarRealtimeInformation sCarRealtimeInformation;
        
    /*�������Ļ������·��Ŀ�������*/
    i = sBBMsgHeadPack.iMsgHeadLen + 1;  
    ucPassthroughMsgType = pSrcBuf[i];//͸����Ϣ���� 0xFD
    printf("ucPassthroughMsgType:%02x\n",ucPassthroughMsgType);

    memset(&(sTatoiResultPackage->sEyeExtendedProtocolBag),0,sizeof(s_EyeExtendedProtocol));

    i++;
    sTatoiResultPackage->sEyeExtendedProtocolBag.ucIDCode = pSrcBuf[i];

    i++;
    sTatoiResultPackage->sEyeExtendedProtocolBag.ucVersionNm = pSrcBuf[i];
    i++;
    sTatoiResultPackage->sEyeExtendedProtocolBag.ucSubProtocolFunction = pSrcBuf[i];
    i++;
    sTatoiResultPackage->sEyeExtendedProtocolBag.ucSubProtocolCommond = pSrcBuf[i];
    i++;
    sTatoiResultPackage->sEyeExtendedProtocolBag.usDataLen = MAKEWORD(pSrcBuf[i],pSrcBuf[i+1]);

    i++;
    i++;    
    memcpy(sTatoiResultPackage->sEyeExtendedProtocolBag.ucEyeExtendData,
        pSrcBuf+i,sTatoiResultPackage->sEyeExtendedProtocolBag.usDataLen);

    /*����Ӧ����*/
    if(sTatoiResultPackage->sEyeExtendedProtocolBag.ucSubProtocolFunction == 0xF5){
        switch(sTatoiResultPackage->sEyeExtendedProtocolBag.ucSubProtocolCommond){
         case 0x01:break;
         case 0x02:break;
         case 0x03:/*����ʵʱ��Ϣ��ȡ*/
            printf("Start get car real time informtion\n");
            memset(&sCarRealtimeInformation,0,sizeof(s_CarRealtimeInformation));
            memcpy(sCarRealtimeInformation.ucTime,pSrcBuf+i,6);
            i+=6;
            sCarRealtimeInformation.usBatteryVoltage = MAKEWORD(pSrcBuf[i],pSrcBuf[i+1]);//˳��ȷ��
            i+=2;
            sCarRealtimeInformation.usEngineSpeed = MAKEWORD(pSrcBuf[i],pSrcBuf[i+1]);
            i+=2;
            sCarRealtimeInformation.ucCarSpeed = pSrcBuf[i];
            i++;
            sCarRealtimeInformation.ucThrottlePercentage = pSrcBuf[i];
            i++;            
            sCarRealtimeInformation.ucEngineLoad = pSrcBuf[i];
            i++;    
            sCarRealtimeInformation.ucCoolantTemperature = pSrcBuf[i];
            i++;                

            sCarRealtimeInformation.usInstantaneousFuelConsumption = MAKEWORD(pSrcBuf[i],pSrcBuf[i+1]);
            i+=2;

            sCarRealtimeInformation.usAverageFuelConsumption = MAKEWORD(pSrcBuf[i],pSrcBuf[i+1]);
            i+=2;
            sCarRealtimeInformation.usTheMileage = MAKEWORD(pSrcBuf[i],pSrcBuf[i+1]);
            i+=2;
            
            sCarRealtimeInformation.usTotalMileage = MAKEWORD(pSrcBuf[i],pSrcBuf[i+1]);
            i+=2;
            
            sCarRealtimeInformation.usTheFuelConsumption = MAKEWORD(pSrcBuf[i],pSrcBuf[i+1]);
            i+=2;
            
            sCarRealtimeInformation.usTotalFuelConsumption = MAKEWORD(pSrcBuf[i],pSrcBuf[i+1]);
            i+=2;

            ShowCarRealInformtion(sCarRealtimeInformation);
            break;
         case 0x04:       break;/*���Ķ�дOBD����*/  
        }
    }
    /*Ҫ���г���¼���Լ켰Ӧ��*/
    if(sTatoiResultPackage->sEyeExtendedProtocolBag.ucSubProtocolFunction == 0xF9){
       memset(&sTatoiResultPackage->sKaKaRalationBag,0,sizeof(s_KaKaRalation));
       sTatoiResultPackage->sKaKaRalationBag.ucCmdKey = pSrcBuf[i];
       printf("sTatoiResultPackage->sKaKaRalationBag.ucCmdKey :%02x\n",sTatoiResultPackage->sKaKaRalationBag.ucCmdKey);
       i++;

       sTatoiResultPackage->sKaKaRalationBag.ucPeripheralNmID = pSrcBuf[i];
       i++;
       
       sTatoiResultPackage->sKaKaRalationBag.uDataLen = pSrcBuf[i];
       i++;

       sTatoiResultPackage->sKaKaRalationBag.usCarStatus = MAKEWORD(pSrcBuf[i],pSrcBuf[i+1]);
       i+=2;


       if(sTatoiResultPackage->sKaKaRalationBag.ucCmdKey == 0x38){//���յ���ָ����Ҫ�������ǵ�״̬��Ϣ g_KakaAckBag
          memset(&sTatoiResultPackage->sKakaAckPack,0,sizeof(sKaKaAckBag));
          //Ҫ��ʼ��ȡsKaKaAckBag����Ϣ��  ������Ӧ�� �˽ṹ���������ط���ֵ
       }
    }
    /*E-MIV406��WIFI ��ش���*/
    if(sTatoiResultPackage->sEyeExtendedProtocolBag.ucSubProtocolFunction == 0xFA){
        
        
    }
	return 0;
    
}

/*
����ָ������g_sDataSearchMsgBagȥkakaӦ�ò��ѯָ����Ϣ
��������д��g_sDataSearchAckBag
int g_iAVMediaTatoilCnt = 0;
int g_iPictMediaTatoilCnt = 0;
*/
int  SearchMediaProcess(s_BuBiaoTatoilResult *sTatoiResultPackage,sBBMsgHeaderPack sBBMsgHeadPack)
{
 if(sTatoiResultPackage->sDataSearchMsgBag.ucMediaType == 0x00){    
    //printf("����ͼƬ��Ϣ\n");
    SearchMediaProcessCore("/mnt/mmc/PHOTO",sTatoiResultPackage,sBBMsgHeadPack);
 }
 if(sTatoiResultPackage->sDataSearchMsgBag.ucMediaType == 0x01){
    printf("����ѭ����Ƶ\n");    
    //SearchMediaProcessCore("/mnt/mmc/VIDEO",sTatoiResultPackage,sBBMsgHeadPack);
 }
 if(sTatoiResultPackage->sDataSearchMsgBag.ucMediaType == 0x02){
    printf("����ѭ����Ƶ\n");    
    SearchMediaProcessCore("/mnt/mmc/VIDEO",sTatoiResultPackage,sBBMsgHeadPack);
 }
 if(sTatoiResultPackage->sDataSearchMsgBag.ucMediaType == 0x03){
    printf("����������Ƶ\n");
    SearchMediaProcessCore("/mnt/mmc/GVIDEO",sTatoiResultPackage,sBBMsgHeadPack);
 }
 return 0;
}



/*
����Э��Ĵ��� ��sBBMsgArrPack���ݰ��Ĵ���
*/
int BuBiaoMsgProcess(sBBMsgArr *sBBMsgArrPack)
{ 
    sBBMsgArr sMsgHeadArr,sMsgBodyArr,sMsgArr;   
    sBBMsgHeaderPack sBBMsgHeadPack,sBBAckMsgHeadPack;
    s_BuBiaoTatoilResult sTatoiResultPackage;
    
    unsigned char ucResponseMsgHard[BUBIAO_HEAD_MAX_LEN] = {0};
    unsigned char ucResponseMsgBody[BUBIAO_MAX_LEN-BUBIAO_HEAD_MAX_LEN-2] = {0};
    unsigned char ucBuBiaoAckBag[BUBIAO_MAX_LEN] = {0};

    g_esBuBaioProcState = STATE_USER_BUSY;    
    
    InitAckBag();   
    
    BBEscapeReduction(sBBMsgArrPack);         
    if(-1 == BBCheckCodeProcess(*sBBMsgArrPack)) return -1;
    GetBBRevMsgHeadFromBBMsg(*sBBMsgArrPack,&sBBMsgHeadPack);

    memset(&sTatoiResultPackage,0,sizeof(s_BuBiaoTatoilResult));

    sTatoiResultPackage.sResponsePack.usResponseSerialNm = sBBMsgHeadPack.usMsgSerialNm;
    printf("sTatoiResultPackage.sResponsePack.usResponseSerialNm:%04x\n",
        sTatoiResultPackage.sResponsePack.usResponseSerialNm);
    sTatoiResultPackage.usCMDMsgID = sBBMsgHeadPack.usMsgID;
    
    switch(sBBMsgHeadPack.usMsgID){
       case CMD_DATA_SEARCH_REQUEST:          
        MediaDataInformQueryProcess(*sBBMsgArrPack,sBBMsgHeadPack,&sTatoiResultPackage);             
        break;
       case CMD_DATA_PASSTHROUGH_REQUEST:      
        DataPassthroughDownProcess(*sBBMsgArrPack,sBBMsgHeadPack,&sTatoiResultPackage);              
        break;
       case CMD_REAL_TIME_AV_REQUEST:         
        RealTimeAVTransProcess(*sBBMsgArrPack,sBBMsgHeadPack,&sTatoiResultPackage);                  
        break;
       case CMD_REAL_TIME_AV_CONTROL:
        RealTimeAVTransControl(*sBBMsgArrPack,sBBMsgHeadPack,&sTatoiResultPackage);
        break;
       case CMD_MEDIA_INFORM_REQUEST:         
        MediaProfileInformQueryProcess(*sBBMsgArrPack,sBBMsgHeadPack,&sTatoiResultPackage);          
        break;
       case CMD_MEDIA_UPLOAD_REQUEST:         
        MediaUploadQueryProcess(*sBBMsgArrPack,sBBMsgHeadPack,&sTatoiResultPackage);                 
        break;
       case CMD_AREA_MEDIA_UPLOAD_REQUEST: 
        memset(&(sTatoiResultPackage.sAreaMediaUploadAttrBody),0,sizeof(s_AreaMediaUploadAttr));
        AreaMediaUploadQueryProcess(*sBBMsgArrPack,sBBMsgHeadPack,&sTatoiResultPackage);
        break;
      case SEARCH_SOURCE_LIST_CMD:
        SearchSourceListProcess(*sBBMsgArrPack,sBBMsgHeadPack,&sTatoiResultPackage);
        break;
      case VIDEO_PLAY_BACK_CMD:
        VideoPlayBackProcess(*sBBMsgArrPack,sBBMsgHeadPack,&sTatoiResultPackage);
        break;
    }

    if(sTatoiResultPackage.ucMsgReturnFlag == 0x01){//����ѯ�������ݣ��ڲ�ѯʱ�ѷ����򲻽����������
        sTatoiResultPackage.ucMsgReturnFlag = 0x00;
        g_esBuBaioProcState = STATE_USER_IDLE;
        return 0;
    }
    
    sMsgBodyArr.ucMsgArr = &ucResponseMsgBody[0];
    GetMsgBodyPackage(&sMsgBodyArr,sBBMsgHeadPack,sTatoiResultPackage);   
        
    GetBBAckMsgHeadFromBBRevMsgHead(sBBMsgHeadPack,
        &sBBAckMsgHeadPack,GetSubBagFlag(sMsgBodyArr.iMsgLen),sMsgBodyArr.iMsgLen,0,0);
    sMsgHeadArr.ucMsgArr = &ucResponseMsgHard[0];
    GetAckArrFromBBAckMsgHead(&sMsgHeadArr,sBBAckMsgHeadPack);    
    
    sMsgArr.ucMsgArr = &ucBuBiaoAckBag[0];
    GetAckPack(sMsgHeadArr,sMsgBodyArr,&sMsgArr);    
    BagQueueAddData(&stRetuBagQueue, sMsgArr.ucMsgArr, sMsgArr.iMsgLen);
       
    g_esBuBaioProcState = STATE_USER_IDLE;

   return 0;
}

/*�洢��ý�����ݼ���Ӧ��*/
static int GetMediaDataSearchMsgBodyArr(sBBMsgArr *sMsgBodyArr,s_BuBiaoTatoilResult sTatoiResultPackage)
{
    int iAckMsgBodyLen = 0;
    int iItemIndex = 0;
    int uiMediaID = 0;       //û���ϸ���
    char sTimeBCD[6] = {0};

    sMsgBodyArr->ucMsgArr[iAckMsgBodyLen] = HIBYTE(sTatoiResultPackage.sResponsePack.usResponseSerialNm);
    iAckMsgBodyLen++;
    sMsgBodyArr->ucMsgArr[iAckMsgBodyLen] = LOBYTE(sTatoiResultPackage.sResponsePack.usResponseSerialNm);
    iAckMsgBodyLen++; 

    sMsgBodyArr->ucMsgArr[iAckMsgBodyLen] = HIBYTE(sTatoiResultPackage.sDataSearchMsgBag.usMediaDataTatoil);
    iAckMsgBodyLen++;
    sMsgBodyArr->ucMsgArr[iAckMsgBodyLen] = LOBYTE(sTatoiResultPackage.sDataSearchMsgBag.usMediaDataTatoil);
    iAckMsgBodyLen++;   
    //printf("sTatoiResultPackage.sDataSearchMsgBag.usPreBagMediaDataTatoil:%d\n",sTatoiResultPackage.sDataSearchMsgBag.usPreBagMediaDataTatoil);
    for(iItemIndex = 0;iItemIndex < sTatoiResultPackage.sDataSearchMsgBag.usPreBagMediaDataTatoil;iItemIndex++){
        uiMediaID++;
        memcpy(sMsgBodyArr->ucMsgArr+iAckMsgBodyLen,&uiMediaID,4);
        iAckMsgBodyLen = iAckMsgBodyLen+4; 
        sMsgBodyArr->ucMsgArr[iAckMsgBodyLen] = sTatoiResultPackage.sDataSearchMsgBag.ucMediaType;
        iAckMsgBodyLen++;
        sMsgBodyArr->ucMsgArr[iAckMsgBodyLen] = sTatoiResultPackage.sDataSearchMsgBag.ucChnID;
        iAckMsgBodyLen++;    
        sMsgBodyArr->ucMsgArr[iAckMsgBodyLen] = sTatoiResultPackage.sDataSearchMsgBag.uEventItemCode;
        iAckMsgBodyLen++;   
        
        //sMsgBodyArr->ucMsgArr[iAckMsgBodyLen] = g_sSearchTotailPictBuf[iItemIndex].cLatitudeLongitudeFlag;
        sMsgBodyArr->ucMsgArr[iAckMsgBodyLen] = 0x00;//test ��γ������ʵ�����ͣ�������ʵ��
        iAckMsgBodyLen++;   
        memcpy(sMsgBodyArr->ucMsgArr+iAckMsgBodyLen,(char *)&g_sSearchTotailPictBuf[iItemIndex].dlatitude,4);
        iAckMsgBodyLen = iAckMsgBodyLen+4;//8; 
        memcpy(sMsgBodyArr->ucMsgArr+iAckMsgBodyLen,(char *)&g_sSearchTotailPictBuf[iItemIndex].dlongitude,4);
        iAckMsgBodyLen = iAckMsgBodyLen+4;
        sMsgBodyArr->ucMsgArr[iAckMsgBodyLen] = g_sSearchTotailPictBuf[iItemIndex].ucFileNameLen;
        iAckMsgBodyLen++; 
        memcpy(sMsgBodyArr->ucMsgArr+iAckMsgBodyLen,(char *)&g_sSearchTotailPictBuf[iItemIndex].ucFullFileNameBuf,g_sSearchTotailPictBuf[iItemIndex].ucFileNameLen);
        iAckMsgBodyLen = iAckMsgBodyLen+g_sSearchTotailPictBuf[iItemIndex].ucFileNameLen;                   
        memcpy(sMsgBodyArr->ucMsgArr+iAckMsgBodyLen,(char *)&g_sSearchTotailPictBuf[iItemIndex].uiFileSize,4);
        iAckMsgBodyLen = iAckMsgBodyLen+4;

        TimeStr2BCD((const char*)g_sSearchTotailPictBuf[iItemIndex].ucFileTime,sTimeBCD);
        memcpy(sMsgBodyArr->ucMsgArr+iAckMsgBodyLen,sTimeBCD,6);
        iAckMsgBodyLen = iAckMsgBodyLen+6;

        memcpy(sMsgBodyArr->ucMsgArr+iAckMsgBodyLen,(char *)&g_sSearchTotailPictBuf[iItemIndex].usVideoLen,2);
        iAckMsgBodyLen = iAckMsgBodyLen+2;                                   
    }
    sMsgBodyArr->iMsgLen = iAckMsgBodyLen;
    //printf("iAckMsgBodyLen:%d\n",iAckMsgBodyLen);
    return 0;
}

/*������������*/
static int GetDataPassThroughMsgBodyArr(sBBMsgArr *sMsgBodyArr,s_BuBiaoTatoilResult sTatoiResultPackage)
{
    int iAckMsgBodyLen = 0;
    /*3.2.1͸����Ϣ����*/   
    sMsgBodyArr->ucMsgArr[iAckMsgBodyLen] = 0xFD; 
    iAckMsgBodyLen++;
    /*3.2.2������չЭ�� ͸����Ϣ����A1*/
    sMsgBodyArr->ucMsgArr[iAckMsgBodyLen] = sTatoiResultPackage.sEyeExtendedProtocolBag.ucIDCode; 
    iAckMsgBodyLen++;
    sMsgBodyArr->ucMsgArr[iAckMsgBodyLen] = sTatoiResultPackage.sEyeExtendedProtocolBag.ucVersionNm; 
    iAckMsgBodyLen++;
    sMsgBodyArr->ucMsgArr[iAckMsgBodyLen] = sTatoiResultPackage.sEyeExtendedProtocolBag.ucSubProtocolFunction; 
    iAckMsgBodyLen++;
    sMsgBodyArr->ucMsgArr[iAckMsgBodyLen] = sTatoiResultPackage.sEyeExtendedProtocolBag.ucSubProtocolCommond; 
    iAckMsgBodyLen++;
    sMsgBodyArr->ucMsgArr[iAckMsgBodyLen] = HIBYTE(sTatoiResultPackage.sEyeExtendedProtocolBag.usDataLen); 
    iAckMsgBodyLen++;      
    sMsgBodyArr->ucMsgArr[iAckMsgBodyLen] = LOBYTE(sTatoiResultPackage.sEyeExtendedProtocolBag.usDataLen); 
    iAckMsgBodyLen++;
    /*3.2.3�г���¼��A2*/
    if(g_sKaKaRalationBag.ucCmdKey == 0x38){
      sMsgBodyArr->ucMsgArr[iAckMsgBodyLen] = sTatoiResultPackage.sKaKaRalationBag.ucCmdKey;
      iAckMsgBodyLen++;
      sMsgBodyArr->ucMsgArr[iAckMsgBodyLen] = sTatoiResultPackage.sKaKaRalationBag.ucPeripheralNmID;
      iAckMsgBodyLen++;
      sMsgBodyArr->ucMsgArr[iAckMsgBodyLen] = sTatoiResultPackage.sKaKaRalationBag.uDataLen;
      iAckMsgBodyLen++;      

      sMsgBodyArr->ucMsgArr[iAckMsgBodyLen] = HIBYTE(sTatoiResultPackage.sKakaAckPack.usKaKaStatu);
      iAckMsgBodyLen++;            
      sMsgBodyArr->ucMsgArr[iAckMsgBodyLen] = LOBYTE(sTatoiResultPackage.sKakaAckPack.usKaKaStatu);
      iAckMsgBodyLen++; 
    } 
    sMsgBodyArr->iMsgLen = iAckMsgBodyLen;
    return 0;
}

/*ʵʱ��ý������*/
static int GetRealTimeMediaMsgBodyArr(sBBMsgArr *sMsgBodyArr,s_BuBiaoTatoilResult sTatoiResultPackage)
{
#if(0)    
    int iAckMsgBodyLen = 0;
    sMsgBodyArr->ucMsgArr[iAckMsgBodyLen] = HIBYTE(sTatoiResultPackage.sResponsePack.usResponseSerialNm);
    iAckMsgBodyLen++;
    sMsgBodyArr->ucMsgArr[iAckMsgBodyLen] = LOBYTE(sTatoiResultPackage.sResponsePack.usResponseSerialNm);
    iAckMsgBodyLen++; 
    sMsgBodyArr->ucMsgArr[iAckMsgBodyLen] = sTatoiResultPackage.sKakaAckPack.ucResult = g_KakaAckBag.ucResult; 
    iAckMsgBodyLen++;
    sMsgBodyArr->iMsgLen = iAckMsgBodyLen;
#else
    int iAckMsgBodyLen = 0;
    sMsgBodyArr->ucMsgArr[iAckMsgBodyLen] = HIBYTE(sTatoiResultPackage.sResponsePack.usResponseSerialNm);
    iAckMsgBodyLen++;
    sMsgBodyArr->ucMsgArr[iAckMsgBodyLen] = LOBYTE(sTatoiResultPackage.sResponsePack.usResponseSerialNm);
    iAckMsgBodyLen++; 

    sMsgBodyArr->ucMsgArr[iAckMsgBodyLen] = HIBYTE(sTatoiResultPackage.usCMDMsgID);
    iAckMsgBodyLen++;
    sMsgBodyArr->ucMsgArr[iAckMsgBodyLen] = LOBYTE(sTatoiResultPackage.usCMDMsgID);
    iAckMsgBodyLen++;

    sMsgBodyArr->ucMsgArr[iAckMsgBodyLen] = sTatoiResultPackage.sKakaAckPack.ucResult = g_KakaAckBag.ucResult; 
    iAckMsgBodyLen++;  

    sMsgBodyArr->iMsgLen = iAckMsgBodyLen;
#endif
    return 0;
}

/*�洢ý���Ҫ��Ϣ��ѯ����*/
static int GetMediaPrefileInformMsgBodyArr(sBBMsgArr *sMsgBodyArr,s_BuBiaoTatoilResult sTatoiResultPackage)
{    
    char sTimeBCDBuf[6] = {0};
    int iAckMsgBodyLen = 0;
    
    sMsgBodyArr->ucMsgArr[iAckMsgBodyLen] = HIBYTE(sTatoiResultPackage.sResponsePack.usResponseSerialNm);
    iAckMsgBodyLen++;
    sMsgBodyArr->ucMsgArr[iAckMsgBodyLen] = LOBYTE(sTatoiResultPackage.sResponsePack.usResponseSerialNm);
    iAckMsgBodyLen++; 
    
    sMsgBodyArr->ucMsgArr[iAckMsgBodyLen] = 0x01;//�豸��ͨ������ָ����������Ԫ�صĸ��� 
    iAckMsgBodyLen++; 
    
    sMsgBodyArr->ucMsgArr[iAckMsgBodyLen] = sTatoiResultPackage.sMediaProfileBody.ucChnID;
    iAckMsgBodyLen++; 

    TimeStr2BCD((const char*)sTatoiResultPackage.sMediaProfileBody.ucStartTime,sTimeBCDBuf);
    memcpy(sMsgBodyArr->ucMsgArr+iAckMsgBodyLen,sTimeBCDBuf,6);     
    //printf("sTatoiResultPackage.sMediaProfileBody.ucStartTime:%s sTimeBCDBuf:%s\n",sTatoiResultPackage.sMediaProfileBody.ucStartTime,sTimeBCDBuf);
    iAckMsgBodyLen+=6;

    memset(sTimeBCDBuf,0,sizeof(sTimeBCDBuf));    
    TimeStr2BCD((const char*)sTatoiResultPackage.sMediaProfileBody.ucEndTime,sTimeBCDBuf);
    //printf("sTatoiResultPackage.sMediaProfileBody.ucEndTime:%s sTimeBCDBuf:%s\n",sTatoiResultPackage.sMediaProfileBody.ucEndTime,sTimeBCDBuf);
    memcpy(sMsgBodyArr->ucMsgArr+iAckMsgBodyLen,sTimeBCDBuf,6); 
    iAckMsgBodyLen+=6;

    sMsgBodyArr->ucMsgArr[iAckMsgBodyLen] = HIBYTE(sTatoiResultPackage.sMediaProfileBody.usFileCnt);
    iAckMsgBodyLen++;
    sMsgBodyArr->ucMsgArr[iAckMsgBodyLen] = LOBYTE(sTatoiResultPackage.sMediaProfileBody.usFileCnt);       
    iAckMsgBodyLen++;
  
    sMsgBodyArr->iMsgLen = iAckMsgBodyLen;
    return 0;
}

/*ƽ̨�·�Զ��ý���ϴ�Ӧ��*/
static int GetMediaUploadMsgBodyArr(sBBMsgArr *sMsgBodyArr,s_BuBiaoTatoilResult sTatoiResultPackage)
{    
    int iAckMsgBodyLen = 0;
    sMsgBodyArr->ucMsgArr[iAckMsgBodyLen] = HIBYTE(sTatoiResultPackage.sResponsePack.usResponseSerialNm);
    iAckMsgBodyLen++;
    sMsgBodyArr->ucMsgArr[iAckMsgBodyLen] = LOBYTE(sTatoiResultPackage.sResponsePack.usResponseSerialNm);
    iAckMsgBodyLen++; 
    /*
    sMsgBodyArr->ucMsgArr[iAckMsgBodyLen] = sTatoiResultPackage.sMediaUploadAttrBody.ucChnID; 
    iAckMsgBodyLen++;     
    */
    sMsgBodyArr->ucMsgArr[iAckMsgBodyLen] = sTatoiResultPackage.sKakaAckPack.ucResult; 
    iAckMsgBodyLen++;   

    sMsgBodyArr->iMsgLen = iAckMsgBodyLen;
    return 0;
}

/*��ʱ���������Ƶ��Ԥ��ͼ�ϴ�Ӧ��*/
static int GetAreaMediaUploadMsgBodyArr(sBBMsgArr *sMsgBodyArr,s_BuBiaoTatoilResult sTatoiResultPackage)
{
    int iAckMsgBodyLen = 0;
    sMsgBodyArr->ucMsgArr[iAckMsgBodyLen] = HIBYTE(sTatoiResultPackage.sResponsePack.usResponseSerialNm);
    iAckMsgBodyLen++;
    sMsgBodyArr->ucMsgArr[iAckMsgBodyLen] = LOBYTE(sTatoiResultPackage.sResponsePack.usResponseSerialNm);
    iAckMsgBodyLen++;  
   
    sMsgBodyArr->ucMsgArr[iAckMsgBodyLen] = sTatoiResultPackage.sKakaAckPack.ucResult; 
    iAckMsgBodyLen++; 

    sMsgBodyArr->iMsgLen = iAckMsgBodyLen;
    return 0;
}      

static int GetSearchSourceListMsgBodyArr(sBBMsgArr *sMsgBodyArr,s_BuBiaoTatoilResult sTatoiResultPackage)
{
    int iAckMsgBodyLen = 0;
    int iItemIndex = 0;  
    char sTimeBCD[6] = {0};

    sMsgBodyArr->ucMsgArr[iAckMsgBodyLen] = HIBYTE(sTatoiResultPackage.sResponsePack.usResponseSerialNm);
    iAckMsgBodyLen++;
    sMsgBodyArr->ucMsgArr[iAckMsgBodyLen] = LOBYTE(sTatoiResultPackage.sResponsePack.usResponseSerialNm);
    iAckMsgBodyLen++; 

    sMsgBodyArr->ucMsgArr[iAckMsgBodyLen] = HIBYTE(sTatoiResultPackage.sDataSearchMsgBag.usMediaDataTatoil);
    iAckMsgBodyLen++;
    sMsgBodyArr->ucMsgArr[iAckMsgBodyLen] = LOBYTE(sTatoiResultPackage.sDataSearchMsgBag.usMediaDataTatoil);
    iAckMsgBodyLen++;   
    //printf("usPreBagMediaDataTatoil:%d\n",sTatoiResultPackage.sDataSearchMsgBag.usPreBagMediaDataTatoil);
    for(iItemIndex = 0;iItemIndex < sTatoiResultPackage.sDataSearchMsgBag.usPreBagMediaDataTatoil;iItemIndex++){

        sMsgBodyArr->ucMsgArr[iAckMsgBodyLen] = g_sSearchSourceBuf[iItemIndex].ucLogicChnID;
        iAckMsgBodyLen++;

        TimeStr2BCD((const char*)g_sSearchSourceBuf[iItemIndex].ucFileStartTime,sTimeBCD);
        memcpy(sMsgBodyArr->ucMsgArr+iAckMsgBodyLen,sTimeBCD,6);
        iAckMsgBodyLen = iAckMsgBodyLen+6;

        TimeStr2BCD((const char*)g_sSearchSourceBuf[iItemIndex].ucFileEndTime,sTimeBCD);
        memcpy(sMsgBodyArr->ucMsgArr+iAckMsgBodyLen,sTimeBCD,6);
        iAckMsgBodyLen = iAckMsgBodyLen+6;

        memcpy(sMsgBodyArr->ucMsgArr+iAckMsgBodyLen,(char *)&g_sSearchSourceBuf[iItemIndex].ucWarningMark,8);
        iAckMsgBodyLen = iAckMsgBodyLen+8;                                           

        sMsgBodyArr->ucMsgArr[iAckMsgBodyLen] = g_sSearchSourceBuf[iItemIndex].ucSourceType;
        iAckMsgBodyLen++;   

        sMsgBodyArr->ucMsgArr[iAckMsgBodyLen] = g_sSearchSourceBuf[iItemIndex].ucStreamType;
        iAckMsgBodyLen++;           

        sMsgBodyArr->ucMsgArr[iAckMsgBodyLen] = g_sSearchSourceBuf[iItemIndex].ucStoreType;
        iAckMsgBodyLen++;   

        memcpy(sMsgBodyArr->ucMsgArr+iAckMsgBodyLen,(char *)&g_sSearchSourceBuf[iItemIndex].uiFileSize,4);
        iAckMsgBodyLen = iAckMsgBodyLen+4;                                                                       
    }
    sMsgBodyArr->iMsgLen = iAckMsgBodyLen;
    //printf("iAckMsgBodyLen:%d\n",iAckMsgBodyLen);
    return 0;
}

/*ͨ��Ӧ����Ϣ��*/
static int GetGeneralAckMsgBodyArr(sBBMsgArr *sMsgBodyArr,s_BuBiaoTatoilResult sTatoiResultPackage)
{
    int iAckMsgBodyLen = 0;
    sMsgBodyArr->ucMsgArr[iAckMsgBodyLen] = HIBYTE(sTatoiResultPackage.sResponsePack.usResponseSerialNm);
    iAckMsgBodyLen++;
    sMsgBodyArr->ucMsgArr[iAckMsgBodyLen] = LOBYTE(sTatoiResultPackage.sResponsePack.usResponseSerialNm);
    iAckMsgBodyLen++; 

#if(0)
    sMsgBodyArr->ucMsgArr[iAckMsgBodyLen] = 0x00;//�޸�ͨ��Ӧ����ϢIDΪ��ֵ
    iAckMsgBodyLen++;
    sMsgBodyArr->ucMsgArr[iAckMsgBodyLen] = 0x01;
    iAckMsgBodyLen++;
#else
    sMsgBodyArr->ucMsgArr[iAckMsgBodyLen] = HIBYTE(sTatoiResultPackage.usCMDMsgID);
    iAckMsgBodyLen++;
    sMsgBodyArr->ucMsgArr[iAckMsgBodyLen] = LOBYTE(sTatoiResultPackage.usCMDMsgID);
    iAckMsgBodyLen++;
#endif    

    sMsgBodyArr->ucMsgArr[iAckMsgBodyLen] = sTatoiResultPackage.sKakaAckPack.ucResult; 
    iAckMsgBodyLen++;  

    sMsgBodyArr->iMsgLen = iAckMsgBodyLen;
    return 0;
}

int GetMsgBodyPackage(
    sBBMsgArr *sMsgBodyArr,
    sBBMsgHeaderPack sBBMsgHeadPack,
    s_BuBiaoTatoilResult sTatoiResultPackage)
{  
    switch(sBBMsgHeadPack.usMsgID){
       case CMD_DATA_SEARCH_REQUEST:          
        GetMediaDataSearchMsgBodyArr(sMsgBodyArr,sTatoiResultPackage);
        break;
       case CMD_DATA_PASSTHROUGH_REQUEST:     
        GetDataPassThroughMsgBodyArr(sMsgBodyArr,sTatoiResultPackage);           
        break;
       case CMD_REAL_TIME_AV_REQUEST:         
       case CMD_REAL_TIME_AV_CONTROL:
        GetRealTimeMediaMsgBodyArr(sMsgBodyArr,sTatoiResultPackage);
        break;
       case CMD_MEDIA_INFORM_REQUEST:         
        GetMediaPrefileInformMsgBodyArr(sMsgBodyArr,sTatoiResultPackage);          
        break;
       case CMD_MEDIA_UPLOAD_REQUEST:         
        GetMediaUploadMsgBodyArr(sMsgBodyArr,sTatoiResultPackage);                 
        break;
       case CMD_AREA_MEDIA_UPLOAD_REQUEST:    
        GetAreaMediaUploadMsgBodyArr(sMsgBodyArr,sTatoiResultPackage);             
        break;
       case SEARCH_SOURCE_LIST_CMD:
        GetSearchSourceListMsgBodyArr(sMsgBodyArr,sTatoiResultPackage);             
        break;
       case VIDEO_PLAY_BACK_CMD:
        
        break;
       default:
        GetGeneralAckMsgBodyArr(sMsgBodyArr,sTatoiResultPackage);
        break;
    }    
  return 0;
}


static void bubiao_msg_process_thread(void * pArgs)
{  
  sBBMsgArr *sArgu = (sBBMsgArr*)pArgs;
  while(1)
  {
    usleep(500000);  
    BuBiaoMsgProcess(sArgu);
    if(g_esBuBaioProcState == STATE_USER_IDLE)break;
  }
}

void BuBiaoProtoRetuBagHandle()
{
    int iBagLen = 0;
    unsigned char *ucDstBag = NULL;    
    
    iBagLen = BagQueueGetData(&stRetuBagQueue, &ucDstBag);
    printf("ָ���:");
    PrintfString2Hex(ucDstBag,iBagLen);
    SendDataTo406(ucDstBag,iBagLen);
}


void BuBiaoProtoRecvBagHandle()
{        
    memset(&g_sBBMsgArrBag,0,sizeof(sBBMsgArr));     
    g_sBBMsgArrBag.iMsgLen = BagQueueGetData(&stRecvBagQueue, &(g_sBBMsgArrBag.ucMsgArr));    
    if(g_esBuBaioProcState == STATE_USER_IDLE){
        pthread_t tfid2;
        int ret2 = 0;
        ret2 = pthread_create(&tfid2, NULL, (void*)bubiao_msg_process_thread, (void*)&g_sBBMsgArrBag);
        if (ret2 != 0){
            printf("pthread_create failed, %d, %s\n", errno, strerror(errno));
            return;
        }    
        pthread_detach(tfid2);
    }else{
        BuBiaoMsgBusyProcess(&g_sBBMsgArrBag);
    }
    
}





