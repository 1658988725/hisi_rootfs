#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <sys/time.h>
#include "cdr_wifi.h"
#include "cdr_config.h"
#include "rtsp_client.h"


unsigned char g_ucPushTimerOutFlag = 0;
extern int g_RtpSessionUdpSock;
extern char g_cUrlBuff[200];
extern unsigned char g_ucRealtimePushAVFlag;
void DeinitPushActiveTime(void);

/*
1,60s��ʱ�䵽��
wifiMode =2 Ϊ�����л�ģʽ���������һ��STAģʽ���ӣ������õ��ȵ�һ�������޷��������ӻ������Ӻ������ȵ�Ͽ�ʱ�Զ��л���APģʽ����ģʽ�²���cdr_syscfg.xml���޸ġ�
*/
void PushTimerProcess(int iSigno)
{
  switch (iSigno){
   case SIGALRM:
        #if(1)
        g_ucPushTimerOutFlag++;
        printf("check active time..\n");
        if(g_ucPushTimerOutFlag >= 0x02){//��ʱʱ���رյ�ǰ����
            printf("��ʱû���յ�����ָ��\n");
            if(g_ucRealtimePushAVFlag == 0x01){
               DoTearDown(g_cUrlBuff);
               if(g_RtpSessionUdpSock != -1)close(g_RtpSessionUdpSock);//�費��Ҫclose��ȷ�ϲ���
               g_ucRealtimePushAVFlag = 0x00;
            }
            DeinitPushActiveTime(); 
        }    
        #endif
    break;   
   }
   return;
}


int StartPushActiveTime()
{
    struct itimerval value, ovalue;    

    signal(SIGALRM, PushTimerProcess);
    //Timeout to run first time
    value.it_value.tv_sec = 45;
    value.it_value.tv_usec = 0;
    //After first, the Interval time for clock
    value.it_interval.tv_sec = 45;
    value.it_interval.tv_usec = 0;
    
    setitimer(ITIMER_REAL, &value, &ovalue);

    return 0;
}

//�رն�ʱ��
void DeinitPushActiveTime()  
{  
    struct itimerval value;  
    value.it_value.tv_sec = 0;  
    value.it_value.tv_usec = 0;  
    value.it_interval = value.it_value;  
    setitimer(ITIMER_REAL, &value, NULL);  
}



