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

#include <sys/stat.h>
#include <sys/vfs.h>
#include <dirent.h>

#include <dirent.h>
#include <sys/stat.h> 

#include "cdr_bubiao.h"
#include "cdr_bubiao_analyze.h"
#include "cdr_ftp_process.h"
#include "cdr_device.h"
#include "queue_bag.h"
#include "cdr_bubiao_driver.h"
#include "cdr_count_file.h"
#include "cdr_mp4_api.h"

static unsigned char g_ucCapturePictureFlag = 0;
extern unsigned char s_UrlBuff[200];

int g_iAVMediaTatoilCnt = 0;
int g_iPictMediaTatoilCnt = 0;

unsigned char g_ucSubBagFlag = 0x00;

unsigned char g_usAreaMediaUploadFlag = 0x00;
/*
���� 0:�����зְ�
     1:���зְ�
*/
unsigned char GetSubBagFlag(unsigned short usMsgBodyLen)
{
  //printf("usMsgBodyLen:%d\n",usMsgBodyLen);
  if(usMsgBodyLen > BUBIAO_MAX_LEN - 21){//18+1+2 ����ͷ��β ��ʶ�� �ܳ�
     return 0x01;
  }else{
     return 0x00;
  }
}

/*
������ʼʱ���뾭����ʱ��(��λs)
�����ʱ��(������ʱ����)
*/
void GetEndTime(s_AreaMediaUploadAttr *sAreaMediaUploadAttrBody)
{    
    time_t t1 = 0;    
    struct tm* when = localtime (&t1);
    int year,mon,day,hour,min,sec;

    sscanf((char*)sAreaMediaUploadAttrBody->ucStartTime,"%4d%2d%2d%2d%2d%2d",&year,&mon,&day,&hour,&min,&sec);
    //printf("%04d %02d %02d %02d %02d %02d\n",year,mon,day,hour,min,sec);

    when->tm_year = year-1900;
    when->tm_mon = mon-1;
    when->tm_mday = day;
    
    when->tm_hour = hour ;
    when->tm_min = min;
    when->tm_sec = sec;
    
    t1 = mktime(when);
    t1 = t1 + sAreaMediaUploadAttrBody->usAVTimeLen;
    //printf("sAreaMediaUploadAttrBody->usAVTimeLen:%d\n",sAreaMediaUploadAttrBody->usAVTimeLen);

    struct tm *p;
    p = gmtime(&t1);
    //printf("%04d %02d %02d %02d %02d %02d\n",(1900+p->tm_year), (1+p->tm_mon), p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec);

    char cEndTime[15] = {0};
    sprintf(cEndTime,"%04d%02d%02d%02d%02d%02d", 
        (1900+p->tm_year), (1+p->tm_mon), p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec);
    strcpy((char*)sAreaMediaUploadAttrBody->ucEndTime,cEndTime);
    printf("sAreaMediaUploadAttrBody->ucEndTime:%s\n",sAreaMediaUploadAttrBody->ucEndTime);
}


static void RmmoveTmpFile(s_AreaMediaUploadAttr sAreaMediaUploadAttrBody)
{
    char sRemovTarCmd[100] = {0};
    sprintf(sRemovTarCmd,"rm -rf /mnt/mmc/VIDEO/%s.tar.gz /mnt/mmc/VIDEO/%s",
    sAreaMediaUploadAttrBody.ucStartTime,sAreaMediaUploadAttrBody.ucStartTime);
    cdr_system(sRemovTarCmd);
}


/*
time_t time(time_t *t);
ȡ�ô�1970��1��1�����������
buf.st_ctime:�ļ����һ�θı��ʱ��
*/
void GetFileMaxMinTime(char *pFileName,time_t *lStartTime,time_t *lEndTime)
{
   struct stat buf;
   int result;
   result =stat(pFileName, &buf);    //����ļ�״̬��Ϣ
   
   if( result != 0 ){
        perror("��ʾ�ļ�״̬��Ϣ����");
   }else{      
        if(*lStartTime == 0x00){            
           *lStartTime = buf.st_ctime;
        }
        if(*lEndTime == 0x00){
           *lEndTime = buf.st_ctime;
        }  

        if(*lStartTime > buf.st_ctime) *lStartTime = buf.st_ctime;        
        if(*lEndTime < buf.st_ctime) *lEndTime = buf.st_ctime;
    }
}


/*����ý������*/
int GetMediaProfileInform(unsigned char ucType,s_BuBiaoTatoilResult *sTatoiResultPackage)
{
    DIR *dir = NULL;
    struct dirent * ptr = NULL;
    int total = 0;   
	char ucFullFileName[100] = {0};
	char ucStartTime[15] = {0};
	char ucEndTime[15] = {0};
   
    if(ucType == 0x00){
      if((dir=opendir("/mnt/mmc/PHOTO"))==NULL){
         printf("open fails\n");
         return -1;
      }
      //printf("ͼ���Ҫ��Ϣ��ѯ\n");
    }
    else if(ucType == 0x02){
      if((dir=opendir("/mnt/mmc/VIDEO"))==NULL){
         printf("open fails\n");
         return -1;
      }
    }
    else{
       return 0;
    }

    time_t lStartTime = 0,lEndTime = 0;

    while((ptr = readdir(dir)) != NULL){
        if(strcmp(ptr->d_name,".") == 0 || strcmp(ptr->d_name,"..") == 0){
           continue;
        }          
        if(ucType == 0x00)
        {       
            if(strstr(ptr->d_name,"_pre")){
               continue;
            }
            if(strstr(ptr->d_name,"_")!=NULL){//��ײͼƬ? 20170317173508_010.jpg
               continue;      
            }
            if(strstr(ptr->d_name,"G")!=NULL){
               continue;      
            }
            if(strstr(ptr->d_name,".jpg")){
               total++;
               sprintf(ucFullFileName,"/mnt/mmc/PHOTO/%s",ptr->d_name);
               //printf("ucFullFileName:%s\n",ucFullFileName);
               GetFileMaxMinTime(ucFullFileName,&lStartTime,&lEndTime);
            }
        }
        if(ucType == 0x02){
           if(strstr(ptr->d_name,".MP4")){
             total++;
             sprintf(ucFullFileName,"/mnt/mmc/VIDEO/%s",ptr->d_name);
             GetFileMaxMinTime(ucFullFileName,&lStartTime,&lEndTime);
           }
        }
    }

    struct tm *p;
    p = localtime(&lStartTime); 
    /*
    printf("�����ʱ��%d/%d/%d ", 1900 + p->tm_year, 1 + p->tm_mon, p->tm_mday);
    printf("%d:%d:%d\n", p->tm_hour, p->tm_min, p->tm_sec);
    */
    sprintf(ucStartTime,"%04d%02d%02d%02d%02d%02d", 1900 + p->tm_year, 
        1 + p->tm_mon, p->tm_mday,p->tm_hour, p->tm_min, p->tm_sec);
    
    p = localtime(&lEndTime); 
    /*
    printf("�����ʱ��%d/%d/%d ", 1900 + p->tm_year, 1 + p->tm_mon, p->tm_mday);
    printf("%d:%d:%d\n", p->tm_hour, p->tm_min, p->tm_sec);
    */
    sprintf(ucEndTime,"%04d%02d%02d%02d%02d%02d", 1900 + p->tm_year,
        1 + p->tm_mon, p->tm_mday,p->tm_hour, p->tm_min, p->tm_sec);
   
    memcpy((*sTatoiResultPackage).sMediaProfileBody.ucStartTime,ucStartTime,strlen(ucStartTime)+1);
    memcpy((*sTatoiResultPackage).sMediaProfileBody.ucEndTime,ucEndTime,strlen(ucEndTime)+1);
    //printf("media cnt :%d\n",total);

    if(dir!=NULL){
      closedir(dir);
      dir = NULL;
    }
    if(ucType == 0x02){
      g_iAVMediaTatoilCnt = total;
    }
    if(ucType == 0x00){
      g_iPictMediaTatoilCnt = total;
    } 
  return total;
}


int  SendDataTo406(unsigned char *ucData,int iLen)
{
  int res = 0;
  res = Uart2SendData(ucData,iLen);

  return res;
}

unsigned char GetCapturePictureFlag()
{
  printf("g_ucCapturePictureFlag:%d\n",g_ucCapturePictureFlag);
  return g_ucCapturePictureFlag;
}

unsigned char SetCapturePictureFlag(unsigned char ucValue)
{
  g_ucCapturePictureFlag = ucValue;  
  return g_ucCapturePictureFlag;
}

int GetUrlBuff(char *pcDst)
{
  memcpy(pcDst,s_UrlBuff,strlen((char*)s_UrlBuff)+1);
  return 0;
}

int SetUrlBuff(char *pcDst,int iLen)
{
  memcpy(s_UrlBuff,pcDst,iLen);
  return 0;
}


/*��ȡ�ֻ���*/
int GetTerminalMobilePhoneNm(unsigned char * ucTerminalPhoneNm)
{
  //int res;

  return 0; 
}

extern list* g_uploadfileList;

/*
��λ������ص�ȫ�ֱ���
*/
void InitAckBag()
{
    memset(&g_KakaAckBag,0,sizeof(sKaKaAckBag));
    memset(&g_sBBMsgAckHeadPack,0,sizeof(sBBMsgHeaderPack));
    if(g_uploadfileList == NULL)
	{
		g_uploadfileList = create_list();
	}
}


/*
��ָ�����л�ȡ����ͷ
sBB_ERMsgBag:�������
sBBMsgHeadPack :�������,��Ž�����������Ϣͷ
*/
int GetBBRevMsgHeadFromBBMsg(sBBMsgArr sBBERMsgBag,sBBMsgHeaderPack *sBBMsgHeadPack)
{  
  int i = 0;
  unsigned short usMsgBodyAttrValue = 0;
  unsigned char *ucHeadTemp = NULL;
  unsigned char *pSrcBuf = sBBERMsgBag.ucMsgArr;

  ucHeadTemp = ++pSrcBuf;//��ȥ0x7e
  memset(sBBMsgHeadPack,0,sizeof(sBBMsgHeaderPack));  
  
  sBBMsgHeadPack->usMsgID = MAKEWORD(pSrcBuf[i],pSrcBuf[i+1]);//CMD_ID
  i+=2;      

  usMsgBodyAttrValue = MAKEWORD(pSrcBuf[i],pSrcBuf[i+1]);
  i+=2;   
  sBBMsgHeadPack->sMsgBodyAttr.Reserved = (unsigned char)(usMsgBodyAttrValue>>14);
  sBBMsgHeadPack->sMsgBodyAttr.SubPackage = (unsigned char)((usMsgBodyAttrValue & 0x4000)>>13);
  sBBMsgHeadPack->sMsgBodyAttr.DataEncryption = (unsigned char)((usMsgBodyAttrValue & 0x1C00)>>10);
  sBBMsgHeadPack->sMsgBodyAttr.MsgBodyLen = usMsgBodyAttrValue & 0x03FF;

  pSrcBuf+=i;
  memcpy(sBBMsgHeadPack->ucTerminalPhoneNm,pSrcBuf,MOBILE_PHONE_LEN);
  pSrcBuf = ucHeadTemp;
  i+=6;  
  
  sBBMsgHeadPack->usMsgSerialNm = MAKEWORD(pSrcBuf[i],pSrcBuf[i+1]); 
  //printf("sBBMsgHeadPack->usMsgSerialNm :%04x\n",sBBMsgHeadPack->usMsgSerialNm);
  i+=2; 
  
  if(sBBMsgHeadPack->sMsgBodyAttr.SubPackage == 0x01){
     sBBMsgHeadPack->sMsgPackItem.usMsgTotalPackageNm = MAKEWORD(pSrcBuf[i],pSrcBuf[i+1]);;
     i+=2;
     sBBMsgHeadPack->sMsgPackItem.usMsgPackIndex = MAKEWORD(pSrcBuf[i],pSrcBuf[i+1]);
     i+=2;
  }
  sBBMsgHeadPack->iMsgHeadLen = i;
  //ShowMsgHead(*sBBMsgHeadPack); 
  return 0;  
}


/*
���ݽ�����Ϣͷ��ȡ Ӧ����Ϣͷ
sBBAckMsgHeadPack:������������Ӧ����Ϣͷ
unsigned char ucSubBagFlag :���ذ��ְ���־
*/
int GetBBAckMsgHeadFromBBRevMsgHead(
    sBBMsgHeaderPack sBBMsgHeadPack,
    sBBMsgHeaderPack *sBBAckMsgHeadPack,
    unsigned char ucSubBagFlag,unsigned short usMsgBodyLen,
    unsigned short usMsgBagCnt,unsigned short usMsgBagIndex)
{
  memcpy(sBBAckMsgHeadPack,&sBBMsgHeadPack,sBBMsgHeadPack.iMsgHeadLen); //����ͬ���ͽṹ������ĸ���?

  sBBAckMsgHeadPack->iMsgHeadLen = sBBMsgHeadPack.iMsgHeadLen;
  sBBAckMsgHeadPack->usMsgID = (sBBMsgHeadPack.usMsgID) - 0x1000;
  
  if(sBBAckMsgHeadPack->usMsgID == CMD_REAL_TIME_AV_REQUEST){
    sBBAckMsgHeadPack->usMsgID = GENERAL_ACK_ID;
  }
  if(sBBAckMsgHeadPack->usMsgID == SEARCH_SOURCE_LIST_CMD){
    sBBAckMsgHeadPack->usMsgID = SEARCH_SOURCE_LIST_ACK_CMD;
  }  
  
  sBBAckMsgHeadPack->sMsgBodyAttr.MsgBodyLen = usMsgBodyLen;  
  sBBAckMsgHeadPack->sMsgBodyAttr.SubPackage = ucSubBagFlag;
    
  sBBAckMsgHeadPack->usMsgSerialNm = usResponseSerialNm++;  

  if(sBBAckMsgHeadPack->sMsgBodyAttr.SubPackage == 0x01){
     sBBAckMsgHeadPack->sMsgPackItem.usMsgPackIndex = usMsgBagIndex;
     sBBAckMsgHeadPack->sMsgPackItem.usMsgTotalPackageNm = usMsgBagCnt;
     sBBAckMsgHeadPack->iMsgHeadLen += 0x04;
  }
  return 0;
}

/*
������Ϣͷ
�ӽṹͷ-->����
���ذ�ͷ�ṹ�帴�Ƶ� ucResponseMsgHard ������Ϣͷ����  
*/
int GetAckArrFromBBAckMsgHead(
    sBBMsgArr *sMsgHeadArr,
    sBBMsgHeaderPack sBBAckMsgHeadPack)
{ 
  sMsgHeadArr->ucMsgArr[0] = HIBYTE(sBBAckMsgHeadPack.usMsgID);
  sMsgHeadArr->ucMsgArr[1] = LOBYTE(sBBAckMsgHeadPack.usMsgID);
  
  unsigned short usTemp = 0x00;
  usTemp = (sBBAckMsgHeadPack.sMsgBodyAttr.Reserved<<14)|(sBBAckMsgHeadPack.sMsgBodyAttr.SubPackage<<13)|
           (sBBAckMsgHeadPack.sMsgBodyAttr.DataEncryption<<10)|(sBBAckMsgHeadPack.sMsgBodyAttr.MsgBodyLen);
  sMsgHeadArr->ucMsgArr[2] =  HIBYTE(usTemp);//��������£����ذ�����Ϣͷ�е���Ϣ���Բ�һ������Ҫ������������Ϣ��ĳ��Ȳ�һ��
  sMsgHeadArr->ucMsgArr[3] =  LOBYTE(usTemp);
  
  memcpy(sMsgHeadArr->ucMsgArr+4,sBBAckMsgHeadPack.ucTerminalPhoneNm,sizeof(sBBAckMsgHeadPack.ucTerminalPhoneNm));
  sMsgHeadArr->ucMsgArr[10] = HIBYTE(sBBAckMsgHeadPack.usMsgSerialNm);
  sMsgHeadArr->ucMsgArr[11] = LOBYTE(sBBAckMsgHeadPack.usMsgSerialNm);

  if(sBBAckMsgHeadPack.sMsgBodyAttr.SubPackage == 0x01){
    sMsgHeadArr->ucMsgArr[12] = HIBYTE(sBBAckMsgHeadPack.sMsgPackItem.usMsgTotalPackageNm);
    sMsgHeadArr->ucMsgArr[13] = LOBYTE(sBBAckMsgHeadPack.sMsgPackItem.usMsgTotalPackageNm);
    
    sMsgHeadArr->ucMsgArr[14] = HIBYTE(sBBAckMsgHeadPack.sMsgPackItem.usMsgPackIndex);
    sMsgHeadArr->ucMsgArr[15] = LOBYTE(sBBAckMsgHeadPack.sMsgPackItem.usMsgPackIndex);

  }

  sMsgHeadArr->iMsgLen = sBBAckMsgHeadPack.iMsgHeadLen;
  //printf("sMsgHeadArr->iMsgLen:%d\n",sMsgHeadArr->iMsgLen);
  //ShowMsgHead(sBBAckMsgHeadPack); 
  return 0;
}

/*
sBBMsgArr sMsgAckHeadArr:���������
sBBMsgArr sMsgAckBodyArr:���������
sBBMsgArr sMsgArr :�������
*/
int GetAckPack(sBBMsgArr sMsgAckHeadArr,sBBMsgArr sMsgAckBodyArr,sBBMsgArr *sMsgArr)
{
    unsigned char ucResponseMsgBuff[BUBIAO_MAX_LEN] = {0};
    unsigned char *ucResponseEMsgBuff = sMsgArr->ucMsgArr;  
    unsigned char *ucResponseBodyBuff = sMsgAckBodyArr.ucMsgArr;
    unsigned char *ucAckMsgHeadArr    = sMsgAckHeadArr.ucMsgArr;
        
    unsigned char ucCheckCode = 0;    
    int iAckMsgHeadLen = sMsgAckHeadArr.iMsgLen;
    int iAckMsgBodyLen = sMsgAckBodyArr.iMsgLen;    
    int iAckMsgLen = 0;    
    
    ucResponseMsgBuff[iAckMsgLen] = ID_KEY;
    iAckMsgLen++;

    memcpy(ucResponseMsgBuff+1,ucAckMsgHeadArr,iAckMsgHeadLen);
    iAckMsgLen = iAckMsgHeadLen + iAckMsgLen;  

    memcpy(ucResponseMsgBuff+iAckMsgLen,ucResponseBodyBuff,iAckMsgBodyLen);          
    iAckMsgLen +=iAckMsgBodyLen;
    
    ucCheckCode = GetCheckCode(ucResponseMsgBuff,iAckMsgLen);    
    ucResponseMsgBuff[iAckMsgLen] = ucCheckCode;
    iAckMsgLen++;
    ucResponseMsgBuff[iAckMsgLen] = ID_KEY;   
    iAckMsgLen++;                        
    
    sMsgArr->iMsgLen = EscapeProcess(ucResponseMsgBuff,ucResponseEMsgBuff,iAckMsgLen);
    
    return 0;
}


/*
�洢ý���Ҫ��Ϣ��ѯ
*/
int MediaProfileInformQueryProcess(
   sBBMsgArr sBBMsgArrPack,
   sBBMsgHeaderPack sBBMsgHeadPack,
   s_BuBiaoTatoilResult *sTatoiResultPackage)
{
    int res = 0;
    unsigned char *pSrcBuf = sBBMsgArrPack.ucMsgArr; 
    int i = sBBMsgHeadPack.iMsgHeadLen + 1;      

    sTatoiResultPackage->sMediaProfileBody.ucMediaType = pSrcBuf[i];    
    i++;
    sTatoiResultPackage->sMediaProfileBody.ucChnID = pSrcBuf[i];
    i++;
            
    sTatoiResultPackage->sMediaProfileBody.usFileCnt = GetMediaProfileInform(sTatoiResultPackage->sMediaProfileBody.ucMediaType,sTatoiResultPackage);
   
    return res;
}

/*
�洢��ý�����ݼ���
*/
int MediaDataInformQueryProcess(
       sBBMsgArr sBBMsgArrPack,
       sBBMsgHeaderPack sBBMsgHeadPack,
       s_BuBiaoTatoilResult *sTatoiResultPackage)
{
    int i = 0;
    unsigned char *pSrcBuf = sBBMsgArrPack.ucMsgArr;

    sTatoiResultPackage->sResponsePack.usResponseSerialNm = sBBMsgHeadPack.usMsgSerialNm;
    i = sBBMsgHeadPack.iMsgHeadLen + 1;  
    
    sTatoiResultPackage->sDataSearchMsgBag.ucMediaType = pSrcBuf[i];    
    i++;
    
    sTatoiResultPackage->sDataSearchMsgBag.ucChnID = pSrcBuf[i];//�����ID ��Ĭ��Ϊ0x01
    i++;
    
    sTatoiResultPackage->sDataSearchMsgBag.uEventItemCode = pSrcBuf[i];
    i++;
     
    /*BCD6 to char[15]*/
    //161220180300  1612312020
    //memcpy(g_sDataSearchMsgBag.ucStartTime,pSrcBuf+i,6);//YY-MM-DD-hh-mm-ss
    //printf("start:%2x%2x%2x%2x%2x%2x",*(pSrcBuf+i),*(pSrcBuf+i+1),*(pSrcBuf+i+2),*(pSrcBuf+i+3),*(pSrcBuf+i+4),*(pSrcBuf+i+5));
    sprintf((char*)sTatoiResultPackage->sDataSearchMsgBag.ucStartTime,"20%02x%02x%02x%02x%02x%02x",
    *(pSrcBuf+i),*(pSrcBuf+i+1),*(pSrcBuf+i+2),*(pSrcBuf+i+3),*(pSrcBuf+i+4),*(pSrcBuf+i+5));    
    sTatoiResultPackage->sDataSearchMsgBag.ucStartTime[14] = '\0';
    printf("star time:%s\n",sTatoiResultPackage->sDataSearchMsgBag.ucStartTime);
    i=i+6;    

    sprintf((char*)sTatoiResultPackage->sDataSearchMsgBag.ucEndTime,"20%02x%02x%02x%02x%02x%02x",
    *(pSrcBuf+i),*(pSrcBuf+i+1),*(pSrcBuf+i+2),*(pSrcBuf+i+3),*(pSrcBuf+i+4),*(pSrcBuf+i+5));
    sTatoiResultPackage->sDataSearchMsgBag.ucEndTime[14] = '\0';
    printf("end time:%s\n",sTatoiResultPackage->sDataSearchMsgBag.ucEndTime);
    i=i+6;
     
    SearchMediaProcess(sTatoiResultPackage,sBBMsgHeadPack);   /*��ѯָ����������Ϣ*/    

    return 0;
}

/*
ƽ̨�·�Զ��ý���ϴ�����
ý���ϴ�ָ��� �����ϴ���Ƭ�Ͷ�����Ƶ
*/
int MediaUploadQueryProcess(sBBMsgArr sBBMsgArrPack,sBBMsgHeaderPack sBBMsgHeadPack,s_BuBiaoTatoilResult *sTatoiResultPackage)
{
    int i = 0;
    int iResult = 0;
    
    unsigned char *pSrcBuf = sBBMsgArrPack.ucMsgArr;
    
    i = sBBMsgHeadPack.iMsgHeadLen + 1;
    
    sTatoiResultPackage->sMediaUploadAttrBody.ucUploadActive = pSrcBuf[i];    
    i++;
    sTatoiResultPackage->sMediaUploadAttrBody.ucChnID = pSrcBuf[i];
    i++;
    
    sTatoiResultPackage->sMediaUploadAttrBody.ucFileNameLen = pSrcBuf[i];    
    i++;    
    memcpy(sTatoiResultPackage->sMediaUploadAttrBody.ucFileName,pSrcBuf+i,sTatoiResultPackage->sMediaUploadAttrBody.ucFileNameLen);
    i=i+sTatoiResultPackage->sMediaUploadAttrBody.ucFileNameLen;
    printf("ucFileName:%s\n",sTatoiResultPackage->sMediaUploadAttrBody.ucFileName);

    sTatoiResultPackage->sMediaUploadAttrBody.ucFileType = pSrcBuf[i];
    i++;

    unsigned char ucFullFileName[100] = {0};
    switch(sTatoiResultPackage->sMediaUploadAttrBody.ucFileType){
     case 0x01:
     case 0x02:
        sprintf((char*)ucFullFileName,"/mnt/mmc/PHOTO/%s",sTatoiResultPackage->sMediaUploadAttrBody.ucFileName);
        break;
     case 0x03:
     case 0x04:
        sprintf((char*)ucFullFileName,"/mnt/mmc/GVIDEO/%s",sTatoiResultPackage->sMediaUploadAttrBody.ucFileName);
        break;
    }

    sTatoiResultPackage->sMediaUploadAttrBody.ucUploadURLLen = pSrcBuf[i];
    i++;

    memcpy(sTatoiResultPackage->sMediaUploadAttrBody.ucUploadURL,pSrcBuf+i,sTatoiResultPackage->sMediaUploadAttrBody.ucUploadURLLen);
    i=i + sTatoiResultPackage->sMediaUploadAttrBody.ucUploadURLLen;
    printf("ucUploadURL:%s\n",sTatoiResultPackage->sMediaUploadAttrBody.ucUploadURL);
    
    if((sTatoiResultPackage->sMediaUploadAttrBody.ucFileType == 0x01)
        ||(sTatoiResultPackage->sMediaUploadAttrBody.ucFileType == 0x02)
        ||(sTatoiResultPackage->sMediaUploadAttrBody.ucFileType == 0x04)
        ){

        if(sTatoiResultPackage->sMediaUploadAttrBody.ucUploadActive == 0x01){
        iResult = cdr_ftp_upload(sTatoiResultPackage->sMediaUploadAttrBody.ucUploadURL,
            ucFullFileName,sBBMsgHeadPack,sTatoiResultPackage);
        }
        if(sTatoiResultPackage->sMediaUploadAttrBody.ucUploadActive == 0x00){
           //iResult = cdr_stop_ftp_upload();
           iResult = cdr_stop_ftp_upload_ex_ucChnID(sBBMsgHeadPack.usMsgID,sTatoiResultPackage->sMediaUploadAttrBody.ucChnID);
        }
        sTatoiResultPackage->sKakaAckPack.ucResult = iResult;
    }
    else if(sTatoiResultPackage->sMediaUploadAttrBody.ucFileType == 0x03){
        if(sTatoiResultPackage->sMediaUploadAttrBody.ucUploadActive == 0x01){
          iResult = cdr_ftp_upload(sTatoiResultPackage->sMediaUploadAttrBody.ucUploadURL,
            ucFullFileName,sBBMsgHeadPack,sTatoiResultPackage);
        }
        if(sTatoiResultPackage->sMediaUploadAttrBody.ucUploadActive == 0x00){
           //iResult = cdr_stop_ftp_upload();
           iResult = cdr_stop_ftp_upload_ex_ucChnID(sBBMsgHeadPack.usMsgID,sTatoiResultPackage->sMediaUploadAttrBody.ucChnID);
        }
        sTatoiResultPackage->sKakaAckPack.ucResult = iResult;        
    }
    return iResult;
}


int GetAreaMedia(s_AreaMediaUploadAttr sAreaMediaUploadAttrBody,
    s_BuBiaoTatoilResult *sTatoiResultPackage)
{
    DIR *directory_pointer = NULL;
    struct dirent *entry = NULL;
    int iCnt = 0;
    
    if((directory_pointer=opendir("/mnt/mmc/VIDEO"))==NULL){
       printf("Error opening %s %d\n",__FUNCTION__,__LINE__);
       return 0;
    }
    char sCreateTarFile[200] = {0};    
    sprintf(sCreateTarFile,"/mnt/mmc/VIDEO/%s",sAreaMediaUploadAttrBody.ucStartTime);
    mkdir(sCreateTarFile,0755);
    //printf("sCreateTarFile:%s\n",sCreateTarFile);
    sync();

    char sBufFileName[15] = {0};
    char sFullFileName[50] = {0};
    char sCopy2TarDir[200] = {0};
 
    while((entry=readdir(directory_pointer))!=NULL){
          if(strcmp(entry->d_name,".")==0 || strcmp(entry->d_name,"..")==0){  
           continue;
          }
          if(strstr(entry->d_name,"_pre")!=NULL){
           continue;      
          }
          if(strstr(entry->d_name,".MP4")!=NULL){
           continue;      
          }
          if(strstr(entry->d_name,".mp4")!=NULL){
           continue;      
          }          
          if(strstr(entry->d_name,".jpg")!=NULL){
              memset(sBufFileName,0,sizeof(sBufFileName));

              sscanf(entry->d_name,"%[^_]",sBufFileName); 
              
              if(compare_time(sAreaMediaUploadAttrBody.ucStartTime,
                (unsigned char *)sBufFileName,sAreaMediaUploadAttrBody.ucEndTime) == 1){
                 iCnt++;
                 memset(sFullFileName,0,sizeof(sFullFileName));
                 sprintf(sFullFileName,"/mnt/mmc/VIDEO/%s",entry->d_name);
                 printf("%s\n",sFullFileName);
                 sprintf(sCopy2TarDir,"cp -rf %s /mnt/mmc/VIDEO/%s",sFullFileName,
                    sAreaMediaUploadAttrBody.ucStartTime);                                 
                 cdr_system(sCopy2TarDir);                 
              }
          }
     }       
     if(directory_pointer!=NULL){
       closedir(directory_pointer);
       directory_pointer = NULL;
     }
     char sCmd[250] = {0};
     sprintf(sCmd,"tar -czf /mnt/mmc/VIDEO/%s.tar.gz -C /mnt/mmc/VIDEO %s",
        sAreaMediaUploadAttrBody.ucStartTime,sAreaMediaUploadAttrBody.ucStartTime);
     cdr_system(sCmd);
     sync();    //cdr_system("sync");
     sprintf(sTatoiResultPackage->sAreaMediaUploadAttrBody.sAreaMediaUploadFileName,
        "/mnt/mmc/VIDEO/%s.tar.gz",sAreaMediaUploadAttrBody.ucStartTime);

     //printf("sAreaMediaUploadFileName:%s\n",sTatoiResultPackage->sAreaMediaUploadAttrBody.sAreaMediaUploadFileName);
     return iCnt;
}

static sBBMsgHeaderPack g_sBBMsgHeadPackBuf;
static s_BuBiaoTatoilResult g_sTatoiResultPackageBuf;

int BB_mp4cut_finish(void* info,void *pUserData)
{
	cdr_ftp_upload_process_ex((char *)info,pUserData);
	if(pUserData) free(pUserData);	
	
	return 0;
}

/*
��ʱ���������Ƶ��Ԥ��ͼ�ϴ�
*/
int AreaMediaUploadQueryProcess(sBBMsgArr sBBMsgArrPack,
   sBBMsgHeaderPack sBBMsgHeadPack,s_BuBiaoTatoilResult *sTatoiResultPackage)
{
    int i = 0;
    int iResult = 0;
    unsigned char *pSrcBuf = sBBMsgArrPack.ucMsgArr;
    
    i = sBBMsgHeadPack.iMsgHeadLen + 1;
        
    sTatoiResultPackage->sAreaMediaUploadAttrBody.ucUploadActive = pSrcBuf[i];    
    i++;
    sTatoiResultPackage->sAreaMediaUploadAttrBody.ucChnID = pSrcBuf[i];
    i++;

    sprintf((char*)sTatoiResultPackage->sAreaMediaUploadAttrBody.ucStartTime,"20%02x%02x%02x%02x%02x%02x",
    *(pSrcBuf+i),*(pSrcBuf+i+1),*(pSrcBuf+i+2),*(pSrcBuf+i+3),*(pSrcBuf+i+4),*(pSrcBuf+i+5));
    sTatoiResultPackage->sAreaMediaUploadAttrBody.ucStartTime[14] = '\0';
    printf("sAreaMediaUploadAttrBody.ucStartTime:%s\n",sTatoiResultPackage->sAreaMediaUploadAttrBody.ucStartTime);
    i=i+6;

    sTatoiResultPackage->sAreaMediaUploadAttrBody.usAVTimeLen = MAKEWORD(pSrcBuf[i],pSrcBuf[i+1]); /*�Դ�ȥ���������ʱ��*/ 
    i+=2;   
    GetEndTime(&(sTatoiResultPackage->sAreaMediaUploadAttrBody));
    
    sTatoiResultPackage->sAreaMediaUploadAttrBody.ucFileType = pSrcBuf[i];
    i++;

    sTatoiResultPackage->sAreaMediaUploadAttrBody.ucUploadURLLen = pSrcBuf[i];
    i++;
    memcpy(sTatoiResultPackage->sAreaMediaUploadAttrBody.ucUploadURL,pSrcBuf+i,sTatoiResultPackage->sAreaMediaUploadAttrBody.ucUploadURLLen);
    i=i + sTatoiResultPackage->sAreaMediaUploadAttrBody.ucUploadURLLen;
    printf("ucUploadURL:%s\n",sTatoiResultPackage->sAreaMediaUploadAttrBody.ucUploadURL);

    if(sTatoiResultPackage->sAreaMediaUploadAttrBody.ucFileType == 0x02){//����Ԥ��ͼ��ѯ
        iResult = GetAreaMedia(sTatoiResultPackage->sAreaMediaUploadAttrBody,sTatoiResultPackage);
        if(iResult == 0x00){
           return 1;
        }

        if(sTatoiResultPackage->sAreaMediaUploadAttrBody.ucUploadActive == 0x01)
        { 
           //printf("%s,%s\n",sTatoiResultPackage->sAreaMediaUploadAttrBody.ucUploadURL,sTatoiResultPackage->sAreaMediaUploadAttrBody.sAreaMediaUploadFileName);
           iResult = cdr_ftp_upload(sTatoiResultPackage->sAreaMediaUploadAttrBody.ucUploadURL,
            (unsigned char*)sTatoiResultPackage->sAreaMediaUploadAttrBody.sAreaMediaUploadFileName
            ,sBBMsgHeadPack,sTatoiResultPackage);        
           if(iResult == 0x02){
             printf("�ϴ����\n");
             //RmmoveTmpFile(sTatoiResultPackage->sAreaMediaUploadAttrBody);
           }
		   RmmoveTmpFile(sTatoiResultPackage->sAreaMediaUploadAttrBody);
        }
        
        if(sTatoiResultPackage->sAreaMediaUploadAttrBody.ucUploadActive == 0x00){ 
           //iResult = cdr_stop_ftp_upload();
           iResult = cdr_stop_ftp_upload_ex_ucChnID(sBBMsgHeadPack.usMsgID,sTatoiResultPackage->sMediaUploadAttrBody.ucChnID);
        }     
        sTatoiResultPackage->sKakaAckPack.ucResult = iResult;          
    }
    if(sTatoiResultPackage->sAreaMediaUploadAttrBody.ucFileType == 0x01){
		if(sTatoiResultPackage->sAreaMediaUploadAttrBody.ucUploadActive == 0x01){ 
			g_usAreaMediaUploadFlag = 0x01;
			sTatoiResultPackage->ucMsgReturnFlag = 0x01;

			memcpy(&g_sBBMsgHeadPackBuf,&sBBMsgHeadPack,sizeof(sBBMsgHeaderPack));
			memcpy(&g_sTatoiResultPackageBuf,sTatoiResultPackage,sizeof(s_BuBiaoTatoilResult));  

			sBBUploadInfo *pUserData = calloc(1,sizeof(sBBUploadInfo));
			memcpy(&pUserData->pBBUploadInfoHeader,&sBBMsgHeadPack,sizeof(sBBMsgHeaderPack));
			memcpy(&pUserData->sTatoiResultPackageBuf,sTatoiResultPackage,sizeof(s_BuBiaoTatoilResult));        

			int nRet = cdr_read_mp4_ex((char*)sTatoiResultPackage->sAreaMediaUploadAttrBody.ucStartTime,
        			sTatoiResultPackage->sAreaMediaUploadAttrBody.usAVTimeLen,
        			MP4CUTOUTPATH,(stream_out_cb)&BB_mp4cut_finish,pUserData);
			if(nRet == -1){
			  sTatoiResultPackage->sKakaAckPack.ucResult = 0x01;
			  AckToPlatform(sBBMsgHeadPack,*sTatoiResultPackage);
			  free(pUserData);
               pUserData = NULL;
			}  
			memcpy(&g_sBBMsgHeadPackBuf,&sBBMsgHeadPack,sizeof(sBBMsgHeaderPack));
			memcpy(&g_sTatoiResultPackageBuf,sTatoiResultPackage,sizeof(s_BuBiaoTatoilResult));  
		}
		
        if(sTatoiResultPackage->sAreaMediaUploadAttrBody.ucUploadActive == 0x00){ 
           iResult = cdr_stop_ftp_upload_ex_ucChnID(sBBMsgHeadPack.usMsgID,sTatoiResultPackage->sMediaUploadAttrBody.ucChnID);
        } 
        
        sTatoiResultPackage->sKakaAckPack.ucResult = iResult;		
    }
    
    return iResult;
}


int SearchSourceListProcess(sBBMsgArr sBBMsgArrPack,
   sBBMsgHeaderPack sBBMsgHeadPack,s_BuBiaoTatoilResult *sTatoiResultPackage)
{
    int i = 0;
    int iCnt = 0;
    unsigned char *pSrcBuf = sBBMsgArrPack.ucMsgArr;    
    
    i = sBBMsgHeadPack.iMsgHeadLen + 1;
        
    sTatoiResultPackage->sSearchSourceListBody.ucLogicChnID = pSrcBuf[i];    
    i++;
    
    sprintf((char*)sTatoiResultPackage->sSearchSourceListBody.ucStartTime,"20%02x%02x%02x%02x%02x%02x",
    *(pSrcBuf+i),*(pSrcBuf+i+1),*(pSrcBuf+i+2),*(pSrcBuf+i+3),*(pSrcBuf+i+4),*(pSrcBuf+i+5));
    sTatoiResultPackage->sSearchSourceListBody.ucStartTime[14] = '\0';
    printf("ucStartTime:%s\n",sTatoiResultPackage->sSearchSourceListBody.ucStartTime);
    i=i+6;

    sprintf((char*)sTatoiResultPackage->sSearchSourceListBody.ucEndTime,"20%02x%02x%02x%02x%02x%02x",
    *(pSrcBuf+i),*(pSrcBuf+i+1),*(pSrcBuf+i+2),*(pSrcBuf+i+3),*(pSrcBuf+i+4),*(pSrcBuf+i+5));
    sTatoiResultPackage->sSearchSourceListBody.ucEndTime[14] = '\0';
    printf("ucEndTime:%s\n",sTatoiResultPackage->sSearchSourceListBody.ucEndTime);
    if(strcmp("20000000000000",(char*)sTatoiResultPackage->sSearchSourceListBody.ucEndTime)==0x00){
      memcpy(sTatoiResultPackage->sSearchSourceListBody.ucEndTime,"21000000000000",15);
    }
    i=i+6;
    
    memcpy(sTatoiResultPackage->sSearchSourceListBody.ucWarningMark,pSrcBuf+i,sizeof(sTatoiResultPackage->sSearchSourceListBody.ucWarningMark));
    i+=8;

    sTatoiResultPackage->sSearchSourceListBody.ucSourceType = pSrcBuf[i];
    i++;

    sTatoiResultPackage->sSearchSourceListBody.ucStreamType = pSrcBuf[i];
    i++;

    sTatoiResultPackage->sSearchSourceListBody.ucStoreType = pSrcBuf[i];
    i++;

    if(sTatoiResultPackage->sSearchSourceListBody.ucSourceType == 0x00){//��ѯ����Ƶ��ָ������Դ

       memcpy(sTatoiResultPackage->sDataSearchMsgBag.ucStartTime,sTatoiResultPackage->sSearchSourceListBody.ucStartTime,
       sizeof(sTatoiResultPackage->sSearchSourceListBody.ucStartTime));
       memcpy(sTatoiResultPackage->sDataSearchMsgBag.ucEndTime,sTatoiResultPackage->sSearchSourceListBody.ucEndTime,
       sizeof(sTatoiResultPackage->sSearchSourceListBody.ucEndTime));
       
       iCnt = SearchMediaProcessCore("/mnt/mmc/VIDEO",sTatoiResultPackage,sBBMsgHeadPack);
    }
    return iCnt;
}

int VideoPlayBackProcess(sBBMsgArr sBBMsgArrPack,
    sBBMsgHeaderPack sBBMsgHeadPack,s_BuBiaoTatoilResult *sTatoiResultPackage)
{
    int i = 0;
    int j = 0;
    unsigned char *pSrcBuf = sBBMsgArrPack.ucMsgArr;
    unsigned char * ucHeadTemp = pSrcBuf;

    i = sBBMsgHeadPack.iMsgHeadLen + 1 ;

    sTatoiResultPackage->sVideoPlayBackBody.ucServerIPAddrLen = pSrcBuf[i];
    i++;

    pSrcBuf+=i;
    for(j=0;j<sTatoiResultPackage->sVideoPlayBackBody.ucServerIPAddrLen;j++)
    {
     sTatoiResultPackage->sVideoPlayBackBody.usServerIPAddr[j] = *pSrcBuf++;
    }  
    sTatoiResultPackage->sVideoPlayBackBody.usServerIPAddr[sTatoiResultPackage->sVideoPlayBackBody.ucServerIPAddrLen] = '\0';

    printf("usServerIPAddr:%s\n",sTatoiResultPackage->sVideoPlayBackBody.usServerIPAddr);

    pSrcBuf = ucHeadTemp;
    i+=(sTatoiResultPackage->sVideoPlayBackBody).ucServerIPAddrLen;
    
    sTatoiResultPackage->sVideoPlayBackBody.usServerTCPListenPort = MAKEWORD(pSrcBuf[i],pSrcBuf[i+1]);
    i+=2;
    
    sTatoiResultPackage->sVideoPlayBackBody.usServerUDPListenPort = MAKEWORD(pSrcBuf[i],pSrcBuf[i+1]);
    i+=2;
    
    sTatoiResultPackage->sVideoPlayBackBody.ucAVChanlNm = pSrcBuf[i];//0x02 ��ֵ ������ǰ��
    i+=1;
    
    sTatoiResultPackage->sVideoPlayBackBody.ucMediumType = pSrcBuf[i];//0x01 av 0x02: picture
    i+=1;
    
    sTatoiResultPackage->sVideoPlayBackBody.ucStreamType = pSrcBuf[i];  
    i+=1;
    
    sTatoiResultPackage->sVideoPlayBackBody.ucPlayBackWay = pSrcBuf[i];  //�طŷ�ʽ
    i+=1;
    
    sTatoiResultPackage->sVideoPlayBackBody.ucSpeedNum = pSrcBuf[i];
    i+=1;

    sprintf((char*)sTatoiResultPackage->sVideoPlayBackBody.ucStartTime,"20%02x%02x%02x%02x%02x%02x",
    *(pSrcBuf+i),*(pSrcBuf+i+1),*(pSrcBuf+i+2),*(pSrcBuf+i+3),*(pSrcBuf+i+4),*(pSrcBuf+i+5));
    sTatoiResultPackage->sVideoPlayBackBody.ucStartTime[14] = '\0';
    printf("ucStartTime:%s\n",sTatoiResultPackage->sVideoPlayBackBody.ucStartTime);
    i=i+6;

    sprintf((char*)sTatoiResultPackage->sVideoPlayBackBody.ucEndTime,"20%02x%02x%02x%02x%02x%02x",
    *(pSrcBuf+i),*(pSrcBuf+i+1),*(pSrcBuf+i+2),*(pSrcBuf+i+3),*(pSrcBuf+i+4),*(pSrcBuf+i+5));
    sTatoiResultPackage->sVideoPlayBackBody.ucEndTime[14] = '\0';
    printf("ucStartTime:%s\n",sTatoiResultPackage->sVideoPlayBackBody.ucStartTime);
    i=i+6;

    switch(sTatoiResultPackage->sVideoPlayBackBody.ucMediumType){
       case 0x00:break;  
       case 0x01:
        //�طſ��ƴ���
        break;    
    }   

    return 0;
}


void BuBiaoMsgBusyProcess(sBBMsgArr *sBBMsgArrPack)
{   
    sBBMsgArr sMsgHeadArr,sMsgBodyArr,sMsgArr;   
    sBBMsgHeaderPack sBBMsgHeadPack,sBBAckMsgHeadPack;
    
    unsigned char ucResponseMsgHard[BUBIAO_HEAD_MAX_LEN] = {0};
    unsigned char ucResponseMsgBody[BUBIAO_MAX_LEN-BUBIAO_HEAD_MAX_LEN-2] = {0};
    unsigned char ucBuBiaoAckBag[BUBIAO_MAX_LEN] = {0};
   
    BBEscapeReduction(sBBMsgArrPack);
    if(-1 == BBCheckCodeProcess(*sBBMsgArrPack)) return;    
    GetBBRevMsgHeadFromBBMsg(*sBBMsgArrPack,&sBBMsgHeadPack);        
        
    sMsgBodyArr.ucMsgArr = &ucResponseMsgBody[0];
    sMsgBodyArr.ucMsgArr[sMsgBodyArr.iMsgLen] = HIBYTE(usResponseSerialNm);
    sMsgBodyArr.iMsgLen++;
    sMsgBodyArr.ucMsgArr[sMsgBodyArr.iMsgLen] = LOBYTE(usResponseSerialNm);
    sMsgBodyArr.iMsgLen++; 
    sMsgBodyArr.ucMsgArr[sMsgBodyArr.iMsgLen] = 0x05;//������״̬æ
    sMsgBodyArr.iMsgLen++; 

    GetBBAckMsgHeadFromBBRevMsgHead(sBBMsgHeadPack,
        &sBBAckMsgHeadPack,GetSubBagFlag(sMsgBodyArr.iMsgLen),sMsgBodyArr.iMsgLen,0,0);
    sMsgHeadArr.ucMsgArr = &ucResponseMsgHard[0];
    GetAckArrFromBBAckMsgHead(&sMsgHeadArr,sBBAckMsgHeadPack);    

    sMsgArr.ucMsgArr = &ucBuBiaoAckBag[0];
    GetAckPack(sMsgHeadArr,sMsgBodyArr,&sMsgArr);    
    BagQueueAddData(&stRetuBagQueue, sMsgArr.ucMsgArr, sMsgArr.iMsgLen);
}


void cdr_ftp_upload_process(char *file_name)
{
   int iResult = 0x00;
   if(g_sTatoiResultPackageBuf.sAreaMediaUploadAttrBody.ucUploadActive == 0x01){ 
      iResult = cdr_ftp_upload(g_sTatoiResultPackageBuf.sAreaMediaUploadAttrBody.ucUploadURL,
        (unsigned char*)file_name,g_sBBMsgHeadPackBuf,&g_sTatoiResultPackageBuf);
   }else{
     //printf("ֹͣ�ϴ�ָ��\n");
     iResult = cdr_stop_ftp_upload();//3��ֹͣ�ɹ���4��ֹͣʧ��
   }   
    //printf("�ϴ���� iResult:%d\n",iResult);
    g_sTatoiResultPackageBuf.sKakaAckPack.ucResult = iResult;
    AckToPlatform(g_sBBMsgHeadPackBuf,g_sTatoiResultPackageBuf);

}
void cdr_ftp_upload_process_ex(char *file_name,void *pUserData)
{
	sBBUploadInfo *p = (sBBUploadInfo*)pUserData;
	
	int iResult = 0x00;
	if(p->sTatoiResultPackageBuf.sAreaMediaUploadAttrBody.ucUploadActive == 0x01)
     { 
	  iResult = cdr_ftp_upload(p->sTatoiResultPackageBuf.sAreaMediaUploadAttrBody.ucUploadURL,
	    (unsigned char*)file_name,p->pBBUploadInfoHeader,&p->sTatoiResultPackageBuf);
	}
	p->sTatoiResultPackageBuf.sKakaAckPack.ucResult = iResult;
	AckToPlatform(p->pBBUploadInfoHeader,p->sTatoiResultPackageBuf);
}



void AckToPlatform(sBBMsgHeaderPack sBBMsgHeadPack,s_BuBiaoTatoilResult sTatoiResultPackage)
{
    sBBMsgArr sMsgHeadArr,sMsgBodyArr,sMsgArr;
    sBBMsgHeaderPack sBBAckMsgHeadPack;
    
    unsigned char ucResponseMsgHard[BUBIAO_HEAD_MAX_LEN] = {0};
    unsigned char ucResponseMsgBody[BUBIAO_MAX_LEN-BUBIAO_HEAD_MAX_LEN-2] = {0};
    unsigned char ucBuBiaoAckBag[BUBIAO_MAX_LEN] = {0};            

    sMsgBodyArr.ucMsgArr = &ucResponseMsgBody[0];
    GetMsgBodyPackage(&sMsgBodyArr,sBBMsgHeadPack,sTatoiResultPackage);   
        
    GetBBAckMsgHeadFromBBRevMsgHead(sBBMsgHeadPack,
        &sBBAckMsgHeadPack,GetSubBagFlag(sMsgBodyArr.iMsgLen),sMsgBodyArr.iMsgLen,0,0);
    sMsgHeadArr.ucMsgArr = &ucResponseMsgHard[0];
    GetAckArrFromBBAckMsgHead(&sMsgHeadArr,sBBAckMsgHeadPack);    
    
    sMsgArr.ucMsgArr = &ucBuBiaoAckBag[0];
    GetAckPack(sMsgHeadArr,sMsgBodyArr,&sMsgArr);    
    BagQueueAddData(&stRetuBagQueue, sMsgArr.ucMsgArr, sMsgArr.iMsgLen); 
}


