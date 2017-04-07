#ifndef CDR_FTP_UPLOAD_H
#define CDR_FTP_UPLOAD_H


//int cdr_ftp_upload(char* url, char *FullFileName,sBBMsgHeaderPack sBBMsgHeadPack,s_BuBiaoTatoilResult *sTatoiResultPackage);
//int cdr_ftp_upload(CURL *curlhandle, const char * remotepath, const char * localpath, long timeout, long tries);
int cdr_stop_ftp_upload();
int cdr_stop_ftp_upload_ex_ucChnID(unsigned short usMsgID,unsigned char ucChnID);
int cdr_stop_ftp_upload_ex_MsgSerialNm(unsigned short usMsgID,unsigned short usMsgSerialNm);

#endif


