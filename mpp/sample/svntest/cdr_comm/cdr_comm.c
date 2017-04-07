/************************************************************************	
** Filename: 	cdr_comm.c
** Description:  
** Author: 	xjl
** Create Date: 
** Version: 	v1.0

	Copyright(C) 2016 e-eye CO.LTD. ShenZhen <www.e-eye.cn>

*************************************************************************/
#include<sys/types.h>
#include<sys/stat.h>
#include<errno.h>
#include<fcntl.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include <signal.h>
#include <unistd.h>

#include <sys/vfs.h>
#include <dirent.h>
#include <time.h>
#include "cdr_comm.h"
#include "cdr_XmlLog.h"

//#define SEND_FIFO "/tmp/myfifo"    //send fifo
//#define REVC_FIFO "/tmp/myfifo1"   //revc fifo

//static unsigned char g_ucExitFlag = 0;

char g_pCdrStartDateTimeBuf[120] = {0};


int recspslen = 0;
int recppslen = 0;

unsigned char recspsdata[64];
unsigned char recppsdata[64];

VENC_GETSTREAM_CH_PARA_S gsrec_stPara;
VENC_GETSTREAM_CH_PARA_S gslive_stPara;



/*
��ch�ַ�����posλ�ÿ�ʼ��ȡ
ָ������length���ַ�����
subch
*/
char* substring(char* ch,int pos,int length,char* subch)  
{  
    char* pch=ch;  
    int i;  
    pch=pch+pos;  
    for(i=0;i<length;i++)  
    {  
        subch[i]=*(pch++);       
    }  
    subch[length]='\0'; 
    return subch;       
}  

/*
��ָ�����ȵ��ַ�����ʮ��������ʽ��ӡ����
*/
void PrintfString2Hex(unsigned char *pSrc,int iLen)
{
    int i = 0;
    printf("iLen %d pSrc: \n",iLen);
    for(i=0;i<iLen;i++){
      printf("%02x",*pSrc++);
    }
    printf("\n"); 
}


/*
���ܽ�20170308153211��ΪBCD[6]
�������:sTimeStr
�������:sBCDBuf
*/
void TimeStr2BCD(const char *sTimeStr,char *sBCDBuf)
{
    if((sTimeStr == NULL) || (sBCDBuf == NULL))return;
    //const char *sTimeStrBuf = "20170308155510";
    int year1,year,mon,day,hour,min,sec;
    //sscanf(sTimeStr,"%2d%2d%2d%2d%2d%2d%2d",&year1,&year,&mon,&day,&hour,&min,&sec);    
    sscanf(sTimeStr,"%2x%2x%2x%2x%2x%2x%2x",&year1,&year,&mon,&day,&hour,&min,&sec);    
    sBCDBuf[0] = year;
    sBCDBuf[1] = mon;
    sBCDBuf[2] = day;
    sBCDBuf[3] = hour;        
    sBCDBuf[4] = min;    
    sBCDBuf[5] = sec;    
    //PrintfString2Hex((unsigned char *)sBCDBuf,6);
}


/*
��ָ�����ȵ��ַ�����ʮ��������ʽ��ӡ����
*/
void PrintfString2Hex2(char *pSrc,int len)
{
    int i = 0;    
    printf("Len:%d\n",len);
    for(i=0;i<len;i++){
      printf("%02x",pSrc[i]);
    }
    printf("\n"); 
}

void cdr_log_get_time(char * ucTimeTemp)
{   
    if(ucTimeTemp == NULL)
    {
      printf("[%s %d] ucTimeTemp is null\n",__FUNCTION__,__LINE__);
      return;
    }
    time_t curTime;
	struct tm *ptm = NULL;
	time(&curTime);
	ptm = gmtime(&curTime);
    
    sprintf(ucTimeTemp, "%04d%02d%02d%02d%02d%02d",
      ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday,ptm->tm_hour,ptm->tm_min,ptm->tm_sec);
}

void cdr_get_curr_time(char * ucTimeTemp)
{   
    #if(0)
    if(ucTimeTemp == NULL)
    {
      printf("[%s %d] ucTimeTemp is null\n",__FUNCTION__,__LINE__);
      return;
    }
    time_t curTime;
	struct tm *ptm = NULL;
	time(&curTime);
	ptm = gmtime(&curTime);
    
    sprintf(ucTimeTemp, "%04d%02d%02d%02d%02d%02d",
      ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday,ptm->tm_hour,ptm->tm_min,ptm->tm_sec);
    #else
    cdr_log_get_time(ucTimeTemp);
    #endif
}


int set_cdr_start_time()
{
    char pDateTimeBuf[120] = {0};
    
    time_t curTime;
	struct tm *ptm = NULL;
	time(&curTime);
	ptm = gmtime(&curTime);
    
    sprintf(pDateTimeBuf, "%04d%02d%02d%02d%02d%02d",
      ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday,ptm->tm_hour,ptm->tm_min,ptm->tm_sec);

    memcpy(g_pCdrStartDateTimeBuf,pDateTimeBuf,strlen(pDateTimeBuf)+1);

    set_cdr_start_sec();
        
    return 0;
}

#if(0)
static int cdr_system_self(const char * cmd) 
{ 
    FILE * fp; 
    int res = 0; 
    char buf[1024] = {0}; 
    
    if (cmd == NULL) 
    { 
        printf("my_system cmd is NULL!\n");
        return -1;
    } 
    if ((fp = popen(cmd, "r") ) == NULL) 
    { 
        perror("popen");
        printf("popen error: %s/n", strerror(errno)); return -1; 
    } 
    else
    {
         while(fgets(buf, sizeof(buf), fp)) 
        { 
            printf("%s", buf); 
        } 
        if ( (res = pclose(fp)) == -1) 
        { 
            printf("close popen file pointer fp error!\n"); return res;
        } 
        else if (res == 0) 
        {
            return res;
        } 
        else 
        { 
            printf("popen res is :%d\n", res); return res; 
        } 
    }
 }
#endif

#if 0
static void timer(int sig)  
{  
    if(SIGALRM == sig)  
    {  
       g_ucExitFlag = 1;   
    }  
    return ;  
} 


//read system process execuce 's result
static int cdr_revc_systm_result()
{
    int fd = 0;
    int iReadCount = 0;
    int iResult = 0;
    //unsigned char ucMsg[20] = {0};
    char ucReadBufArr[200] = {0};

    unlink(REVC_FIFO);
    if(access(REVC_FIFO, F_OK) == -1){
       if((mkfifo(REVC_FIFO,O_CREAT|O_EXCL)<0)&&(errno!=EEXIST)){
            printf("[%s %d]cannot create fifoserver\n",__FUNCTION__,__LINE__);
            return -1;
       }
    }
    
    fd = open(REVC_FIFO,O_RDONLY|O_NONBLOCK,0);
    if(fd == -1)
    {
      perror("open");
      exit(1);
    }
    //������ʱ������1���ӻ�û���յ������������Զ�ʧ�ܲ���������
    signal(SIGALRM, timer); 
    g_ucExitFlag = 0;    
    alarm(60*1);     

    while(1)
    {
       memset(ucReadBufArr,0,sizeof(ucReadBufArr));
       iReadCount = read(fd,ucReadBufArr,100);
       if(iReadCount ==-1){
         printf("[%s %d]read from system process\n",__FUNCTION__,__LINE__);
         if(errno==EAGAIN) printf("no data yet\n");
       }
       if(iReadCount>0){
          //printf("read %s from  iReadCount %02x \n",ucReadBufArr,iReadCount);          
          iResult = atoi(ucReadBufArr);
          //printf("iResult:%d\n",iResult);
          break;
       }
       if(g_ucExitFlag)break;       
       sleep(1);
    }
    return iResult;
}
#endif

#if(0)
static int cdr_send_msg_to_system(char *cmd)
{
    int fd;
    char w_buf[200] = {0};
    int nwrite;
    int iResult = 0;
    
    if(access(SEND_FIFO, F_OK) == -1){
       printf("��ǰ��fifo������\n");
       return -1;
    }           
    fd=open(SEND_FIFO,O_WRONLY|O_NONBLOCK,0);
    if(fd==-1){
       if(errno==ENXIO){
        printf("open error;no reading process\n");
        return -1;
       }
    }

    memcpy(w_buf,cmd,strlen(cmd)+1);    
    if((nwrite=write(fd,w_buf,strlen(w_buf)+1))==-1)
    {
      if(errno==EAGAIN){
        printf("The FIFO has not been read yet. Please try later\n");
        return -1;
      }
    }else{ 
      //printf("%s\n",w_buf);//����buf���
      iResult = cdr_revc_systm_result(); //������һ������ ���ص�ִ�н��
      printf("[%s %d] get system process exece result %d\n",__FUNCTION__,__LINE__,iResult);
    }    
    return iResult;
}
#endif


#if(0)
int cdr_system_new(const char * cmd) 
{ 
    int iResult = 0;
    //FILE * fp; 
    //int res = 0; 
    //char buf[1024] = {0}; 
    
    iResult = cdr_system_self(cmd);
    if(iResult == 0)
    {
      return 0;
    }
    
    if (cmd == NULL) 
    { 
        printf("my_system cmd is NULL!\n");
        return -1;
    }     
    iResult = cdr_send_msg_to_system(cmd);

    if(iResult == -1)    
    {
      iResult = cdr_system_self(cmd);
    }    
    return iResult;
 }
#endif

#if(0)
void main(void)
{
  const unsigned char *pCmd="ls -l";  
  //const unsigned char *pCmd="ls -l";    
  cdr_system(pCmd);

  while(1);
}
#endif


void PrintTime()
{
    char *wday[]={"����","��һ","�ܶ�","����","����","����","����"};
    time_t timep;
    struct tm *p;
    time(&timep);
    p=gmtime(&timep);
    printf("%04d-%02d-%02d ",(1900+p->tm_year),(1+p->tm_mon),p->tm_mday);
    printf("%02d:%02d:%02d %s ", p->tm_hour,p->tm_min, p->tm_sec,wday[p->tm_wday]);
}

//��ʱ���ַ���תΪ��.      201601121750
time_t _strtotime(char *pstr)
{
	if(pstr == NULL)
		return 0;
	
	//printf("%s\r\n",pstr);
	int y,m,d,h,M,s;
	sscanf(pstr,"%04d%02d%02d%02d%02d%02d",&y,&m,&d,&h,&M,&s);
	
	struct tm t;
	time_t t_of_day;//long int t_of_day
	t.tm_year=y-1900;
	t.tm_mon=m-1;
	t.tm_mday=d;
	t.tm_hour=h;
	t.tm_min=M;
	t.tm_sec=s;
	t.tm_isdst=0;
	t_of_day=mktime(&t);
	//printf("%s:%ld\r\n",pstr,t_of_day);
	return t_of_day;		
}

/*
����г��쳣������.gp�ļ�
��ȡ/mmt/mmc/GRSGrailĿ¼�µ��ļ� ��Ϊ.gp��β����ѹ��
*/
void TarGPSGrailLog()
{
  DIR *directory_pointer;
  struct dirent *entry;  
  struct FileList
  {
    char filename[64];
    struct FileList *next;
  }start,*node;
  
  char cCmdBuf[400] = {0};
 
  if((directory_pointer=opendir("/mnt/mmc/GPSTrail"))==NULL){
    printf("[%s %d] Error opening\n",__FUNCTION__,__LINE__);
  }else{
    start.next=NULL;
    node=&start;
    while ((entry=readdir(directory_pointer))!=NULL)
    {
      node->next=(struct FileList *)malloc(sizeof(struct FileList));
      node=node->next;
      strcpy(node->filename,entry->d_name);
      node->next=NULL;
    }
    closedir(directory_pointer);
    node=start.next;
    while(node)
    {
      if(strcmp(node->filename,".")==0 || strcmp(node->filename,"..")==0){  //current dir OR parrent dir
       node=node->next;
       continue;
      }   
      if(strstr(node->filename,".gp")!=NULL){        
         printf("GPS :%s\n",node->filename);
         char ucBufTemp[64] = {0};
         sscanf(node->filename, "%[^.]", ucBufTemp);
         printf("%s\n", ucBufTemp);

         char sFullFileName[100] = {0};
         sprintf(sFullFileName,"/mnt/mmc/GPSTrail/%s",node->filename);

         sprintf(cCmdBuf,"zip -qj /mnt/mmc/GPSTrail/%s_100.zip %s",ucBufTemp,sFullFileName);
         cdr_system(cCmdBuf);
         memset(cCmdBuf,0,sizeof(cCmdBuf));
         cdr_system("sync");   
      }
      //printf("%s\n",node->filename);
      node=node->next;    
    }
    cdr_system("rm -f /mnt/mmc/GPSTrail/*.gp");
    cdr_system("sync");
  }
}


int kfifo_put(kfifo *fifo, void * element )
{
    if ( QUEUE_SIZE - fifo->in + fifo->out == 0 )
        return 0;

    __sync_synchronize();      //ȷ��ȡ����out�����µģ�������ô˵�ģ������Ȼ��ɲ���Ҫ��

    unsigned int index = fifo->in & (QUEUE_SIZE - 1);
    fifo->data[ index ] = element;

    __sync_synchronize();      //ȷ����д�������ٸ���in

    fifo->in++;

    return 1;
}

int kfifo_get(kfifo *fifo, void **element )
{
    if ( fifo->in - fifo->out == 0 )
        return 0;
    unsigned int index = fifo->out & (QUEUE_SIZE - 1);

    __sync_synchronize();       //ȷ��������in�����µģ�ͬ�ϣ�

    *element = fifo->data[ index ];

    __sync_synchronize();       //ȷ���ȶ�ȡ�����ٸ���out

    fifo->out++;

    return 1;
}


