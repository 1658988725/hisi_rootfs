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

extern char g_addressBuffer[INET_ADDRSTRLEN];  //16 存储本设备端的ip

#if(0)

/*
1,60s的时间到了
wifiMode =2 为智能切换模式；仅仅针对一次STA模式连接，在设置的热点一分钟仍无法建立连接或建立连接后被迫与热点断开时自动切换到AP模式；该模式下不对cdr_syscfg.xml做修改。
*/
void AutoSwitchApMode(int iSigno)
{
  switch (iSigno){
   case SIGALRM:
        printf("timer check sta net connect status\n");
        if(GetNetConnectStatus() != 0x00){//sta 联接失败或，中间突然断开了
           usleep(10000);
           if(GetNetConnectStatus() != 0x00){//有时会出现信号不好情型
            printf("autor cut to ap mode start\n");
            DeinitWifiTime();              
            SetWifiApMode();        //修改为AP模式
           }
        }
    break;   
   }
   return;
}

//开启定时器sta获取ip
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

//关闭定时器
void DeinitWifiTime()  
{  
    struct itimerval value;  
    value.it_value.tv_sec = 0;  
    value.it_value.tv_usec = 0;  
    value.it_interval = value.it_value;  
    setitimer(ITIMER_REAL, &value, NULL);  
}


/*
定时1分钟去获取本机ip
若失败，则更改为ap模式
*/
int AutoSwitchProcess()
{
    //开启定时器，
    StartWifiTime();
}
#endif


