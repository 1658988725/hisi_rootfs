#include <stdio.h>    
#include <string.h>    
#include <curl/curl.h>    

#include <stdlib.h>  
#include <sys/stat.h>  

#include "cdr_bubiao_analyze.h"
#include "cdr_bubiao.h"
#include "queue_bag.h"
#include "list.h"

#define STOP_DOWNLOAD_AFTER_THIS_MANY_BYTES         6000
#define MINIMAL_PROGRESS_FUNCTIONALITY_INTERVAL     3

struct myprogress {
  double lastruntime;
  CURL *curl;
  sBBMsgHeaderPack sBBMsgHeadPack;
  s_BuBiaoTatoilResult sTatoiResultPackage;
};

typedef struct {
	CURL *curlId;
	unsigned short   usMsgID;		//messge ID
	unsigned short 	 usMsgSerialNm; //Serial number
	unsigned char  	 ucChnID;   	//Channel ID
}sUploadItem;


list* g_uploadfileList = NULL;

unsigned char ucPrintFlag = 1;        //仅打印一次
unsigned char ucPercentage = 1;       //十分比



int uploaditem_eq(const void* a, const void* b)
{
	const sUploadItem *s1 = a;
	const sUploadItem *s2 = b;

	if(s1->usMsgID == s2->usMsgID &&
		s1->usMsgSerialNm == s2->usMsgSerialNm &&
		s1->ucChnID == s2->ucChnID)
		return 1;  
	
	return 0;	
}

int uploaditem_eq_Chnid(const void* a, const void* b)
{
	const sUploadItem *s1 = a;
	const sUploadItem *s2 = b;

	if(s1->usMsgID == s2->usMsgID && s1->ucChnID == s2->ucChnID)
		return 1;  
	
	return 0;	
}
int uploaditem_eq_usMsgSerialNm(const void* a, const void* b)
{
	const sUploadItem *s1 = a;
	const sUploadItem *s2 = b;
/*
	if(s1->usMsgID == s2->usMsgID &&
		s1->usMsgSerialNm == s2->usMsgSerialNm)
		return 1;  
*/	
	//usMsgID not same.
	if(s1->usMsgSerialNm == s2->usMsgSerialNm)
		return 1;  
	return 0;	
}

void uploaditem_free(void *data)
{
	sUploadItem *p = data;
	free(p);
    p = NULL;
}

 
/* this is how the CURLOPT_XFERINFOFUNCTION callback works */ 
static int xferinfo(void *p,
                    curl_off_t dltotal, curl_off_t dlnow,
                    curl_off_t ultotal, curl_off_t ulnow)
{
  struct myprogress *myp = (struct myprogress *)p;
  CURL *curl = myp->curl;
  double curtime = 0;
 
  curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &curtime);

  /* under certain circumstances it may be desirable for certain functionality
     to only run every N seconds, in order to do this the transaction time can
     be used */ 
  if((curtime - myp->lastruntime) >= MINIMAL_PROGRESS_FUNCTIONALITY_INTERVAL) {
    myp->lastruntime = curtime;
    fprintf(stderr, "TOTAL TIME: %f \r\n", curtime);
  }
  int iTotalSize = 0x00;
  int iCurrSize = 0x00;
  
  //printf("ultotal size:%d\n",ultotal);
  if(ultotal < 10*1024) return 0;
  
  iCurrSize = (int)(ulnow>>10);
  iTotalSize = (int)(ultotal>>10);
  
  if((iCurrSize!= 0x00)&&((ucPercentage!= 0x0a)) 
    &&(iCurrSize >= (ucPercentage*(iTotalSize/10) -10))&&(iCurrSize <= (ucPercentage*(iTotalSize/10) + 10)) 
    && (ucPrintFlag == 0x01)){
     ucPrintFlag = 0x00;
     //printf("UP: %d of %d \r\n",(int)iCurrSize, (int)iTotalSize);  
     printf(" %d\n",ucPercentage*10);
     myp->sTatoiResultPackage.sKakaAckPack.ucResult = ucPercentage*10;
     ucPercentage++;
     AckToPlatform(myp->sBBMsgHeadPack,myp->sTatoiResultPackage);
  }  
  ucPrintFlag = 1;
  
  if(dlnow > STOP_DOWNLOAD_AFTER_THIS_MANY_BYTES)
    return 1;
  return 0;
}
 
/* for libcurl older than 7.32.0 (CURLOPT_PROGRESSFUNCTION) */ 
static int older_progress(void *p,
                          double dltotal, double dlnow,
                          double ultotal, double ulnow)
{
  return xferinfo(p,
                  (curl_off_t)dltotal,
                  (curl_off_t)dlnow,
                  (curl_off_t)ultotal,
                  (curl_off_t)ulnow);
}

CURL *curl = NULL;

int cdr_ftp_upload(unsigned char* url,unsigned char *FullFileName,sBBMsgHeaderPack sBBMsgHeadPack,s_BuBiaoTatoilResult *sTatoiResultPackage)
{
	sUploadItem *pUploadItem = calloc(1,sizeof(sUploadItem));
	if(pUploadItem == NULL)
		return 1;

	CURLcode res;
	struct stat file_info;
	double speed_upload, total_time;
	FILE *fd;

	fd = fopen((char*)FullFileName, "rb"); /* open file to upload */ 
	if(!fd)
	{
		free(pUploadItem);
		return 1; /* can't continue */ 
	}

	struct myprogress prog;

	/* to get the file size */ 
	if(fstat(fileno(fd), &file_info) != 0)
	{
		free(pUploadItem);
		return 1; /* can't continue */ 
	}

	
	pUploadItem->usMsgID = sBBMsgHeadPack.usMsgID;
	pUploadItem->usMsgSerialNm = sBBMsgHeadPack.usMsgSerialNm;
	pUploadItem->ucChnID = sTatoiResultPackage->sMediaUploadAttrBody.ucChnID;

  	pUploadItem->curlId = curl_easy_init();	

  	if(pUploadItem->curlId)
	{
		//开始上传
		//printf("开始上传\n");   
		sTatoiResultPackage->sKakaAckPack.ucResult = 0x00;
		AckToPlatform(sBBMsgHeadPack,*sTatoiResultPackage);
		g_esBuBaioProcState = STATE_USER_IDLE; //为接收停止上传指令

		prog.lastruntime = 0;
		prog.curl = pUploadItem->curlId;
		//prog.sBBMsgHeadPack = sBBMsgHeadPack;
		//prog.sTatoiResultPackage = *sTatoiResultPackage;
		memcpy(&prog.sBBMsgHeadPack,&sBBMsgHeadPack,sizeof(sBBMsgHeaderPack));
		memcpy(&prog.sTatoiResultPackage,sTatoiResultPackage,sizeof(s_BuBiaoTatoilResult));

		curl_easy_setopt(pUploadItem->curlId, CURLOPT_URL,url);


		curl_easy_setopt(pUploadItem->curlId, CURLOPT_PROGRESSFUNCTION, older_progress);
		/* pass the struct pointer into the progress function */ 
		curl_easy_setopt(pUploadItem->curlId, CURLOPT_PROGRESSDATA, &prog);

#if LIBCURL_VERSION_NUM >= 0x072000
		curl_easy_setopt(pUploadItem->curlId, CURLOPT_XFERINFOFUNCTION, xferinfo);
		curl_easy_setopt(pUploadItem->curlId, CURLOPT_XFERINFODATA, &prog);
#endif

		curl_easy_setopt(pUploadItem->curlId, CURLOPT_NOPROGRESS, 0L);

		/* tell it to "upload" to the URL */ 
		curl_easy_setopt(pUploadItem->curlId, CURLOPT_UPLOAD, 1L);

		/* set where to read from (on Windows you need to use READFUNCTION too) */ 
		curl_easy_setopt(pUploadItem->curlId, CURLOPT_READDATA, fd);

		/* and give the size of the upload (optional) */ 
		curl_easy_setopt(pUploadItem->curlId, CURLOPT_INFILESIZE_LARGE,
		             (curl_off_t)file_info.st_size);

		printf("file_info.st_size:%d\n",(int)file_info.st_size);

		/* enable verbose for easier tracing */ 
		curl_easy_setopt(pUploadItem->curlId, CURLOPT_VERBOSE, 1L);


		if(!g_uploadfileList)
		{
			g_uploadfileList = create_list();
		}
		push_front(g_uploadfileList,(void*)pUploadItem);


		res = curl_easy_perform(pUploadItem->curlId);
		/* Check for errors */ 
		if(res != CURLE_OK)
		{
			fprintf(stderr, "curl_easy_perform() failed: %s\n",
			curl_easy_strerror(res));
			remove_data(g_uploadfileList,(void*)pUploadItem,uploaditem_eq,uploaditem_free);
			return 1;
		}
		else
		{
			/* now extract transfer info */ 
			curl_easy_getinfo(pUploadItem->curlId, CURLINFO_SPEED_UPLOAD, &speed_upload);
			curl_easy_getinfo(pUploadItem->curlId, CURLINFO_TOTAL_TIME, &total_time);

			fprintf(stderr, "Speed: %.3f bytes/sec during %.3f seconds\n",speed_upload, total_time);
		}

		/* always cleanup */ 
		curl_easy_cleanup(pUploadItem->curlId);
		remove_data(g_uploadfileList,(void*)pUploadItem,uploaditem_eq,uploaditem_free);
	}
	fclose(fd);
	fd = NULL;
	return 2;
}

//0xFB83 按照通道取消.
//已知bug.如果一个通道发起两次上传，则取消一次?还是取消两次?
//3：停止成功；4：停止失败
int cdr_stop_ftp_upload_ex_ucChnID(unsigned short usMsgID,unsigned char ucChnID)
{
	sUploadItem *pUploadItem = NULL,sSearchItem;

	int iResult = 0x03;
	sSearchItem.usMsgID = usMsgID;
	sSearchItem.ucChnID = ucChnID;

	pUploadItem = get_if(g_uploadfileList,&sSearchItem,uploaditem_eq_Chnid);
	if(pUploadItem == NULL)
		return 0x04;
  
	if(pUploadItem->curlId != NULL){  
		curl_easy_cleanup(pUploadItem->curlId);
		iResult = 0x03;
	}else{
		iResult = 0x04;
	} 
	remove_data(g_uploadfileList,(void*)pUploadItem,uploaditem_eq_Chnid,uploaditem_free);	
	return iResult;  
}

//0x9206是按照upload流水号取消任务.
//3：停止成功；4：停止失败
int cdr_stop_ftp_upload_ex_MsgSerialNm(unsigned short usMsgID,unsigned short usMsgSerialNm)
{
	sUploadItem *pUploadItem = NULL,sSearchItem;

	int iResult = 0x03;
	sSearchItem.usMsgID = usMsgID;
	sSearchItem.usMsgSerialNm = usMsgSerialNm;

	pUploadItem = get_if(g_uploadfileList,&sSearchItem,uploaditem_eq_usMsgSerialNm);
	if(pUploadItem == NULL)
		return 0x04;
  
	if(pUploadItem->curlId != NULL){  
		curl_easy_cleanup(pUploadItem->curlId);
		iResult = 0x03;
	}else{
		iResult = 0x04;
	} 
	remove_data(g_uploadfileList,(void*)pUploadItem,uploaditem_eq_usMsgSerialNm,uploaditem_free);	
	return iResult;  
}



//3：停止成功；4：停止失败
int cdr_stop_ftp_upload()
{
  int iResult = 0x03;
  if(curl != NULL){  
    curl_easy_cleanup(curl);
    iResult = 0x03;
  }else{
    iResult = 0x04;
  }  
  return iResult;  
}


int cdr_ftp_upload_ex(unsigned char* url,unsigned char *FullFileName,sBBMsgHeaderPack sBBMsgHeadPack,s_BuBiaoTatoilResult *sTatoiResultPackage)
{
  CURL *curl;
  CURLcode res;
  struct stat file_info;
  double speed_upload, total_time;
  FILE *fd;
 
  //fd = fopen("./19700126184824_179.MP4", "rb"); /* open file to upload */ 
  fd = fopen((char*)FullFileName, "rb"); /* open file to upload */ 
  //fd = fopen("../mnt/mmc/PHOTO/20170323160156.jpg", "rb"); /* open file to upload */ 
  
  if(!fd)
    return 1; /* can't continue */ 

  struct myprogress prog;
  
  /* to get the file size */ 
  if(fstat(fileno(fd), &file_info) != 0)
    return 1; /* can't continue */ 

  curl = curl_easy_init();
  if(curl) {
    //开始上传
    //printf("开始上传\n");   
    sTatoiResultPackage->sKakaAckPack.ucResult = 0x00;
    AckToPlatform(sBBMsgHeadPack,*sTatoiResultPackage);
    g_esBuBaioProcState = STATE_USER_IDLE; //为接收停止上传指令
    
    prog.lastruntime = 0;
    prog.curl = curl;
    memcpy(&prog.sBBMsgHeadPack,&sBBMsgHeadPack,sizeof(sBBMsgHeaderPack));
    memcpy(&prog.sTatoiResultPackage,sTatoiResultPackage,sizeof(s_BuBiaoTatoilResult));
    
    /* upload to this place */ 
    /*
    curl_easy_setopt(curl, CURLOPT_URL,
                     "ftp://xjl:xjl@192.168.40.252/nfs/demo_project/19700126184824_180.MP4");
                     */
#if(1)
    curl_easy_setopt(curl, CURLOPT_URL,
                     url);
#else
    //curl_easy_setopt(curl, CURLOPT_URL,"ftp://xjl:xjl@192.168.100.101/nfs/demo_project/19700126184824_180.MP4");//ok
    curl_easy_setopt(curl, CURLOPT_URL,
                     "ftp://admin:123456@192.168.40.16/I62/2017/0321/test.MP4");//ok
#endif

    curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, older_progress);
    /* pass the struct pointer into the progress function */ 
    curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, &prog);
 
#if LIBCURL_VERSION_NUM >= 0x072000
    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, xferinfo);
    curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &prog);
#endif

    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
 
    /* tell it to "upload" to the URL */ 
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
 
    /* set where to read from (on Windows you need to use READFUNCTION too) */ 
    curl_easy_setopt(curl, CURLOPT_READDATA, fd);
 
    /* and give the size of the upload (optional) */ 
    curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE,
                     (curl_off_t)file_info.st_size);

    printf("file_info.st_size:%d\n",(int)file_info.st_size);
 
    /* enable verbose for easier tracing */ 
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
 
    res = curl_easy_perform(curl);
    /* Check for errors */ 
    if(res != CURLE_OK) {
      fprintf(stderr, "curl_easy_perform() failed: %s\n",
              curl_easy_strerror(res));
      return 1;
    }
    else {
      /* now extract transfer info */ 
      curl_easy_getinfo(curl, CURLINFO_SPEED_UPLOAD, &speed_upload);
      curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &total_time);
 
      fprintf(stderr, "Speed: %.3f bytes/sec during %.3f seconds\n",speed_upload, total_time);
     }
    /* always cleanup */ 
    curl_easy_cleanup(curl);
  }
  fclose(fd);
  fd = NULL;
  return 2;
}



