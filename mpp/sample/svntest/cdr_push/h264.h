
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PACKET_BUFFER_END            (unsigned int)0x00000000 //为什么结尾全是00？
//#define MAX_RTP_PKT_LENGTH     1400 //  MUT最大值1460 
#define MAX_RTP_PKT_LENGTH     500 //  MUT最大值1460 
#define H264                    96

typedef struct 
{
    /**//* byte 0 */
    unsigned char csrc_len:4;        /* expect 0  4. CSRC计数(CO: Obit  CSRC 计数包含了紧跟在固定头后的 CSRC标识符的个数。 */
    unsigned char extension:1;       /* expect 1, see RTP_OP below 3.扩展位(X): lbit    如果扩展位置 1，固定头后必须跟了一个头扩展*/
    unsigned char padding:1;         /* expect 0 的是第 0版)    2.填充位(P): l bit    如果填充位置 1，末尾有一个或者多个附加的非有效载荷的填充字节。填充的最后一个  字节是该忽略的填充数目，包括本身所占一个字节。填充在按照固定块大小加密的加密算法  中或者在低层的协议数据单元中装载几个 RTP包时可能需要*/    
  	unsigned char version:2;         /* expect 2 1.版本(V): 2bit RTP版本信息。目前版本是 2。 (RTP第一版草案是版本 1，最初应用在“vat"音频工具中的是第 0版)*/
    /**//* byte 1 */
    unsigned char payload:7;         /* RTP_PAYLOAD_RTSP 6.有效载荷类型(PT) : 7bit  定义 RTP 有效载荷的格式，通过应用程序定义具体的说明。序言可以指定和有效载荷格式相对应的有效载荷码的默认静态影像码。附加的有效载荷码可以通过非 RTP 方式动态定义。在 RFC 3551 中定义了一系列音频和视频的默认映射初集。会话中，RTP元可以修改有效载荷的类型，但是该域不能用于复用单独的媒体流。  对于有效载荷类型无法识别的数据包，接收方一律忽略。*/
    unsigned char marker:1;          /* expect 1 5.标记(M) : 1 bit  在序言中定义了该标记的具体解释，目的是允许重要事件在包流中标记 l 止来。序言可以定义附加的标记位，或者通过改变有效载荷类型域中的数据位来指示没有标记位。*/
    /**//* bytes 2, 3 */
    unsigned short seq_no;           /*7.序列号(Sequence Number) : 16bit  每发送一个 RTP数据包序列号就增加 1，这样接收方可以检测到数据包的丢失重新保存包序列。序列号的初始值是随机的，这样可以加大被人窃取攻击的难度。 */  
    /**//* bytes 4-7 */
    unsigned  long timestamp;        /*8.时间戳(Timestamp) : 32bit时间戳反映了 RTP 数据包的第一个字节的采样信息。*/  
    /**//* bytes 8-11 */
    unsigned long ssrc;              /* stream number is used here.SSRC 段定义了同步源。标识应该是随机选择的，防止在同一个 RTP会话中有两个同名的同步源。 */
} RTP_FIXED_HEADER;



typedef struct {
//byte 0
/*
nal_unit_type. 这个 NALU 单元的类型. 简述如下:
 0 没有定义
1-23 NAL单元 单个 NAL 单元包.
24 STAP-A 单一时间的组合包
24 STAP-B 单一时间的组合包
26 MTAP16 多个时间的组合包
27 MTAP24 多个时间的组合包
28 FU-A 分片的单元
29  FU-B 分片的单元
30-31 没有定义
*/
	unsigned char TYPE:5;
    unsigned char NRI:2;//nal_ref_idc. 取 00 ~ 11, 似乎指示这个 NALU 的重要性, 如 00 的 NALU 解码器可以丢弃它而不影响图像的回放. 不过一般情况下不太关心这个属性.
	unsigned char F:1;  //forbidden_zero_bit. 在 H.264 规范中规定了这一位必须为 0  若为1，则说明是网络错误
         
} NALU_HEADER; /* 1 BYTES */



//Fragmentation Units (FUs) 分片的单元
typedef struct {
    //byte 0
    unsigned char TYPE:5;
	unsigned char NRI:2; 
	unsigned char F:1;    
                         
} FU_INDICATOR; /* 1 BYTES  指示仪表*/

typedef struct {
    //byte 0
    unsigned char TYPE:5;
	unsigned char R:1;
	unsigned char E:1;
	unsigned char S:1;    
} FU_HEADER; /**//* 1 BYTES */




