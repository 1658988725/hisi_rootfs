

#ifndef RTSP_CLIENT_H
#define RTSP_CLIENT_H

int SendOpationRequest(const char *cmd_url);
int DoAnnounce2(const char *cURL);
int DoAnnounce1(const char *cURL);
int DoSetUp(const char *cURL);
int DoRecord(const char *cURL);
int DoTearDown(const char *cURL);

#endif

