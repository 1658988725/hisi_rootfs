#include <linux/sockios.h>  
#include <sys/socket.h>  
#include <sys/ioctl.h>  
#include <linux/if.h>  
#include <string.h>  
#include <stdio.h>  
#include <stdlib.h>  
#include <unistd.h>  


#define ETHTOOL_GLINK        0x0000000a /* Get link status (ethtool_value) */  
  
typedef enum { IFSTATUS_UP, IFSTATUS_DOWN, IFSTATUS_ERR } interface_status_t;  
  
typedef signed int u32;  
  
/* for passing single values */  
struct ethtool_value  
{  
    u32    cmd;  
    u32    data;  
};  
  
interface_status_t interface_detect_beat_ethtool(int fd, char *iface)  
{  
    struct ifreq ifr;  
    struct ethtool_value edata;  
     
    memset(&ifr, 0, sizeof(ifr));  
    strncpy(ifr.ifr_name, iface, sizeof(ifr.ifr_name)-1);  
  
    edata.cmd = ETHTOOL_GLINK;  
    ifr.ifr_data = (caddr_t) &edata;  
  
    if (ioctl(fd, SIOCETHTOOL, &ifr) == -1)  
    {  
        perror("ETHTOOL_GLINK failed ");  
        return IFSTATUS_ERR;  
    }  
  
    return edata.data ? IFSTATUS_UP : IFSTATUS_DOWN;  
}  
  
int GetNetConnectStatus1()  
{  
    FILE *fp;  
    interface_status_t status;  
    char buf[512] = {'\0'};  
    char hw_name[10] = {'\0'};  
    char *token = NULL;  
  
    /* ��ȡ�������� */  
    if ((fp = fopen("/proc/net/dev", "r")) != NULL)  
    {  
        while (fgets(buf, sizeof(buf), fp) != NULL)  
        {  
            //if(strstr(buf, "eth") != NULL)  
            if(strstr(buf, "ra0") != NULL)      
            {         
                token = strtok(buf, ":");  
                while (*token == ' ') ++token;  
                strncpy(hw_name, token, strlen(token));  
            }  
        }  
    }  
    fclose(fp);  
//����һ���鿴һ���ļ��ļ��������˵�Ƚϼ�  
#if 1  
    char carrier_path[512] = {'\0'};  
      
    memset(buf, 0, sizeof(buf));   
    snprintf(carrier_path, sizeof(carrier_path), "/sys/class/net/%s/carrier", hw_name);  
    if ((fp = fopen(carrier_path, "r")) != NULL)  
    {  
        while (fgets(buf, sizeof(buf), fp) != NULL)  
        {  
            if (buf[0] == '0')  
            {  
                status = IFSTATUS_DOWN;  
            }  
            else  
            {  
                status = IFSTATUS_UP;  
            }  
        }  
    }  
    else  
    {  
        perror("Open carrier ");  
    }  
    fclose(fp);  
#endif  
  
//���������ú����ɣ��е㸴�ӣ�����Ҳ��һ����Ч�İ취  
#if 1  
    int fd;  
      
    if((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)  
    {  
        perror("socket ");  
        exit(0);  
    }  
    status = interface_detect_beat_ethtool(fd, hw_name);  
    close(fd);  
#endif  
  
    switch (status)  
    {  
        case IFSTATUS_UP:  
            printf("%s : link up\n", hw_name);  
            break;  
          
        case IFSTATUS_DOWN:  
            printf("%s : link down\n", hw_name);  
            break;  
          
        default:  
            printf("Detect Error\n");  
            break;  
    }  
  
    return 0;  
}  


int ParseNetStatu(unsigned char *pSrc)
{
  unsigned char ucDstBuf[20] = {0};
  int i = 0; 
  
  if(NULL==pSrc)return -1;

  //ra0       connStatus:Connected(AP: E-EYE[88:25:93:A2:E3:E4])
  char *pTemp = strstr((char *)pSrc,":");
  printf("%s\n",pTemp);
  pTemp++;
 
  for(i=0;i<20;i++){
      if(*pTemp=='(')break;
      ucDstBuf[i] = *pTemp++;
  }
  ucDstBuf[i] = '\0';
  printf("status:%s",ucDstBuf);

  if(strcmp("Connected",(char*)ucDstBuf) == 0x00){
     return 0;
  }else{
     return -1;
  }
  
}

#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
int GetNetConnectStatus()
{
    FILE *stream;
    unsigned char ucRes = 0;    
    char buf[100] = {0};
    stream = popen( "iwpriv ra0  connStatus", "r" ); //����iwpriv ra0  connStatus���������� ͨ���ܵ���ȡ����r����������FILE* stream
    //wstream = fopen( "TestNetStatus.txt", "w+"); //�½�һ����д���ļ�
    fread( buf, sizeof(char), sizeof(buf), stream); //���ո�FILE* stream����������ȡ��buf��
    //fwrite( buf, 1, sizeof(buf), wstream );//��buf�е�����д��FILE *wstream��Ӧ�����У�Ҳ��д���ļ���
    //printf("=====================buf:%s\n",buf);
    //����buf�ַ�����
    ucRes = ParseNetStatu((unsigned char*)buf);
   
    pclose(stream);

    return ucRes;
}


