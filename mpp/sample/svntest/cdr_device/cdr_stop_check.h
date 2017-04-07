
#ifndef _CDR_STOP_APP_H_
#define _CDR_STOP_APP_H_

typedef int (*cdr_stop_check_callback)(unsigned int uiRegValue);
int cdr_stop_check_init(void);
void cdr_stop_check_setevent_callbak(cdr_stop_check_callback callback);

#endif



