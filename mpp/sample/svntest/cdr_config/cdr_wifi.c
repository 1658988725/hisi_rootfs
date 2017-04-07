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
#include "NetConnectStatu.h"
#include "cdr_app_service.h"

extern char g_addressBuffer[INET_ADDRSTRLEN];  //16 �洢���豸�˵�ip

#if(0)

/*
1,60s��ʱ�䵽��
wifiMode =2 Ϊ�����л�ģʽ���������һ��STAģʽ���ӣ������õ��ȵ�һ�������޷��������ӻ������Ӻ������ȵ�Ͽ�ʱ�Զ��л���APģʽ����ģʽ�²���cdr_syscfg.xml���޸ġ�
*/
void AutoSwitchApMode(int iSigno)
{
  switch (iSigno){
   case SIGALRM:
        printf("timer check sta net connect status\n");
        if(GetNetConnectStatus() != 0x00){//sta ����ʧ�ܻ��м�ͻȻ�Ͽ���
           usleep(10000);
           if(GetNetConnectStatus() != 0x00){//��ʱ������źŲ�������
            printf("autor cut to ap mode start\n");
            DeinitWifiTime();              
            SetWifiApMode();        //�޸�ΪAPģʽ
           }
        }
    break;   
   }
   return;
}

//������ʱ��sta��ȡip
int StartWifiTime()
{
    struct itimerval value, ovalue;    

    signal(SIGALRM, AutoSwitchApMode);
    //Timeout to run first time
    value.it_value.tv_sec = 60;
    value.it_value.tv_usec = 0;
    //After first, the Interval time for clock
    value.it_interval.tv_sec = 60;
    value.it_interval.tv_usec = 0;
    
    setitimer(ITIMER_REAL, &value, &ovalue);

    return 0;
}

//�رն�ʱ��
void DeinitWifiTime()  
{  
    struct itimerval value;  
    value.it_value.tv_sec = 0;  
    value.it_value.tv_usec = 0;  
    value.it_interval = value.it_value;  
    setitimer(ITIMER_REAL, &value, NULL);  
}


/*
��ʱ1����ȥ��ȡ����ip
��ʧ�ܣ������Ϊapģʽ
*/
int AutoSwitchProcess()
{
    //������ʱ����
    StartWifiTime();
}
#endif


