/************************************************************************	
** Filename: 	cdr_comm.c
** Description:  
** Author: 	xjl
** Create Date: 
** Version: 	v1.0

	Copyright(C) 2016 e-eye CO.LTD. ShenZhen <www.e-eye.cn>

*************************************************************************/
#include<stdio.h>  
#include<stdlib.h>  
#include"malloc.h"  
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>  
#include "cdr_count_file.h"
#include "cdr_bubiao_analyze.h"
#include "cdr_bubiao.h"
#include "cdr_exif.h"
#include "queue_bag.h"
#include "cdr_comm.h"


//��ǰ�ļ������ʱ���Ƿ���ָ����ʱ�䷶Χ��
int compare_time(unsigned char *pTimeStart,unsigned char *pTm2,unsigned char *pTimeEnd)
{
	time_t settm1 = 0;
	settm1 = _strtotime((char *)pTimeStart);	

    	time_t settm2 = 0;
	settm2 = _strtotime((char *)pTm2);

    time_t settm3 = 0;
	settm3 = _strtotime((char *)pTimeEnd);

    if((settm2>=settm1)&&(settm2<=settm3)) return 1;
   
	return 0;
}


int GetFileSize(char* filename)  
{  
    struct stat statbuf;  
    stat(filename,&statbuf);  
    int size=statbuf.st_size;    
    return size;  
} 

//����޸�ʱ��
int GetFileCreateTime(char *sFullFileName,unsigned char *pFileCreateTime)
{
   struct stat buf;
   int result = stat(sFullFileName, &buf);    //����ļ�״̬��Ϣ
   
   if( result != 0 ){
     perror("��ʾ�ļ�״̬��Ϣ����");
   }else{        
     struct tm *p;
     p = localtime(&buf.st_ctime); 
     //printf("%d/%d/%d %d:%d:%d\n", 1900 + p->tm_year, 1 + p->tm_mon, p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec);
     //printf("�ļ�����ʱ��:%s\n",ctime(&buf.st_ctime));        
     sprintf((char*)pFileCreateTime,"%04d%02d%02d%02d%02d%02d",1900 + p->tm_year, 1 + p->tm_mon, p->tm_mday,
     p->tm_hour, p->tm_min, p->tm_sec);
   }   
   return 0;
}

/*
��ȡ����������ý����Ŀ
*/
int GetMediaCnt(const char *pDir,s_BuBiaoTatoilResult *sTatoiResultPackage)
{
    int iCount = 0;
    DIR *directory_pointer;
    struct dirent *entry;
    
    //printf("pDir:%s\n",pDir);
    
    if(strcmp("/mnt/mmc/GVIDEO",pDir)==0x00){
        if((directory_pointer=opendir("/mnt/mmc/PHOTO"))==NULL){
           printf("Error opening %s\n",pDir);
           return 0;
        }
        while ((entry=readdir(directory_pointer))!=NULL){
          if(strcmp(entry->d_name,".")==0 || strcmp(entry->d_name,"..")==0){    ///current dir OR parrent dir
           continue;
          }
          if(strstr(entry->d_name,"_pre")!=NULL){//_pre.jpgȥ�� ��ͼ��Ԥ��ͼ
           continue;      
          }  
          if(strstr(entry->d_name,"_")!=NULL){//��ײͼƬ?
           continue;      
          }
          if(strstr(entry->d_name,"G")!=NULL)
          {            
              char sBufFileNameTmp[16] = {0};
              char sBufFileName[15] = {0};
              sscanf(entry->d_name,"%[^.]",sBufFileNameTmp);
              substring(sBufFileNameTmp,1,14,sBufFileName);  
              if(compare_time(sTatoiResultPackage->sDataSearchMsgBag.ucStartTime,
                (unsigned char *)sBufFileName,sTatoiResultPackage->sDataSearchMsgBag.ucEndTime) == 1){
                iCount++;                      
                printf("GFileName:%s\n",entry->d_name);               
              } 
          }
       } 
       sTatoiResultPackage->sDataSearchMsgBag.usPreBagMediaDataTatoil = iCount; 
    }else{
        if((directory_pointer=opendir(pDir))==NULL){
           printf("Error opening %s\n",pDir);
           return 0;
        }
    }
    
    if(strcmp("/mnt/mmc/PHOTO",pDir)==0x00)
    {        
        while ((entry=readdir(directory_pointer))!=NULL){
          if(strcmp(entry->d_name,".")==0 || strcmp(entry->d_name,"..")==0){    ///current dir OR parrent dir
           continue;
          }
          if(strstr(entry->d_name,"_pre")!=NULL){//_pre.jpgȥ�� ��ͼ��Ԥ��ͼ
           continue;      
          }  
          if(strstr(entry->d_name,"_")!=NULL){//��ײͼƬ? xxxx_101.jpg
           continue;      
          }
          if(strstr(entry->d_name,"G")!=NULL){
           continue;      
          }
          char sBufFileName[15] = {0};
          sscanf(entry->d_name,"%14s",sBufFileName);
          if(compare_time(sTatoiResultPackage->sDataSearchMsgBag.ucStartTime,
            (unsigned char *)sBufFileName,sTatoiResultPackage->sDataSearchMsgBag.ucEndTime) == 1){
            printf("Photo File Name:%s\n",entry->d_name);
            iCount++;                      
          }                
       } 
       sTatoiResultPackage->sDataSearchMsgBag.usPreBagMediaDataTatoil = iCount;  
    }
    if(strcmp("/mnt/mmc/VIDEO",pDir)==0x00)
    {        
        while ((entry=readdir(directory_pointer))!=NULL){
          if(strcmp(entry->d_name,".")==0 || strcmp(entry->d_name,"..")==0){    ///current dir OR parrent dir
           continue;
          }
          if(strstr(entry->d_name,"_pre")!=NULL){//_pre.jpgȥ�� ��ͼ��Ԥ��ͼ
           continue;      
          } 
          if(strstr(entry->d_name,".MP4")!=NULL){
              char sBufFileName[15] = {0};
              sscanf(entry->d_name,"%14s",sBufFileName);
              printf("Mp4FileName:%s\n",entry->d_name); 
              if(compare_time(sTatoiResultPackage->sDataSearchMsgBag.ucStartTime,
                (unsigned char *)sBufFileName,sTatoiResultPackage->sDataSearchMsgBag.ucEndTime) == 1){
                //printf("Mp4FileName:%s\n",entry->d_name);               
                iCount++;                      
              }                
          }
       } 
       sTatoiResultPackage->sDataSearchMsgBag.usPreBagMediaDataTatoil = iCount;  
    }    
   return iCount;
}

extern unsigned char g_ucSubBagFlag;
/*
ͳ���ļ�����Ŀ
�ܳ��� :1024  
12+4+2 header 18
��Ϣ��:2+N*35

һ�������28 ��ͼ����Ϣ����
��86 �洢��ý�����ݼ���Ӧ����Ϣ�����ݸ�ʽ
��ʼ�ֽ�	�ֶ�	��������	������Ҫ��
0	Ӧ����ˮ��	WORD 	��Ӧ�Ķ�ý�����ݼ�����Ϣ����ˮ��
2	��ý������������	WORD 	������������Ķ�ý������������
4	������		��ý����������ݸ�ʽ����87

��87 ��ý����������ݸ�ʽ
��ʼ�ֽ�	�ֶ�	��������	������Ҫ��
0	��ý��ID	DWORD	>0
4	��ý������	BYTE	0��ͼ��1����Ƶ��2����Ƶ
5	 ͨ��ID	BYTE	
6	�¼������	BYTE	0��ƽ̨�·�ָ�1����ʱ������2�����ٱ���������3����ײ�෭������������������
7	λ����Ϣ�㱨
(0x0200)��Ϣ��	BYTE[28]	��ʾ�����¼�Ƶ���ʼʱ�̵�λ�û�����Ϣ����

�������ɶ��  ��ý����������� ��ɣ�
ÿ�����ذ������(28)PER_BAG_MAX_ITEMS_CNT ��  ��ý�����������

��ѯָ���ļ�����ͼƬ����ϸ��Ϣ����� g_sSearchTotailPictBuf[5]��
��������5���ͷ��ص��ƶ���


������ѯ����ͼƬ����Ϣ
*/
int SearchMediaProcessCore(const char *pDir,s_BuBiaoTatoilResult *sTatoiResultPackage,
    sBBMsgHeaderPack sBBMsgHeadPack)
{
    DIR *directory_pointer;
    struct dirent *entry;
    int iCount = 0;                   //��������ͼƬ�����Ƿ���һ�����ذ�
    int iCntIndex = 0;                //������ Ϊ��¼�Ƿ���������е�ͼƬ
    unsigned short usMsgBagIndex = 0; //��ǰΪ�ڼ��η���
    unsigned short usMsgBagCnt = 0;   //���ص��ƶ��˵��ܴ���

    sBBMsgArr sMsgHeadArr,sMsgBodyArr,sMsgArr;   
    sBBMsgHeaderPack sBBAckMsgHeadPack;

    if((sTatoiResultPackage->sDataSearchMsgBag.ucStartTime == NULL) 
        || (sTatoiResultPackage->sDataSearchMsgBag.ucEndTime == NULL)){
       printf("Not set start and end time \n");
       return 0;
    } 

    sTatoiResultPackage->sDataSearchMsgBag.usMediaDataTatoil = GetMediaCnt(pDir,sTatoiResultPackage);
    if(sTatoiResultPackage->sDataSearchMsgBag.usMediaDataTatoil == 0x00){
        printf("usMediaDataTatoil is null\n");
        return -1;
    }
    
    if((sTatoiResultPackage->sDataSearchMsgBag.usMediaDataTatoil)%PER_BAG_MAX_ITEMS_CNT == 0x00){
        usMsgBagCnt = (sTatoiResultPackage->sDataSearchMsgBag.usMediaDataTatoil)/PER_BAG_MAX_ITEMS_CNT;
    }else{
        usMsgBagCnt = (sTatoiResultPackage->sDataSearchMsgBag.usMediaDataTatoil)/PER_BAG_MAX_ITEMS_CNT + 1;
    }
    sTatoiResultPackage->sDataSearchMsgBag.usMediaBagCnt = usMsgBagCnt;
    printf("usMsgBagCnt:%d usMediaDataTatoil:%d\n",usMsgBagCnt,sTatoiResultPackage->sDataSearchMsgBag.usMediaDataTatoil);
    if(usMsgBagCnt > 0x01){
       g_ucSubBagFlag = 0x01;
    }else{
       g_ucSubBagFlag = 0x00;
    }

    memset(g_sSearchTotailPictBuf,0,sizeof(g_sSearchTotailPictBuf));   
    if(strcmp("/mnt/mmc/GVIDEO",pDir)==0x00)
    {
        if((directory_pointer=opendir("/mnt/mmc/PHOTO"))==NULL){
           printf("Error opening %s\n",pDir);
           return 0;
        }
        while ((entry=readdir(directory_pointer))!=NULL){
          if(strcmp(entry->d_name,".")==0 || strcmp(entry->d_name,"..")==0){  
           continue;
          }
          if(strstr(entry->d_name,"_pre")!=NULL){
           continue;      
          }      
          if(strstr(entry->d_name,".jpg")!=NULL){
           continue;      
          }                
          if(strstr(entry->d_name,"G")!=NULL){

              char sBufFileNameTmp[16] = {0};
              char sBufFileName[15] = {0};
              //sscanf(entry->d_name,"%14s",sBufFileName); 
              sscanf(entry->d_name,"%[^.]",sBufFileNameTmp);
              substring(sBufFileNameTmp,1,14,sBufFileName);
                    
              if(compare_time(sTatoiResultPackage->sDataSearchMsgBag.ucStartTime,
                (unsigned char *)sBufFileName,sTatoiResultPackage->sDataSearchMsgBag.ucEndTime) == 1){

                char sFullFileName[100] = {0};
                sprintf(sFullFileName,"%s/%s",pDir,entry->d_name);

                g_sSearchTotailPictBuf[iCount].ucFileNameLen = strlen(sFullFileName);
                memcpy(g_sSearchTotailPictBuf[iCount].ucFullFileNameBuf,sFullFileName,strlen(sFullFileName)+1);        
                g_sSearchTotailPictBuf[iCount].uiFileSize = GetFileSize(sFullFileName);
                GetFileCreateTime(sFullFileName,g_sSearchTotailPictBuf[iCount].ucFileTime);

                test_read_exif(sFullFileName,iCount);
                iCount++;
                iCntIndex++;
                g_sSearchTotailPictBuf[iCount].iIndex = iCount;
              }
              
              if(iCount == PER_BAG_MAX_ITEMS_CNT || iCntIndex >= sTatoiResultPackage->sDataSearchMsgBag.usMediaDataTatoil){
                sTatoiResultPackage->sDataSearchMsgBag.usPreBagMediaDataTatoil = iCount;            
                
                unsigned char ucResponseMsgHard[BUBIAO_HEAD_MAX_LEN] = {0};
                unsigned char ucResponseMsgBody[BUBIAO_MAX_LEN-BUBIAO_HEAD_MAX_LEN-2] = {0};
                unsigned char ucBuBiaoAckBag[BUBIAO_MAX_LEN] = {0};            

                iCount = 0x00;
                usMsgBagIndex++;
                sTatoiResultPackage->pSearchTotailMediaItemBuf = &g_sSearchTotailPictBuf[0];

                sMsgBodyArr.ucMsgArr = &ucResponseMsgBody[0];
                GetMsgBodyPackage(&sMsgBodyArr,sBBMsgHeadPack,*sTatoiResultPackage);   

                GetBBAckMsgHeadFromBBRevMsgHead(sBBMsgHeadPack,
                &sBBAckMsgHeadPack,g_ucSubBagFlag,sMsgBodyArr.iMsgLen,usMsgBagCnt,usMsgBagIndex);
                sMsgHeadArr.ucMsgArr = &ucResponseMsgHard[0];
                GetAckArrFromBBAckMsgHead(&sMsgHeadArr,sBBAckMsgHeadPack);    

                sMsgArr.ucMsgArr = &ucBuBiaoAckBag[0];
                GetAckPack(sMsgHeadArr,sMsgBodyArr,&sMsgArr);    
                BagQueueAddData(&stRetuBagQueue, sMsgArr.ucMsgArr, sMsgArr.iMsgLen);
                sTatoiResultPackage->ucMsgReturnFlag = 0x01;
                memset(g_sSearchTotailPictBuf,0,sizeof(sSearchTotailMediaItemBuf));
              }       
           }
           if(iCntIndex >= sTatoiResultPackage->sDataSearchMsgBag.usMediaDataTatoil)break;
        }
        
    }else{
        if((directory_pointer=opendir(pDir))==NULL){
           printf("Error opening %s\n",pDir);
           return 0;
        }
    }
    
    if(strcmp("/mnt/mmc/VIDEO",pDir)==0x00){
       while ((entry=readdir(directory_pointer))!=NULL){
          if(strcmp(entry->d_name,".")==0 || strcmp(entry->d_name,"..")==0){
           continue;
          }
          if(strstr(entry->d_name,".jpg")!=NULL){
              char sBufFileName[15] = {0};
              sscanf(entry->d_name,"%14s",sBufFileName); 
                    
              if(compare_time(sTatoiResultPackage->sDataSearchMsgBag.ucStartTime,
                (unsigned char *)sBufFileName,sTatoiResultPackage->sDataSearchMsgBag.ucEndTime) == 1){

                char sFullFileName[100] = {0};//.jpg
                sprintf(sFullFileName,"%s/%s",pDir,entry->d_name);
                g_sSearchTotailPictBuf[iCount].ucFileNameLen = strlen(sFullFileName);
                char sFullMP4FileNameTemp[100] = {0};
                char sFullMP4FileName[100] = {0};
                sscanf(sFullFileName,"%[^.]",sFullMP4FileNameTemp);
                sprintf(sFullMP4FileName,"%s.MP4",sFullMP4FileNameTemp);                
                memcpy(g_sSearchTotailPictBuf[iCount].ucFullFileNameBuf,sFullMP4FileName,strlen(sFullFileName)+1);        
                
                g_sSearchTotailPictBuf[iCount].uiFileSize = GetFileSize(sFullMP4FileName);
                GetFileCreateTime(sFullMP4FileName,g_sSearchTotailPictBuf[iCount].ucFileTime);


                g_sSearchSourceBuf[iCount].ucLogicChnID = 0x02;//��ֵ
                sscanf(entry->d_name,"%[^_]",g_sSearchSourceBuf[iCount].ucFileStartTime);
                printf("ucFileStartTime:%s\n",g_sSearchSourceBuf[iCount].ucFileStartTime);
                strcpy((char*)g_sSearchSourceBuf[iCount].ucFileEndTime,(char*)g_sSearchTotailPictBuf[iCount].ucFileTime);          
                memset((char*)g_sSearchSourceBuf[iCount].ucWarningMark,0,sizeof(g_sSearchSourceBuf[iCount].ucWarningMark));
                g_sSearchSourceBuf[iCount].ucSourceType = sTatoiResultPackage->sSearchSourceListBody.ucSourceType;
                g_sSearchSourceBuf[iCount].ucStreamType = sTatoiResultPackage->sSearchSourceListBody.ucStreamType ;
                g_sSearchSourceBuf[iCount].ucStoreType = sTatoiResultPackage->sSearchSourceListBody.ucStoreType;
                g_sSearchSourceBuf[iCount].uiFileSize = g_sSearchTotailPictBuf[iCount].uiFileSize;
      
                //test_read_exif(sFullFileName,iCount);//û��д��,ȫΪ0
                iCount++;
                iCntIndex++;
                g_sSearchTotailPictBuf[iCount].iIndex = iCount;
                sTatoiResultPackage->sDataSearchMsgBag.usPreBagMediaDataTatoil = iCount;            
              }
              
              if(iCount == PER_BAG_MAX_ITEMS_CNT || iCntIndex >= sTatoiResultPackage->sDataSearchMsgBag.usMediaDataTatoil){
                unsigned char ucResponseMsgHard[BUBIAO_HEAD_MAX_LEN] = {0};
                unsigned char ucResponseMsgBody[BUBIAO_MAX_LEN-BUBIAO_HEAD_MAX_LEN-2] = {0};
                unsigned char ucBuBiaoAckBag[BUBIAO_MAX_LEN] = {0};            

                iCount = 0x00;
                usMsgBagIndex++;
                sTatoiResultPackage->pSearchTotailMediaItemBuf = &g_sSearchTotailPictBuf[0];

                sMsgBodyArr.ucMsgArr = &ucResponseMsgBody[0];
                GetMsgBodyPackage(&sMsgBodyArr,sBBMsgHeadPack,*sTatoiResultPackage);   

                GetBBAckMsgHeadFromBBRevMsgHead(sBBMsgHeadPack,
                &sBBAckMsgHeadPack,g_ucSubBagFlag,sMsgBodyArr.iMsgLen,usMsgBagCnt,usMsgBagIndex);
                sMsgHeadArr.ucMsgArr = &ucResponseMsgHard[0];
                GetAckArrFromBBAckMsgHead(&sMsgHeadArr,sBBAckMsgHeadPack);    

                sMsgArr.ucMsgArr = &ucBuBiaoAckBag[0];
                GetAckPack(sMsgHeadArr,sMsgBodyArr,&sMsgArr);    
                BagQueueAddData(&stRetuBagQueue, sMsgArr.ucMsgArr, sMsgArr.iMsgLen);
                sTatoiResultPackage->ucMsgReturnFlag = 0x01;
                memset(g_sSearchTotailPictBuf,0,sizeof(g_sSearchTotailPictBuf));
              }
              if(iCntIndex >= sTatoiResultPackage->sDataSearchMsgBag.usMediaDataTatoil)break;
          }
       }
    }

    if(strcmp("/mnt/mmc/PHOTO",pDir)==0x00)
    {        
        while ((entry=readdir(directory_pointer))!=NULL){
          if(strcmp(entry->d_name,".")==0 || strcmp(entry->d_name,"..")==0){   
           continue;
          }
          if(strstr(entry->d_name,"_pre")!=NULL){
           continue;      
          }  
          if(strstr(entry->d_name,"_")!=NULL){
           continue;      
          }
          if(strstr(entry->d_name,"G")!=NULL){
           continue;      
          }
          char sBufFileName[15] = {0};
          sscanf(entry->d_name,"%14s",sBufFileName);
          if(compare_time(sTatoiResultPackage->sDataSearchMsgBag.ucStartTime,
            (unsigned char *)sBufFileName,sTatoiResultPackage->sDataSearchMsgBag.ucEndTime) == 1){

            char sFullFileName[100] = {0};
            sprintf(sFullFileName,"%s/%s",pDir,entry->d_name);         
            g_sSearchTotailPictBuf[iCount].ucFileNameLen = strlen(sFullFileName);
            memcpy(g_sSearchTotailPictBuf[iCount].ucFullFileNameBuf,sFullFileName,strlen(sFullFileName)+1);        
            g_sSearchTotailPictBuf[iCount].uiFileSize = GetFileSize(sFullFileName);
            GetFileCreateTime(sFullFileName,g_sSearchTotailPictBuf[iCount].ucFileTime);
            test_read_exif(sFullFileName,iCount);//��ȡλ�ü���γ��Ϣ ͨ���ļ���/mnt/mmc/xxx.jpg exif 
            
            iCount++;
            iCntIndex++;
            g_sSearchTotailPictBuf[iCount].iIndex = iCount;
            sTatoiResultPackage->sDataSearchMsgBag.usPreBagMediaDataTatoil = iCount;            
          }          
          if(iCount == PER_BAG_MAX_ITEMS_CNT || iCntIndex >= sTatoiResultPackage->sDataSearchMsgBag.usMediaDataTatoil)
          {
            unsigned char ucResponseMsgHard[BUBIAO_HEAD_MAX_LEN] = {0};
            unsigned char ucResponseMsgBody[BUBIAO_MAX_LEN-BUBIAO_HEAD_MAX_LEN-2] = {0};
            unsigned char ucBuBiaoAckBag[BUBIAO_MAX_LEN] = {0};            
            
            iCount = 0x00;
            usMsgBagIndex++;
            sTatoiResultPackage->pSearchTotailMediaItemBuf = &g_sSearchTotailPictBuf[0];

            sMsgBodyArr.ucMsgArr = &ucResponseMsgBody[0];
            GetMsgBodyPackage(&sMsgBodyArr,sBBMsgHeadPack,*sTatoiResultPackage);   

            GetBBAckMsgHeadFromBBRevMsgHead(sBBMsgHeadPack,
            &sBBAckMsgHeadPack,g_ucSubBagFlag,sMsgBodyArr.iMsgLen,usMsgBagCnt,usMsgBagIndex);
            sMsgHeadArr.ucMsgArr = &ucResponseMsgHard[0];
            GetAckArrFromBBAckMsgHead(&sMsgHeadArr,sBBAckMsgHeadPack);    

            sMsgArr.ucMsgArr = &ucBuBiaoAckBag[0];
            GetAckPack(sMsgHeadArr,sMsgBodyArr,&sMsgArr);    
            BagQueueAddData(&stRetuBagQueue, sMsgArr.ucMsgArr, sMsgArr.iMsgLen);
            
            sTatoiResultPackage->ucMsgReturnFlag = 0x01;
            memset(g_sSearchTotailPictBuf,0,sizeof(sSearchTotailMediaItemBuf));
          } 
          if(iCntIndex >= sTatoiResultPackage->sDataSearchMsgBag.usMediaDataTatoil)break;
       }    
    }

    if(directory_pointer!=NULL){
      closedir(directory_pointer);
      directory_pointer = NULL;
    }

    return iCount;  
}




