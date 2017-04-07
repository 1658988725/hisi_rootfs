#ifndef __CDR_APP_SERVICE_H_
#define __CDR_APP_SERVICE_H_


#define WIFI_CFG_SIZE 10 //字段个数，即共有10个参数，对应9个逗号
#define SIZE_OF_SSID  20

//wifi
#pragma pack(1)
typedef struct{
	char ucWifiMode[2];
    
	char ucApSsid[20];
	char ucApPwd[20];

	char ucStaSsid[20];
	char ucStaPwd[20];    

    char  ucDhcp[2];
    char ucIpAddr[16];
    char ucGateWay[16];    
    char ucNetMask[16];
    char ucDns[16];    

}sWifiCfg;
#pragma pack()

sWifiCfg g_sWifiCfgNumber;

int Get_BootVoiceCtrl();
void cdr_app_service_init(void);
void ack_capture_update(char *jpgname);
int cdr_system(const char * cmd);

int cdr_system_reset();
int Set_MuteVolume(unsigned char ucVolMuteFlag);//ao
int Set_AI_MuteVolume(unsigned char ucVolMuteFlag);


int SetWifiStaMode(void);
int SetWifiApMode();

int cdr_mp4cut_finish(void* info,void *pUserData);

int gps_update_system_time(char *ptimestr,int len);
int cdr_AssociatedVideo(int flag,int len);
void send_ack_data(int clientIndex);
int Set_VolumeCtrl(int iValueIndex);
int Set_VolumeRecordingSensitivity(int iIndex);


#endif

