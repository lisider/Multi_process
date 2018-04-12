/*************************************************************************
  > File Name: common.h
  > Author: Sues
  > Mail: sumory.kaka@foxmail.com 
  > Created Time: Mon 02 Apr 2018 02:54:49 PM CST
 ************************************************************************/


#ifndef __COMMON_H__
#define __COMMON_H__


#include "list.h"
#include <sys/types.h>
#include <unistd.h>

/**************************************************************************************
 * 宏定义区
 **************************************************************************************/

#define SHM_PATH "/tmp/ws_client_shm"
#define ARRAY_SIZE(arr) (sizeof(arr)/sizeof((arr)[0]))

/**************************************************************************************
 * 函数类型声明区
 **************************************************************************************/

typedef void (*call_back_fun_t)(int count , char state);
typedef void (*init_t)(void * arg);


/**************************************************************************************
 * 结构体/枚举体声明区
 **************************************************************************************/

typedef enum{
    SEND_NORMAL,
    SEND_WEBSOCKET,
    RECV_1,
    RECV_2,
} data_state_t;

typedef struct {
    char context[64*1024]; //内容
    char msg[32];
    int count;//表示第几次
    int pid_to; // 表示接收数据的进程
    int pid_from; //表示发送数据的进程
    data_state_t data_state;
    int deadline;
    char sha1[40];
}data_t;

typedef struct  _list_xxx_t{
    data_t data;
    struct list_head list;
}list_xxx_t;

typedef enum {
    NONE_PROCESS,
    BLUETOOTH,
    AUDIO,
    WEBSOCKET,
    STATE,
    STATE2,
} process_type_t;



typedef struct{
    call_back_fun_t callback_send; //针对websocket 进程特有的
    call_back_fun_t callback_ack; // 每个进程都有的,表示一次,通话结束
    call_back_fun_t callback_passive; //针对websocket 特有的
    init_t  init;
    void (*todel)(list_xxx_t* list_todel_head);
    void (*waitfor)(data_t *data);
}msg_del_method_t;

typedef struct {
    pid_t pid; 
    process_type_t process_type;
    msg_del_method_t msg_del_method;
    int semid;
}process_msg_t;


//注册的与 每个进程通信的回调函数


struct shm
{
    // sem_t sem; 
    int semid;//用来同步进程间的通信
    //进程属性
    process_msg_t process_register[64];
    //进程发送的数据
    data_t data; //目前的状态.

    //对进程发送数据的同步
    volatile char read_write_state; //对 发送buf 的 状态管理            
    volatile char unwriteable_times_send; // 想写 buff_to_send 的次数            
    //volatile char unwriteable_times_recv; // 想写 buff_to_recv 的次数  //目前buff_to_recv 已经做到 管道里面了,但仍然想用 unwriteable_times_recv 来维护,但是还未做
};


union semun {
    int val; /* value for SETVAL */
    struct semid_ds *buf; /* buffer for IPC_STAT, IPC_SET */
    unsigned short *array; /* array for GETALL, SETALL */
    struct seminfo *__buf; /* buffer for IPC_INFO */
};


extern process_type_t process_type;
extern list_xxx_t list_tosend_head;
extern list_xxx_t list_todel_head;
extern list_xxx_t list_deled_head;

extern void * recv_thread_1(void *arg);
extern void * recv_thread_2(void *arg);
extern void * recv_thread_3(void *arg);
extern void sig_handler(int arg);


list_xxx_t list_tosend_head;
list_xxx_t list_todel_head;
list_xxx_t list_deled_head;
struct shm *shms;
int shmid;

pthread_cond_t  cond1,cond2,cond3;
pthread_mutex_t mutex;
pthread_t       pthid1;
pthread_t       pthid2;
pthread_t       pthid3;

#define VIEWLIST \
    do{\
        i = 0;j = 0; k = 0;\
        list_for_each_entry(tmp_xxx_node,&list_tosend_head.list,list)\
        i++;\
        list_for_each_entry(tmp_xxx_node,&list_todel_head.list,list)\
        j++;\
        list_for_each_entry(tmp_xxx_node,&list_deled_head.list,list)\
        k++;\
        printf(GREEN "%s,%s,line = %d,list_tosend_head : %d,list_todel_head : %d,list_deled_head : %d\n" NONE,__FILE__,__func__,__LINE__,i,j,k);\
    }while(0)

#endif
