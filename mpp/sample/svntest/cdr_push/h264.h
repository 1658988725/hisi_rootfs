
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PACKET_BUFFER_END            (unsigned int)0x00000000 //Ϊʲô��βȫ��00��
//#define MAX_RTP_PKT_LENGTH     1400 //  MUT���ֵ1460 
#define MAX_RTP_PKT_LENGTH     500 //  MUT���ֵ1460 
#define H264                    96

typedef struct 
{
    /**//* byte 0 */
    unsigned char csrc_len:4;        /* expect 0  4. CSRC����(CO: Obit  CSRC ���������˽����ڹ̶�ͷ��� CSRC��ʶ���ĸ����� */
    unsigned char extension:1;       /* expect 1, see RTP_OP below 3.��չλ(X): lbit    �����չλ�� 1���̶�ͷ��������һ��ͷ��չ*/
    unsigned char padding:1;         /* expect 0 ���ǵ� 0��)    2.���λ(P): l bit    ������λ�� 1��ĩβ��һ�����߶�����ӵķ���Ч�غɵ�����ֽڡ��������һ��  �ֽ��Ǹú��Ե������Ŀ������������ռһ���ֽڡ�����ڰ��չ̶����С���ܵļ����㷨  �л����ڵͲ��Э�����ݵ�Ԫ��װ�ؼ��� RTP��ʱ������Ҫ*/    
  	unsigned char version:2;         /* expect 2 1.�汾(V): 2bit RTP�汾��Ϣ��Ŀǰ�汾�� 2�� (RTP��һ��ݰ��ǰ汾 1�����Ӧ���ڡ�vat"��Ƶ�����е��ǵ� 0��)*/
    /**//* byte 1 */
    unsigned char payload:7;         /* RTP_PAYLOAD_RTSP 6.��Ч�غ�����(PT) : 7bit  ���� RTP ��Ч�غɵĸ�ʽ��ͨ��Ӧ�ó���������˵�������Կ���ָ������Ч�غɸ�ʽ���Ӧ����Ч�غ����Ĭ�Ͼ�̬Ӱ���롣���ӵ���Ч�غ������ͨ���� RTP ��ʽ��̬���塣�� RFC 3551 �ж�����һϵ����Ƶ����Ƶ��Ĭ��ӳ��������Ự�У�RTPԪ�����޸���Ч�غɵ����ͣ����Ǹ��������ڸ��õ�����ý������  ������Ч�غ������޷�ʶ������ݰ������շ�һ�ɺ��ԡ�*/
    unsigned char marker:1;          /* expect 1 5.���(M) : 1 bit  �������ж����˸ñ�ǵľ�����ͣ�Ŀ����������Ҫ�¼��ڰ����б�� l ֹ�������Կ��Զ��帽�ӵı��λ������ͨ���ı���Ч�غ��������е�����λ��ָʾû�б��λ��*/
    /**//* bytes 2, 3 */
    unsigned short seq_no;           /*7.���к�(Sequence Number) : 16bit  ÿ����һ�� RTP���ݰ����кž����� 1���������շ����Լ�⵽���ݰ��Ķ�ʧ���±�������С����кŵĳ�ʼֵ������ģ��������ԼӴ�����ȡ�������Ѷȡ� */  
    /**//* bytes 4-7 */
    unsigned  long timestamp;        /*8.ʱ���(Timestamp) : 32bitʱ�����ӳ�� RTP ���ݰ��ĵ�һ���ֽڵĲ�����Ϣ��*/  
    /**//* bytes 8-11 */
    unsigned long ssrc;              /* stream number is used here.SSRC �ζ�����ͬ��Դ����ʶӦ�������ѡ��ģ���ֹ��ͬһ�� RTP�Ự��������ͬ����ͬ��Դ�� */
} RTP_FIXED_HEADER;



typedef struct {
//byte 0
/*
nal_unit_type. ��� NALU ��Ԫ������. ��������:
 0 û�ж���
1-23 NAL��Ԫ ���� NAL ��Ԫ��.
24 STAP-A ��һʱ�����ϰ�
24 STAP-B ��һʱ�����ϰ�
26 MTAP16 ���ʱ�����ϰ�
27 MTAP24 ���ʱ�����ϰ�
28 FU-A ��Ƭ�ĵ�Ԫ
29  FU-B ��Ƭ�ĵ�Ԫ
30-31 û�ж���
*/
	unsigned char TYPE:5;
    unsigned char NRI:2;//nal_ref_idc. ȡ 00 ~ 11, �ƺ�ָʾ��� NALU ����Ҫ��, �� 00 �� NALU ���������Զ���������Ӱ��ͼ��Ļط�. ����һ������²�̫�����������.
	unsigned char F:1;  //forbidden_zero_bit. �� H.264 �淶�й涨����һλ����Ϊ 0  ��Ϊ1����˵�����������
         
} NALU_HEADER; /* 1 BYTES */



//Fragmentation Units (FUs) ��Ƭ�ĵ�Ԫ
typedef struct {
    //byte 0
    unsigned char TYPE:5;
	unsigned char NRI:2; 
	unsigned char F:1;    
                         
} FU_INDICATOR; /* 1 BYTES  ָʾ�Ǳ�*/

typedef struct {
    //byte 0
    unsigned char TYPE:5;
	unsigned char R:1;
	unsigned char E:1;
	unsigned char S:1;    
} FU_HEADER; /**//* 1 BYTES */




