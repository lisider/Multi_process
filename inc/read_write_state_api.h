#ifndef __READ_WRITE_STATE_API_H_
#define __READ_WRITE_STATE_API_H_
void    disable_writeable(char *value);
void    enable_writeable(char * value);
void    init_status(char *value);
int    is_writeable(char value);
// int    is_writeable_recv(char value);
// void    enable_writeable_recv(char *value);
// void disable_writeable_recv(char *value);
#endif
