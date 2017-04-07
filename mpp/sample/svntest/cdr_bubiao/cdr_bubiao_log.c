/************************************************************************	
** Filename: 	
** Description:  
** Author: 	xjl
** Create Date: 
** Version: 	v1.0

	Copyright(C) 2016 e-eye CO.LTD. ShenZhen <www.e-eye.cn>

*************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include "cdr_bubiao_log.h"

int ShowRealTimeAVAttr(s_RealTimeAVBody sRealTimeAVBody)
{
     printf("ShowRealTimeAVAttr*********************************************\n");     
     printf("sRealTimeAVBody.ucServerIPAddrLen:%02x\n",sRealTimeAVBody.ucServerIPAddrLen);
     int j = 0;
     for(j=0;j<sRealTimeAVBody.ucServerIPAddrLen;j++){ 
         printf("%c",sRealTimeAVBody.usServerIPAddr[j]);
     }
     printf("\n");     
     printf("sRealTimeAVBody.usServerTCPListenPort:%d ,%02x\n",sRealTimeAVBody.usServerTCPListenPort,sRealTimeAVBody.usServerTCPListenPort);
     printf("sRealTimeAVBody.usServerUDPListenPort:%d ,%02x\n",sRealTimeAVBody.usServerUDPListenPort,sRealTimeAVBody.usServerUDPListenPort);
     printf("sRealTimeAVBody.ucAVChanlNm: %d ,%02x\n",sRealTimeAVBody.ucAVChanlNm,sRealTimeAVBody.ucAVChanlNm);
     printf("sRealTimeAVBody.ucMediumType:%d ,%02x\n",sRealTimeAVBody.ucMediumType,sRealTimeAVBody.ucMediumType);
     printf("sRealTimeAVBody.ucStreamType:%d ,%02x\n",sRealTimeAVBody.ucStreamType,sRealTimeAVBody.ucStreamType);
     printf("sRealTimeAVBody.ucSwitchFlag:%d ,%02x\n",sRealTimeAVBody.ucSwitchFlag,sRealTimeAVBody.ucSwitchFlag);
     printf("********************************************************\n");
     return 0;
}

void ShowCarRealInformtion(s_CarRealtimeInformation sCarRealtimeInformation)
{
  printf("g_sCarRealtimeInformation.ucCarSpeed:%02x\n",sCarRealtimeInformation.ucCarSpeed);
  printf("g_sCarRealtimeInformation.usEngineSpeed:%02x\n",sCarRealtimeInformation.usEngineSpeed);
  printf("g_sCarRealtimeInformation.usTotalMileage:%02x\n",sCarRealtimeInformation.usTotalMileage);
  printf("g_sCarRealtimeInformation.usTotalFuelConsumption:%02x\n",sCarRealtimeInformation.usTotalFuelConsumption);
}


/*
收到主机或服务器下发的指令，
将解析后的指令的头信息打印
*/
int ShowMsgHead(sBBMsgHeaderPack sMsgHeadPack)
{
 printf("ShowMsgHead**************************************************\n");
 printf("usMsgID:%04x\n",sMsgHeadPack.usMsgID);
 printf("sMsgBodyAttr.Reserved:%02x\n",sMsgHeadPack.sMsgBodyAttr.Reserved);
 printf("sMsgBodyAttr.SubPackage:%02x\n",sMsgHeadPack.sMsgBodyAttr.SubPackage);
 printf("sMsgBodyAttr.DataEncryption:%02x\n",sMsgHeadPack.sMsgBodyAttr.DataEncryption);
 printf("sMsgBodyAttr.MsgBodyLen:%02x\n",sMsgHeadPack.sMsgBodyAttr.MsgBodyLen);

 int i = 0;
 printf("ucTerminalPhoneNm:");
 for(i=0;i<6;i++)
 {
   printf("%02x ",sMsgHeadPack.ucTerminalPhoneNm[i]);
 }
 printf("\n");
 printf("usMsgSerialNm:%04x\n",sMsgHeadPack.usMsgSerialNm);
 if(sMsgHeadPack.sMsgBodyAttr.SubPackage == 0x01){
   printf("sMsgPackItem.usMsgTotalPackageNm:%02x\n",sMsgHeadPack.sMsgPackItem.usMsgTotalPackageNm); 
   printf("sMsgPackItem.usMsgPackIndex:%02x\n",sMsgHeadPack.sMsgPackItem.usMsgPackIndex); 
 }
 printf("sBBMsgHeaderPack Len :%d\n",sMsgHeadPack.iMsgHeadLen);
 printf("********************************************************\n");
 return 0;
}


