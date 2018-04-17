#ifndef __FUNCTION_H__
#define __FUNCTION_H__
#include "common.h"
int   findpidbyname(process_type_t process_type);
int    is_existed(process_type_t process_type);
int    pkt_send(data_t * send_pkt_p,int size);
int  register_process(process_msg_t *p);
void  sig_handler(int arg);
int    traverse_process(void);
char *whoami(process_type_t process_type);
int  process_init(char *s);
int timer_init(void);
#endif
