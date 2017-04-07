
#ifndef _CDR_KEY_CHECK_H_
#define _CDR_KEY_CHECK_H_

typedef int (*cdr_key_check_callback)(unsigned int uiRegValue);

int cdr_key_check_init();
void cdr_key_check_setevent_callbak(cdr_key_check_callback callback);

#endif



