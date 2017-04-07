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


//当前文件所摄的时间是否在指定的时间范围内
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

//最后修改时间
int GetFileCreateTime(char *sFullFileName,unsigned char *pFileCreateTime)
{
   struct stat buf;
   int result = stat(sFullFileName, &buf);    //获得文件状态信息
   
   if( result != 0 ){
     perror("显示文件状态信息出错");
   }else{        
     struct tm *p;
     p = localtime(&buf.st_ctime); 
     //printf("%d/%d/%d %d:%d:%d\n", 1900 + p->tm_year, 1 + p->tm_mon, p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec);
     //printf("文件创建时间:%s\n",ctime(&buf.st_ctime));        
     sprintf((char*)pFileCreateTime,"%04d%02d%02d%02d%02d%02d",1900 + p->tm_year, 1 + p->tm_mon, p->tm_mday,
     p->tm_hour, p->tm_min, p->tm_sec);
   }   
   return 0;
}

/*
获取满足条件的媒体数目
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
          if(strstr(entry->d_name,"_pre")!=NULL){//_pre.jpg去除 大图的预览图
           continue;      
          }  
          if(strstr(entry->d_name,"_")!=NULL){//碰撞图片?
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
          if(strstr(entry->d_name,"_pre")!=NULL){//_pre.jpg去除 大图的预览图
           continue;      
          }  
          if(strstr(entry->d_name,"_")!=NULL){//碰撞图片? xxxx_101.jpg
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
          if(strstr(entry->d_name,"_pre")!=NULL){//_pre.jpg去除 大图的预览图
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
统计文件的数目
总长度 :1024  
12+4+2 header 18
消息体:2+N*35

一个包最多28 张图像信息返回
表86 存储多媒体数据检索应答消息体数据格式
起始字节	字段	数据类型	描述及要求
0	应答流水号	WORD 	对应的多媒体数据检索消息的流水号
2	多媒体数据总项数	WORD 	满足检索条件的多媒体数据总项数
4	检索项		多媒体检索项数据格式见表87

表87 多媒体检索项数据格式
起始字节	字段	数据类型	描述及要求
0	多媒体ID	DWORD	>0
4	多媒体类型	BYTE	0：图像；1：音频；2：视频
5	 通道ID	BYTE	
6	事件项编码	BYTE	0：平台下发指令；1：定时动作；2：抢劫报警触发；3：碰撞侧翻报警触发；其他保留
7	位置信息汇报
(0x0200)消息体	BYTE[28]	表示拍摄或录制的起始时刻的位置基本信息数据

检索项由多个  多媒体检索项数据 组成；
每个返回包最多有(28)PER_BAG_MAX_ITEMS_CNT 个  多媒体检索项数据

查询指定文件夹中图片的详细信息存放于 g_sSearchTotailPictBuf[5]中
当存满了5个就返回到移动端


若当查询的是图片的信息
*/
int SearchMediaProcessCore(const char *pDir,s_BuBiaoTatoilResult *sTatoiResultPackage,
    sBBMsgHeaderPack sBBMsgHeadPack)
{
    DIR *directory_pointer;
    struct dirent *entry;
    int iCount = 0;                   //计数器，图片数量是否满一个返回包
    int iCntIndex = 0;                //计数器 为记录是否查找完所有的图片
    unsigned short usMsgBagIndex = 0; //当前为第几次返回
    unsigned short usMsgBagCnt = 0;   //返回到移动端的总次数

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


                g_sSearchSourceBuf[iCount].ucLogicChnID = 0x02;//定值
                sscanf(entry->d_name,"%[^_]",g_sSearchSourceBuf[iCount].ucFileStartTime);
                printf("ucFileStartTime:%s\n",g_sSearchSourceBuf[iCount].ucFileStartTime);
                strcpy((char*)g_sSearchSourceBuf[iCount].ucFileEndTime,(char*)g_sSearchTotailPictBuf[iCount].ucFileTime);          
                memset((char*)g_sSearchSourceBuf[iCount].ucWarningMark,0,sizeof(g_sSearchSourceBuf[iCount].ucWarningMark));
                g_sSearchSourceBuf[iCount].ucSourceType = sTatoiResultPackage->sSearchSourceListBody.ucSourceType;
                g_sSearchSourceBuf[iCount].ucStreamType = sTatoiResultPackage->sSearchSourceListBody.ucStreamType ;
                g_sSearchSourceBuf[iCount].ucStoreType = sTatoiResultPackage->sSearchSourceListBody.ucStoreType;
                g_sSearchSourceBuf[iCount].uiFileSize = g_sSearchTotailPictBuf[iCount].uiFileSize;
      
                //test_read_exif(sFullFileName,iCount);//没有写入,全为0
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
            test_read_exif(sFullFileName,iCount);//获取位置即经纬信息 通过文件名/mnt/mmc/xxx.jpg exif 
            
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




