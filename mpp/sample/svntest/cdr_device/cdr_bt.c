#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <sys/wait.h>
#include <memory.h>
#include "cdr_bt.h"
#include "cdr_led.h"

#include "cdr_config.h"
#include "cdr_comm.h"
#include "cdr_uart.h"

static int bt_thread_flag = 0;
static int bt_cmd = CDRBT_CMD_NULL;

void bt_close(void);

/*
???????
0x07C muxctrl_reg31 GPIO1_0   0x07c
0x080 muxctrl_reg32 GPIO1_1   0x080
0x084 muxctrl_reg33 GPIO1_2   0x084

//1 set GPIO mode
himm 0x200f007C 0x0
himm 0x200f0080 0x0
himm 0x200f0084 0x0
//2 set GPIO out mode
himm 0x20150400 0x07 //1_0 1_1 1_2
//3 set GPIO value   //1_0 1_1 1_2
himm 0x2015001C 0x00

//ble data statu
GPIO 6_1  0x0C4
GPIO 6_5  0x0D4
//1set ?? gpio
himm 0x200f00C4 0x0
himm 0x200f00D4 0x0
//2set out/in mode
himm 0x201A0400 0x0 //??????????6?io
//3set 

//GPIO1_2
*/
void bt_hw_iocfg()
{
    //set 1_0 1_1 1_2
	cdr_system("himm 0x200f007C 0x0");
	cdr_system("himm 0x200f0080 0x0");
	cdr_system("himm 0x200f0084 0x0");//set gpio mode
	cdr_system("himm 0x20150400 0x07");//set output mode	
	cdr_system("himm 0x2015001C 0x00");//set low value

    //mode is 6_5 电话中为1 音乐0
	cdr_system("himm 0x200f00d4  0x00");//d12 led gpio6_5 
	cdr_set_gpio_mode(0x201A0400,5,0);  //6-5 input mode

	bt_close();

}

/*
PIO21(pin 35) ??: ????
??: ???
 GPIO 1_1
*/
void bt_vol_add(void)
{
	cdr_system("himm 0x20150008 0xFF");
	usleep(500 * 1000);
	cdr_system("himm 0x20150008 0x00");
	printf("vol + ...\r\n");
}

/*
PIO18(pin 25) ??: ????
??: ???
GPIO 1_2
*/
void bt_vol_sub(void)
{
	cdr_system("himm 0x20150010 0xFF");
	usleep(500 * 1000);
	cdr_system("himm 0x20150010 0x00");
	printf("vol - ...\r\n");
}

/*
??: ??1S
??: ??2S
??: ??/??,??/??
????????/??

PIO9(pin 30) ?  ??p22??,?????
????:???? ??1S ??
???: ????
???:??1S???????,????????
???:????,????
???: ????????,?????
?????:????/????
*/
void bt_ctrl(int ms)
{
	cdr_system("himm 0x20150004 0xFF");
	usleep(ms * 1000);
	cdr_system("himm 0x20150004 0x00");
}

//??: ??1S  GPIO 1_0
void bt_open(void)
{
	 printf("open bt...\n");	 

	if(strcmp(CDR_HW_VERSION,"vh.1.1") == 0)
	{
		//VH.1.1 open的时候需要给电
		cdr_system("himm 0x200f0100 0x01");
		cdr_set_gpio_mode(0x201c0400,0,0x01);
		cdr_system("himm 0x201c0004 0x00");
		sleep(2);
	}
   
	bt_ctrl(1100);
    cdr_led_contr(LED_BLUE,LED_STATUS_DFLICK);
}

//GPIO8_0 20170113
//vh.1.0 版本硬件GPIO8_0 接在BT模块rst口.低电平复位
//vh.1.0 版本硬件GPIO8_0 接在BT模块电源引脚，高电平关闭
void bt_close(void)
{
	//VH.1.0 关闭蓝牙的方法为:reset复位一下.
	if(strcmp(CDR_HW_VERSION,"vh.1.0") == 0)
	{
		cdr_system("himm 0x200f0100 0x01");//set gpio mode
		cdr_set_gpio_mode(0x201c0400,0,0x01);
		cdr_system("himm 0x201c0004 0x00");//set low value
		sleep(2);
		cdr_system("himm 0x201c0004 0xff");//set low value
		sleep(1);
		cdr_led_contr(LED_BLUE,LED_STATUS_OFF);
	}

	//VH.1.0 关闭蓝牙方法为直接给蓝牙模块断电
	if(strcmp(CDR_HW_VERSION,"vh.1.1") == 0)
	{
		cdr_system("himm 0x200f0100 0x01");
		cdr_set_gpio_mode(0x201c0400,0,0x01);
		cdr_system("himm 0x201c0004 0xff");
		cdr_led_contr(LED_BLUE,LED_STATUS_OFF);
	}
}

/*
??  ????:???? GPIO 1_0
*/
void bt_answer(void)
{
	bt_ctrl(400);
	printf("answer the phone...\r\n");
}

void bt_phone(void)
{
	bt_ctrl(400);
	printf("answer or hold up the phone...\r\n");
}


void bt_phone_cut()
{
  printf("cut phone ..\n");
  bt_ctrl(1000);

}

void bt_hang_up()
{

	bt_ctrl(400);
	printf("hold up the phone...\r\n");

}

/*
1, PIO9(pin 30)   ??GPIO 1_0??IO
PIO9(pin 30) ????:???? ??1S ??
*/
void bt_phone_reject(void)
{
    printf("reject the phone\n");
 
	bt_ctrl(1000);
}


/*
GPIO6_1
GPIO6_5
来电或者通话PIO8置高，同时PIO17置高
音乐播放时候PIO8置高，同时PIO17置地
*/
unsigned int read_data_gpio_value()
{
    unsigned int uiRegValue = 0;
#if 0	
    
    //reg_read(0x201A0008, &uiRegValue);//GPIO6_1
    HI_MPI_SYS_GetReg(0x201A0008, &uiRegValue);
    
    if(uiRegValue == 0x02)//audio
    {
       //reg_read(0x201A0080, &uiRegValue);       
       HI_MPI_SYS_GetReg(0x201A0080, &uiRegValue);
       if(uiRegValue == 0x20)//telephone audio
       {
         return 1;//1
       }
       if(uiRegValue == 0x0)//mp3 audio
       {
         return 2;
       }
    }else{
       return 0;//no audio
    }
#else

	//只检测PIO017 6-5
	HI_MPI_SYS_GetReg(0x201A0080, &uiRegValue);
	if(uiRegValue == 0x20)//telephone audio
	{
		return 1;
	}


#endif
    
    return uiRegValue;
}

//get bt status.
int cdr_bt_get_status()
{
	return g_cdr_systemconfig.bluetooth;
}

int cdr_bt_handle_thread(void)
{
	int iTempValue;
	static unsigned char ucSetAIFlag1 = 0;
	while (bt_thread_flag)
	{
        iTempValue = read_data_gpio_value();
        if(iTempValue == 0x01){
          if(ucSetAIFlag1 == 0x00){
            Set_AI_MuteVolume(0x00);//来电时的静音
            ucSetAIFlag1 = 0x01;
			cdr_led_contr(LED_BLUE, LED_STATUS_FLICK);
          }
        }else{
          if(ucSetAIFlag1 == 0x01){
            Set_AI_MuteVolume(0x01);//打开声音
            ucSetAIFlag1 = 0x00;
			cdr_led_contr(LED_BLUE, LED_STATUS_DFLICK);
          }
        }

        switch (bt_cmd)
        {
        case CDRBT_CMD_OPEN:
        	bt_open();
        	break;
        case CDRBT_CMD_STOP:
        	bt_close();
        	break;
            
 
        case CDRBT_CMD_PHONE:
        	bt_phone();
        	break;
            
        case CDRBT_CMD_REJECT:
        	bt_phone_reject();
        	break;
        case CDRBT_CMD_CUT:
         bt_phone_cut();
            break;
            
        case CDRBT_CMD_VOLADD:
        	bt_vol_add();
        	break;
        case CDRBT_CMD_VOSUB:
        	bt_vol_sub();
        	break;
        default:
        	break;
        }
		bt_cmd = CDRBT_CMD_NULL;
		usleep(10000);		
	}
	return 0;
}


int cdr_bt_cmd(eCDRBT_CMD type)
{
	bt_cmd = type;
	return 0;
}

/*
1,蓝牙默认是认为是关机；
2，bt_close()连续发两次；第一次为开则下次为关；第一次为关则下次为开
*/
int cdr_bt_open()
{
    //sleep(5);//等待蓝牙模块状态稳定
    if(g_cdr_systemconfig.bluetooth == 0x01)
    {    
    	  bt_open();       
		  cdr_uart2_printf("$TEST,BT,OPEN\r\n");
		  
    }else{       
      printf("bluetooth close\n");
      //bt_close();
    }
	return 0;
}

int cdr_bt_init(void)
{           
    bt_hw_iocfg();//gpio 初始化      
    
#if 0  
    //sleep(10);
    //sleep(5);
    //sleep(4);
    if(g_cdr_systemconfig.bluetooth == 0x01)
    {    
  	  bt_open();       
    }else{ 
      printf("g_cdr_systemconfig.bluetooth == 0x00\n");
      //bt_close();
    }
#endif    

	pthread_t tfid;
	int ret = 0;
	bt_thread_flag = 1;
	ret = pthread_create(&tfid, NULL, (void *)cdr_bt_handle_thread, NULL);
	if (ret != 0)
	{
		printf("[%s] cdr_bt_handle_thread failed\n", __FUNCTION__);
		return -1;
	}
	pthread_detach(tfid);
    cdr_led_contr(LED_BLUE, LED_STATUS_DFLICK);
	return 0;
}

int cdr_bt_deinit(void)
{
	bt_thread_flag = 0;
	return 0;
	//bt_close();
}
