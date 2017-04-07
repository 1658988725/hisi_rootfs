#ifndef CDR_RTSP_PUSH_H
#define CDR_RTSP_PUSH_H

void InitRtspPush(char *url);
//int Push_RtpSendStream(VENC_STREAM_S *pstStream);
int Push_RtpSendFrame(unsigned char *pFrameBuffer,unsigned int uiFrameLens,unsigned long long pts);

#endif


