/************************************************************************	
** Filename: 	
** Description:  
** Author: 	xjl
** Create Date: 
** Version: 	v1.0

	Copyright(C) 2016 e-eye CO.LTD. ShenZhen <www.e-eye.cn>
*************************************************************************/
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "sample_comm.h"
#include "cdr_uart.h"
#include "cdr_queue.h"
#include "queue_bag.h"
#include "cdr_bubiao_analyze.h"

static void init_uart_handle_thread(void) ;

#define UART_DEVICE "/dev/ttyAMA2"

#define FALSE  -1
#define TRUE    0

int g_uart_thread_flag = 0;

int speed_arr[] = 
{
 B115200, B38400, B19200, B9600, B4800, B2400, B1200, B300,
 B115200, B38400, B19200, B9600, B4800, B2400, B1200, B300, 
};
int name_arr[] = 
{
 115200, 38400, 19200, 9600, 4800, 2400, 1200, 300, 
 115200, 38400, 19200, 9600, 4800, 2400, 1200, 300, 
};

unsigned char g_ucGpsDataBuff[300] = {0};
static cdr_uart_callback g_uart_fun_callbak;

int g_uart2fd = -1;

static int iBuBiaoPackLen = 0;//���沿����ĳ���
static volatile unsigned char ucStartBuBiao = 0x00;

#define  UART_DEBUG  0


/**
*@brief ���ô���ͨ������
*@param fd ���� int �򿪴��ڵ��ļ����
*@param speed ���� int �����ٶ�
*@return void
*/
static void set_speed(int fd, int speed){
  int i; 
  int status; 
  struct termios Opt;
  tcgetattr(fd, &Opt);   
  for ( i= 0; i < sizeof(speed_arr) / sizeof(int); i++)
  { 
    if (speed == name_arr[i]) { 
      tcflush(fd, TCIOFLUSH); 
      cfsetispeed(&Opt, speed_arr[i]); 
      cfsetospeed(&Opt, speed_arr[i]); 
      status = tcsetattr(fd, TCSANOW, &Opt); 
      if (status != 0) { 
        perror("tcsetattr fd1\n"); 
        return; 
      } 
      tcflush(fd,TCIOFLUSH); 
    } 
  }
}


/**
*@brief ���ô�������λ��ֹͣλ��Ч��λ
*@param fd ���� int �򿪵Ĵ����ļ����
*@param databits ���� int ����λ ȡֵ Ϊ 7 ����8
*@param stopbits ���� int ֹͣλ ȡֵΪ 1 ����2
*@param parity ���� int Ч������ ȡֵΪN,E,O,,S
*/
static int set_Parity(int fd,int databits,int stopbits,int parity)  
{   
    struct termios options;   
    if  ( tcgetattr( fd,&options)  !=  0) {   
        perror("SetupSerial 1");       
        return(FALSE);    
    }  
    options.c_cflag &= ~CSIZE;   

    options.c_cflag |= (CLOCAL | CREAD); //һ������õı�־
    
    switch (databits) /*��������λ��*/  
    {     
    case 7:       
        options.c_cflag |= CS7;   
        break;  
    case 8:       
        options.c_cflag |= CS8;  
        break;     
    default:      
        fprintf(stderr,"Unsupported data size\n"); return (FALSE);    
    }  
    switch (parity)   
    {     
        case 'n':  
        case 'N':      
            options.c_cflag &= ~PARENB;    /* Clear parity enable */  
            options.c_iflag &= ~INPCK;     /* Enable parity checking */   
            break;    
        case 'o':     
        case 'O':       
            options.c_cflag |= (PARODD | PARENB); /* ����Ϊ��Ч��*/    
            options.c_iflag |= INPCK;             /* Disnable parity checking */   
            break;    
        case 'e':    
        case 'E':     
            options.c_cflag |= PARENB;      /* Enable parity */      
            options.c_cflag &= ~PARODD;     /* ת��ΪżЧ��*/       
            options.c_iflag |= INPCK;       /* Disnable parity checking */  
            break;  
        case 'S':   
        case 's':  /*as no parity*/     
            options.c_cflag &= ~PARENB;  
            options.c_cflag &= ~CSTOPB;break;    
        default:     
            fprintf(stderr,"Unsupported parity\n");      
            return (FALSE);    
        }    
    /* ����ֹͣλ*/    
    switch (stopbits)  
    {     
        case 1:      
            options.c_cflag &= ~CSTOPB;    
            break;    
        case 2:      
            options.c_cflag |= CSTOPB;    
           break;  
        default:      
             fprintf(stderr,"Unsupported stop bits\n");    
             return (FALSE);   
    }   
    /* Set input parity option */   
    if (parity != 'n')     
        options.c_iflag |= INPCK;   

    options.c_cflag |= (CLOCAL | CREAD);

    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

    options.c_oflag &= ~OPOST;
    options.c_oflag &= ~(ONLCR | OCRNL); 

    options.c_iflag &= ~(ICRNL | INLCR);
    options.c_iflag &= ~(IXON | IXOFF | IXANY); 

    tcflush(fd,TCIFLUSH); 
    
    options.c_cc[VTIME] = 150; /* ���ó�ʱ15 seconds*/     
    options.c_cc[VMIN] = 0;    /* Update the options and do it NOW */  
    if (tcsetattr(fd,TCSANOW,&options) != 0)     
    {   
        perror("SetupSerial 3");     
        return (FALSE);    
    }    
    return (TRUE); 
}


int cdr_uart_init_factory(void)
{    
    return g_uart2fd;
}


int GetBuBiaoPackLen(void)
{
  return iBuBiaoPackLen;
}


/*
0�����Ͷ��е�ʵ��
1, ���read �����ݵ����У�
2���Ӷ�����ȡһ��gprm��  �ӻ��Ͷ�����ȡһ��Bag 2.1��һ�̶��ַ�����ȡָ��gprm����ͷ��β��У��
*/
int uart_get_dst_bag(char *pSrcBuf,char *pDstBag)
{
    int i = 0;
    int m = 0; 
    int n = 0;
    int res = 0;     
    static int j = 0;
    static unsigned  char pDstBagTemp[1200] = {0};
    static unsigned  char cGPSBagStartFlag = 0;//GPS Э�����ʼ��־
    static unsigned  char cGPSBagStopFlag = 0;
    unsigned char ucTempBuff = 0x00;//Ϊ����������ݵ��õĻ���
   
    if(pSrcBuf==NULL || pDstBag == NULL){
        printf("Invalid parameter!\n");
        return -1;
    }

#if UART_DEBUG
    printf("stUartQueue.usBufSize:%d\n",stUartQueue.usBufSize);
    printf("\r\n");    
    PrintfString2Hex2(pSrcBuf,stUartQueue.usBufSize);
#endif    

    /*ѭ�������������� RMC*/
    for(i=stUartQueue.usHead;;i++,n++){
        
     if(n > stUartQueue.usBufSize){//������Ͷ�����û�д���Ŀ������       
        printf("������Ͷ�����û�д���Ŀ������ \n");
        tcflush(g_uart2fd,TCIOFLUSH);//clear uart buff
        return -1;   
     }
     
     if(QueueCheckEmpty(&stUartQueue) == 0x01)  return -1;     
     if(i == stUartQueue.usBufSize) i = 0;

     /*gps test Э�� start--------------------------------------------------------------*/    
     if(pSrcBuf[i] == '$'){
        i++;
        if(i == stUartQueue.usBufSize) i = 0;
        if(pSrcBuf[i] == 'G'){//GNRMC  GPRMC
            i++;
            if(i == stUartQueue.usBufSize) i = 0;
            if((pSrcBuf[i] == 'N')||(pSrcBuf[i] == 'P')){
               i++;
               if(i == stUartQueue.usBufSize) i = 0;
               if(pSrcBuf[i] == 'R'){
                  i++;
                  if(i == stUartQueue.usBufSize) i = 0;
                  if(pSrcBuf[i] == 'M'){ 
                     i++;
                     if(i == stUartQueue.usBufSize) i = 0;
                     if(pSrcBuf[i] == 'C'){  
                       //printf("get the head,%d\n",i);                                     
                       if(i>=5){
                          stUartQueue.usHead = i - 5;//ͷָ���ƶ�
                       }else{
                          stUartQueue.usHead = (i+stUartQueue.usBufSize) - 5;//ͷָ���ƶ�
                       }                       
                       j = 0;
                       for(m=0;m<6;m++){
                         res = QueueGetData(&stUartQueue,&pDstBagTemp[j]);
                         //printf("%c",pDstBagTemp[j]);//sleep(2);                         
                         if(res == 0x01){
                            j++;
                         }
                       }                        
                       cGPSBagStartFlag = 0x01;
                       continue;
                     }
                   }
                 }
             }
        }//end GPRMC GNRMC
        else if(pSrcBuf[i] == 'B'){//start BDRMC
            i++;
            if(i == stUartQueue.usBufSize) i = 0;
            if(pSrcBuf[i] == 'D'){
               i++;
               if(i == stUartQueue.usBufSize) i = 0;
               if(pSrcBuf[i] == 'R'){
                  i++;
                  if(i == stUartQueue.usBufSize) i = 0;
                  if(pSrcBuf[i] == 'M'){ 
                     i++;
                     if(i == stUartQueue.usBufSize) i = 0;
                     if(pSrcBuf[i] == 'C'){  
                       //printf("get the head\n");
                       if(i>=5)stUartQueue.usHead = i - 5;//ͷָ���ƶ�
                       else stUartQueue.usHead = (i+stUartQueue.usBufSize) - 5;//ͷָ���ƶ�
                       
                       j = 0;
                       for(m=0;m<6;m++){
                         res = QueueGetData(&stUartQueue,&pDstBagTemp[j]);
                         printf("%c",pDstBagTemp[j]);//sleep(2);
                         if(res == 0x01){
                            j++;
                         }
                       }     
                       cGPSBagStartFlag = 0x01;
                       continue;
                     }
                   }
                 }
             }         
         }//end BDRMC
         else if(pSrcBuf[i] == 'T'){//TEST,START $TEST,START
          i++;
          if(i == stUartQueue.usBufSize) i = 0;
          if(pSrcBuf[i] == 'E'){ 
            i++;
            if(i == stUartQueue.usBufSize) i = 0;  
            if(pSrcBuf[i] == 'S'){
               i++;
               if(i == stUartQueue.usBufSize) i = 0;                 
               if(pSrcBuf[i] == 'T'){
                   i++;
                   if(i == stUartQueue.usBufSize) i = 0;                 
                   if(pSrcBuf[i] == ','){   
                       i++;
                       if(i == stUartQueue.usBufSize) i = 0;                 
                       if(pSrcBuf[i] == 'S'){   
                           i++;
                           if(i == stUartQueue.usBufSize) i = 0;                 
                           if(pSrcBuf[i] == 'T'){   
                               i++;
                               if(i == stUartQueue.usBufSize) i = 0;                 
                               if(pSrcBuf[i] == 'A'){   
                                   i++;
                                   if(i == stUartQueue.usBufSize) i = 0;                 
                                   if(pSrcBuf[i] == 'R'){   
                                       i++;
                                       if(i == stUartQueue.usBufSize) i = 0;                 
                                       if(pSrcBuf[i] == 'T'){  
                                          //printf("revc test start.\n");
                                          return 1;//test start           
                                       }                            
                                   }                            
                               }                            
                           }                        
                       }//end start                    
                   }
               }
            }
        }
     }//end TEST    
   }//end $ 0x24

   /*gps test Э�� end--------------------------------------------------------------*/
 
    /*GPS Э��-------------------------------------------------------------------------*/
    /*GPRMC GNRMC DBRMC body*/
    else if((cGPSBagStartFlag == 0x01)&&(pSrcBuf[i] != 0x0D)){       //get the body
       res = QueueGetData(&stUartQueue,&pDstBagTemp[j]);       
       //printf("%c",pDstBagTemp[j]);
       if(res == 0x01){
         j++;
       }
     }
     /*GPRMC GNRMC DBRMC end*/
     //if((pSrcBuf[i] == '\r')&&(cGPSBagStartFlag == 0x01))
     else if(((pSrcBuf[i] == 0x0D)||(pSrcBuf[i] == 0x0A))&&(cGPSBagStartFlag == 0x01))    
     {    
        //printf("\nGet the end %02x\n",pSrcBuf[i]);
        QueueGetData(&stUartQueue,&pDstBagTemp[j]);        
        i++;
        j++;
        if(i == stUartQueue.usBufSize) i = 0;
                
        if((pSrcBuf[i] == '\r')||(pSrcBuf[i] == '\n'))//406 send �������� \r\r\n ԭʼ��׼\r\n
        {
           QueueGetData(&stUartQueue,&pDstBagTemp[j]);
           j++;
           pDstBagTemp[j] = '\0';
           cGPSBagStopFlag = 0x01;

           cGPSBagStartFlag = 0x00;//xjl 2017-02-09
           //printf("end of gps\n");
           break;
        }       
     }

   /*start ����Э��-----------------------------------------------------------------*/
   else if((cGPSBagStartFlag!=0x01)&&(pSrcBuf[i] == 0x7e)&&(ucStartBuBiao == 0x00)){//start   ~   

        stUartQueue.usHead = i;
        res = QueueGetData(&stUartQueue,&pDstBagTemp[0]);//��pDstBagTemp��Ų�������
        ucStartBuBiao = 0x01;     
        //printf("pDstBagTemp:%02x;i=%d\n",pDstBagTemp[0],i);
        j = 0;
        j++;
        //printf("get the bb hard\n");
        continue;
    }
          
   else if((cGPSBagStartFlag!=0x01)&&(pSrcBuf[i] != 0x7e)&&(ucStartBuBiao == 0x01)){
        //get the body
        res = QueueGetData(&stUartQueue,&pDstBagTemp[j]);       
        if(res == 0x01){
         j++;         
        }
        continue;
    }
    else if((cGPSBagStartFlag!=0x01)&&(pSrcBuf[i] == 0x7e)&&(ucStartBuBiao == 0x01)){//bubiao end
        res = QueueGetData(&stUartQueue,&pDstBagTemp[j]);
        if(res == 0x01){
          //printf("get the bb end j:%d  cStartFlag:%d %02x\n",j,cGPSBagStartFlag,pDstBagTemp[j]);
          j++;          
        }
        #if(0)
        res = QueueGetData(&stUartQueue,&pDstBagTemp[j]);//2017 1-4  �޸�???
        printf("j:%d ,%02x\n",j,pDstBagTemp[j]);
        //pDstBagTemp[j] = '\0';//2017 1-4  �޸�
        ucStartBuBiao = 0x00;
        iBuBiaoPackLen = j+1;
        #else              
        pDstBagTemp[j] = '\0';
        ucStartBuBiao = 0x00;
        iBuBiaoPackLen = j;
        #endif

        if(iBuBiaoPackLen == 0x02){//�����������
           //1.������ǰ��
           //2.��ջ��Ͷ������½���
           printf("�����������\n");
           UartRecvQueueInit(&stUartQueue);           
           tcflush(g_uart2fd,TCIOFLUSH);//clear uart buff
           memset(pDstBag,0,sizeof(char)*BUBIAO_MAX_LEN);
           
           ucStartBuBiao = 0x00;
           iBuBiaoPackLen = 0;
           cGPSBagStartFlag = 0;
           return -1;
        }        
        memcpy(pDstBag,pDstBagTemp,iBuBiaoPackLen);//��ȡһ��ָ���İ�
        memset(pDstBagTemp,0,sizeof(pDstBagTemp));        
        return 2;//���ز���Э���
    }
   /*end of ����Э��-----------------------------------------------------------------*/ 
    else{
          QueueGetData(&stUartQueue,&ucTempBuff);//ȡ��������������ݶ��� ��ջ��Ͷ���
     }     
     /*GPS Э��end-------------------------------------------------------------------------*/    
   }//end of for
    
    /*GPS DBS��ȡ���*/
    if(cGPSBagStopFlag == 0x01){
      cGPSBagStartFlag = 0x00;
      cGPSBagStopFlag = 0x00;
      j = 0x00;
      memcpy(pDstBag,pDstBagTemp,strlen((char*)pDstBagTemp));//��ȡһ��ָ���İ�
      memset(pDstBagTemp,0,sizeof(pDstBagTemp));
      ucStartBuBiao = 0x00;
      return 0;
    }else{
      printf("data is invalid\n");
      return -1;
    }  
}

void RecvBagPoll(void)
{
   if(BagQueueCheckEmpty(&stRecvBagQueue))	return;
   BuBiaoProtoRecvBagHandle();
}

void RetuBagPoll(void)
{
  if(BagQueueCheckEmpty(&stRetuBagQueue))	return;
  BuBiaoProtoRetuBagHandle();
}


static void cdr_proc_uart_data_thread(void * pArgs)
{  
  while(1)
  {
    usleep(500000);  
    RecvBagPoll();    
  }
}

static void cdr_uart_data_ack_thread()
{
  while(1)
  {
    usleep(500000);     
    RetuBagPoll();//�ɵ�����һ�̣߳�ʹ�÷����������ͬ�����У��ﵽ������Ч��
  } 

}


static void cdr_get_uart_data_thread(void * pArgs)
{    
    int res = 0;
    int i = 0;
    char buf[1024] = {0};

    UartRecvQueueInit(&stUartQueue);    
    cdr_uart_callback uartfcallbak = g_uart_fun_callbak;
    fd_set rd;
    struct timeval timeout;   
#if(1)//ʵʱ���ݽ���  
	while(1)
	{
        //UartRecvQueueInit(&stUartQueue);
        timeout.tv_sec = 0;   
        timeout.tv_usec = 200000; 
		FD_ZERO(&rd);
		FD_SET(g_uart2fd, &rd);        
		if (select(g_uart2fd + 1, &rd, NULL, NULL,  &timeout) < 0)
		{
			perror("select error\n");
			continue;			
		}
		if(FD_ISSET(g_uart2fd, &rd))
		{   
            memset(buf,0,sizeof(buf));
            res = read(g_uart2fd, buf, UART_QUEUE_BUF_SIZE-10);
            if(res>0){
                for(i=0;i<res;i++){
                  QueueAddData(&stUartQueue,buf[i]);
                }   				
                if(uartfcallbak){
                  uartfcallbak(buf);				
                }
            }
		}
	}  
#else
    FILE * fdTest = fopen("/home/gps_20160817090212_2406.log","r");
    if(fdTest == NULL){
       printf("not find /home/gps_20160817090212_2406.log");
       close(fdTest);
       return ;
    }
    while(g_uart_thread_flag) 
	{
       usleep(1000000);   //sleep(1);		      
       uartfcallbak = g_uart_fun_callbak;
       if(feof(fdTest))
       {
          fseek(fdTest,0L,SEEK_SET);//cycle read file
          continue;
       }       
       memset(buf,0,sizeof(buf));
       res = fread(buf,1,74, fdTest);
       //res = fgets(buf,sizeof(buf),fdTest);
       if(res!= 0 && res!= -1)
	   {
           for(i=0;i<res;i++)
           {
             QueueAddData(&stUartQueue,buf[i]);
           }   
           if(uartfcallbak)
           {
             uartfcallbak(buf);				
           }   
	   }     
	}
    close(fdTest);
#endif
    close(g_uart2fd);
}


static void init_uart_handle_thread(void) 
{ 	
	pthread_t tfid,tfid2,tfid3;
    int ret = 0;

	ret = pthread_create(&tfid, NULL, (void*)cdr_get_uart_data_thread, NULL);
    if (ret != 0)
    {
        printf("pthread_create failed, %d, %s\n", errno, strerror(errno));
        return ;
    }    
    pthread_detach(tfid);

	ret = pthread_create(&tfid2, NULL, (void*)cdr_proc_uart_data_thread, NULL);
    if (ret != 0)
    {
        printf("pthread_create failed, %d, %s\n", errno, strerror(errno));
        return ;
    }    
    
    ret = pthread_create(&tfid3, NULL, (void*)cdr_uart_data_ack_thread, NULL);
    if (ret != 0)
    {
        printf("pthread_create failed, %d, %s\n", errno, strerror(errno));
        return ;
    }
    pthread_detach(tfid3);
    
}

int cdr_uart_deinit(void)
{    
	g_uart_thread_flag = 0;
	return 1;
}


void cdr_uart_setevent_callbak(cdr_uart_callback callback)
{
	if(callback == NULL)
	{
		printf("[%s->%d] callbak is NULL\r\n",__FUNCTION__,__LINE__);
		return;
	}
    
	g_uart_fun_callbak = callback;

    if(g_uart_fun_callbak  != NULL)	
    {
		printf("%s register g_uart_fun_callbak . \r\n",__FUNCTION__);
    }

}


void cdr_uart2_printf(char *ch)
{
	char chUart[200];
	memset(chUart,0x00,200);
	sprintf(chUart,"%s",ch);
	write(g_uart2fd,chUart,strlen(chUart)+1);//���ԣ�������ͨ�ţ���������406�������⡣��
}


int Uart2SendData(unsigned char *pucData,int iLen)
{
    int res = 0;
    res = write(g_uart2fd,pucData,iLen);
    
    return res;
}

static int  int_uart()
{
    int ret = 0;
    cdr_system("himm  0x200F00D0  0x03");//uart tx 
    cdr_system("himm  0x200f00CC  0x03");//uart rx
    usleep(10000);    

	g_uart2fd = open(UART_DEVICE, O_RDWR);
    if(g_uart2fd < 0)
    {
        perror(UART_DEVICE);
	    return -1;
    }

	fcntl(g_uart2fd,F_SETFL,O_NONBLOCK);//O_NONBLOCK :������  0 :���� 

    set_speed(g_uart2fd,9600);		
    
    if (set_Parity(g_uart2fd,8,1,'O') == FALSE)//��У��
    {
		printf("Set Parity Error\n");
		return -1;
	}
	
    return ret;
}

int cdr_uart_init(void)
{    
    int_uart();
    g_uart_thread_flag = 1;	

    RecvBagQueueInit(&stRecvBagQueue);
    RetuBagQueueInit(&stRetuBagQueue);

    init_uart_handle_thread();
    return 1;
}

