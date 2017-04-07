/************************************************************************	
** Filename: 	cdr_device.c
** Description:  
** Author: 	xjl
** Create Date: 
** Version: 	v1.0

	Copyright(C) 2011 e-eye CO.LTD. ShenZhen <www.e-eye.cn>

*************************************************************************/
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <memory.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include "cdr_bma250e.h"
#include "cdr_lt8900.h"
#include "cdr_app_service.h"
#include "cdr_XmlLog.h"
#include "cdr_gps_data_analyze.h"
#include "cdr_bubiao_analyze.h"
#include "cdr_device_discover.h"
#include "cdr_device.h"
#include "cdr_writemov.h"
#include "cdr_bt.h"
#include "cdr_led.h"
#include "cdr_key_check.h"
#include "cdr_gp.h"
#include "cdr_fm.h"
#include "cdr_uart.h"
#include "cdr_queue.h"
#include "queue_bag.h"
#include "cdr_config.h"
#include "cdr_comm.h"
#include "cdr_mp4_api.h"
#include "cdr_stop_check.h"
#include "cdr_mpp.h"


char g_cdr_device_app_stop_flag = CDR_POWER_ON;
GPS_INFO g_GpsInfo;

#define SINGLE_PRESS 0x01
#define LONG_PRESS   0x02
#define DOUBLE_PRESS 0x03
#define HOLD_KEY     0x04

static unsigned char ucCalibTimeFlag = 0x00;
int cdr_start_factest(void);

int cdr_lt8900_cb_pro(int iKeyValue)
{
    int iTempValue = 0;
    printf("iKeyValue:%02x\n",iKeyValue);

    iTempValue = read_data_gpio_value();

	//如果bt关闭状态.
	if(cdr_bt_get_status() == 0) iTempValue = 0;

    switch(iKeyValue)
    {
		case SINGLE_PRESS:
		if(iTempValue == 0x01)		//来电状态
		{
			cdr_bt_cmd(CDRBT_CMD_PHONE);//挂断/接听电话(同一个命令)			
		}
		else
		{          
			cdr_uart2_printf("$TEST,2.4G,TRIGGER\r\n");
			cdr_capture_jpg(0);  
			cdr_AssociatedVideo(g_cdr_systemconfig.photoWithVideo,CDR_CUTMP4_EX_TIME);
		}
     break;
     case LONG_PRESS:
        if(iTempValue == 0x01)
        {
          cdr_bt_cmd(CDRBT_CMD_REJECT);
        }else{

		printf("LONG_PRESS \n");
		//强制录视频 
		cdr_AssociatedVideo(1,CDR_CUTMP4_EX_TIME);
		cdr_play_audio(CDR_AUDIO_DIDI,0);
          //StartForcedRecord();         
          //printf("StartForcedRecord...\n");
        }
        break;
     case DOUBLE_PRESS:
        if(iTempValue == 0x01)
        {
          cdr_bt_cmd(CDRBT_CMD_CUT);//长按1S语音切换到手机，再长按切换到蓝牙
        }else{          
          cdr_bt_cmd(CDRBT_CMD_PHONE); //空闲状态单击，激活手机语音助手（需手机支持）
        }

        break;
     case HOLD_KEY:  
		printf("HOLD_KEY \n");
		cdr_AssociatedVideo(1,CDR_CUTMP4_EX_TIME);		
		cdr_play_audio(CDR_AUDIO_DIDI,0);
        break;
     default:
        break;     
    }
    return 0;
}


///1.拍照大图
///2.cdr_AssociatedVideo(2, 代表关联视频，结束的时候需要截取视频.
int cdr_bma250_cb_pro(unsigned int uiRegValue)
{  
    cdr_capture_jpg(1); 
    cdr_AssociatedVideo(2,CDR_CUTMP4_EX_TIME);
	cdr_uart2_printf("$TEST,GSENSOR,TRIGGER\r\n");
	return 0;
}

//retur 1 pown on
int cdr_get_powerflag(void)
{
	return g_cdr_device_app_stop_flag;
}

int cdr_system_wait(void)
{
	while(cdr_get_powerflag())		
	{
		sleep(1);
	}
	return 0;
}

int cdr_stop_check_cb_pro(unsigned int uiRegValue)
{
    if(uiRegValue != 0x00) 
    {
      g_cdr_device_app_stop_flag = CDR_POWER_ON;
      return -1;
    }   	
	printf("******************************************\r\n");
	printf("***********CDR USB POWER OFF**************\r\n");
	printf("******************************************\r\n");
	//Tell test.
	//cdr_system("echo -e \"10.CDR USB POWER OFF \r\n \" >> /dev/ttyAMA2");
	        
    g_cdr_device_app_stop_flag = CDR_POWER_OFF;   //tell app
	
    return 0;
}

int cdr_key_check_cb_pro(unsigned int uiRegValue)
{
    
    if(uiRegValue != 0x00)   return -1;   
	cdr_play_audio(CDR_AUDIO_RESETSYSTEM,0);
    printf("key press down.\n");
    cdr_system_reset();
    
    return 0;
}

/*gps 校时*/
int GpsCalibTime(GPS_INFO *GPS)
{
	char ucTimeBuff[15] = {0};
	if(GPS->ucDataValidFlag == 0x00) return 0;    
	if(ucCalibTimeFlag == 0x00)
	{
		cdr_play_audio(CDR_GPS_OK,0);
		ucCalibTimeFlag = 0x01;
		sprintf(ucTimeBuff,"%d%02d%02d%02d%02d%02d",
		GPS->D.year,GPS->D.month,GPS->D.day,GPS->D.hour,GPS->D.minute,GPS->D.second);
		//printf("ucTimeBuff:%s\n",ucTimeBuff);           
		gps_update_system_time(ucTimeBuff,strlen(ucTimeBuff));
	}
	return 0;
}


/*
* analyze gps data get the use informnation
*/
int cdr_uart_cb_pro(char *pRevcBuff)
{
    int res = 0;
    static char ucDstBag[BUBIAO_MAX_LEN] = {0};
    //unsigned char *ucDstBagTemp = NULL;

    res = uart_get_dst_bag((char*)stUartQueue.pucBuf,ucDstBag);
    if(res == 0x00){
        gps_rmc_parse(ucDstBag,&g_GpsInfo);
        //show_gps(&g_GpsInfo);        
        GpsCalibTime(&g_GpsInfo);                
        memcpy(g_ucGpsDataBuff,ucDstBag,strlen(ucDstBag)+1);
        cdr_add_gp(ucDstBag);
        memset(ucDstBag,0,sizeof(ucDstBag));
        res = 0;        
    }else if(res == 0x01){//factory test
        cdr_uart2_printf("$TEST,START,OK\r\n");
    }else if(res == 0x02){
        //PrintTime();
        printf("接收到的数据:");
        PrintfString2Hex((unsigned char *)ucDstBag,GetBuBiaoPackLen());
        BagQueueAddData(&stRecvBagQueue, (unsigned char *)ucDstBag, GetBuBiaoPackLen());
    }else{
       res = -1;  
    }   
    return res;
}

int cdr_device_init(void)
{
    unsigned short usDeviceID = 0x9ea3;       

	cdr_uart_init();
    cdr_uart_setevent_callbak(cdr_uart_cb_pro);	

	cdr_start_factest();

	cdr_bt_init();
	
    cdr_lt8900_init(usDeviceID);
    cdr_lt8900_setevent_callbak(cdr_lt8900_cb_pro);
    
    cdr_stop_check_init();
    cdr_stop_check_setevent_callbak(cdr_stop_check_cb_pro);

	cdr_bma250_init(0x01);//设置加速度量程16g
    cdr_bma250_setevent_callbak(cdr_bma250_cb_pro);
    cdr_bma250_mode_ctrl(g_cdr_systemconfig.accelerationSensorSensitivity); 


    cdr_led_init();
    //cdr_led_contr(LED_RED,LED_STATUS_DFLICK);
    //cdr_led_contr(LED_BLUE,LED_STATUS_DFLICK);
	cdr_uart2_printf("$TEST,LED,OPEN\r\n");	

    cdr_key_check_init();
    cdr_key_check_setevent_callbak(cdr_key_check_cb_pro);

    cdr_qn8027_config();
    cdr_set_qn8027_freq(g_cdr_systemconfig.fmFrequency);//100*100 xM*100  10000 此值 应从文件中读取要修改
	cdr_uart2_printf("$TEST,FM,OPEN\r\n");
		
    cdr_bt_open();

	if(g_sdIsOnline == 1){
		cdr_uart2_printf("$TEST,SD,OK\r\n");
	}

	return 0;
}


int cdr_device_deinit(void)
{
	cdr_play_audio(CDR_AUDIO_USBOUT,0);
	cdr_add_log(eSTOP_CDR, NULL,NULL);
	cdr_uart_deinit();
    cdr_add_end_gp_record();
	cdr_system("ifconfig wlan0 down");    
	cdr_bt_deinit();
	cdr_lt8900_close();
	cdr_rec_realease();
	cdr_mp4dirlist_free();
	sleep(4);          	//Just sleep 4 seconds.  wait for save to sd.
	cdr_unload_sd();
	printf("[%s %d]\r\n",__FUNCTION__,__LINE__);  
	return 0;
}

//param: reg GPIO_DIR value.
//nIndex:0-7 GPIOx_0 - GPIOx_7
//mode:0 input 1 output.
int cdr_set_gpio_mode(unsigned int reg,int nIndex,int mode)
{
	//只能设定GPIO_DIR值.
	if((reg & 0x0400) != 0x0400)
		return -1;

	unsigned int nValue;
	if(HI_SUCCESS == HI_MPI_SYS_GetReg(reg,&nValue))
	{
		if(mode == 1)
		{
			nValue |= (0x01<<nIndex);
		}
		else
		{
			nValue &= ~(0x01<< nIndex);
		}
		HI_MPI_SYS_SetReg(reg,nValue);
	}
	return -1;	
}

//设置一个标志位.生产测试软件可用.
int cdr_start_factest(void)
{
	char ch[100];
	char mac[14];
	char chTM[20];
	

	struct tm tm_now;	
	snprintf(ch,sizeof(ch),"%s %s",__DATE__,__TIME__);
	strptime(ch, "%b %d %Y %H:%M:%S", &tm_now); 
	strftime(chTM, sizeof(chTM),"%Y%m%d%H%M%S", &tm_now);
	//strftime(ch, sizeof(ch),"%Y%m%d%H%M%S", &tm_now);
	memset(ch,0x00,sizeof(ch));
	cdr_get_host_mac(mac);
	snprintf(ch,sizeof(ch),"$TEST,DEVICE,%s,%s,%s,%s\r\n",g_cdr_systemconfig.sCdrNetCfg.apSsid,mac,CDR_FW_VERSION,chTM);
	cdr_uart2_printf(ch);
	return 0;
}

