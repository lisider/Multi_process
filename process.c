/*************************************************************************
  > File Name: process_sample.c
  > Author: Sues
  > Mail: sumory.kaka@foxmail.com 
  > Created Time: Fri 30 Mar 2018 10:31:22 AM CST
 ************************************************************************/

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <errno.h>
#include <strings.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include "print_color.h"
#include <string.h>
#include "list.h"
#include "common.h"
#include "shm.h"
#include <pthread.h>
#include "function.h"



process_type_t process_type = -1;

void init(void * arg){//每个进程都有的一个初始化.
    printf(TODO"specific init\n"NONE);
}

void call_back_SEND(int count,char state){                                          

    printf(TODO"call_back_SEND  count :%d, state:%d\n"NONE,count,state);          
    return ;                                                                        
}                                                                                   

void call_back_ACK(int count,char state){                                           

    printf(TODO"call_back_ACK  count :%d, state:%d\n"NONE,count,state);        
    return ;                                                                     
}      

void call_back_PASSIVE(int count,char state){                                     

    printf(TODO"call_back_ACTIVE  count :%d, state:%d\n"NONE,count,state);     
    return ;                                                                     
}    


void todel(list_xxx_t* list_todel_head){//websocket 独有的 发送消息的过程

    printf(TODO"todel fuction\n"NONE);     
    printf("sws : %s,%s,line = %d\n",__FILE__,__func__,__LINE__);
}

void waitfor(data_t * data){//websocket 独有的 接收的过程
    printf(TODO"waitfor fuction\n"NONE);     
}


int  process_init(char *s){

    int ret = -1;

    //注册信号
    signal(SIGINT, sig_handler);
    signal(SIGUSR1, sig_handler);

    //初始化链表
    INIT_LIST_HEAD(&list_tosend_head.list);  
    INIT_LIST_HEAD(&list_todel_head.list);  
    INIT_LIST_HEAD(&list_deled_head.list);  

    //共享内存及信号量的初始化
    ret = shm_init();
    if(ret < 0){
        printf(ERROR"shm_init failed\n"NONE);
        return ret;
    }

    //初始化锁和条件变量
    if(0 != pthread_mutex_init(&mutex,NULL)){
        perror("mutex init");
        return -1;
    }
    if(0 != pthread_cond_init(&cond1,NULL)){
        perror("cond1 init");
        return -1;
    }
    if(0 != pthread_cond_init(&cond2,NULL)){
        perror("cond2 init");
        return -1;
    }

    if(0 != pthread_cond_init(&cond3,NULL)){
        perror("cond2 init");
        return -1;
    }

    //开三个线程
    if(0 != pthread_create(&pthid1,NULL,recv_thread_1,NULL)){
        perror("pthid1");
        return -1;
    }
    if(0 != pthread_create(&pthid2,NULL,recv_thread_2,NULL)){
        perror("pthid1");
        return -1;
    }
    //sleep(1);
    if(0 != pthread_create(&pthid3,NULL,recv_thread_3,NULL)){
        perror("pthid1");
        return -1;
    }

    return 1;
}

int main(int argc,char ** argv){

    int count;
    int i ,j ;
    //要发送的数据
    data_t data;

    //要注册的信息
    process_msg_t process_msg;

    int ret = 0;

    ret = process_init(NULL);
    if(ret < 0){
        printf(ERROR"process_init failed\n"NONE);
        return ret;
    }

    process_type = strtoul(argv[1],NULL, 10);
    printf(INFO"my name is %s\n"NONE,whoami(process_type));

    //填充要注册的信息
    process_msg.pid = getpid();
    process_msg.process_type  = process_type;
    process_msg.msg_del_method.callback_send = call_back_SEND;
    process_msg.msg_del_method.callback_ack = call_back_ACK;
    process_msg.msg_del_method.callback_passive = call_back_PASSIVE;
    process_msg.msg_del_method.init = init;
    process_msg.msg_del_method.todel = NULL;
    process_msg.msg_del_method.waitfor = NULL;

    register_process(&process_msg);


    //遍历存在的进程
    traverse_process();

    //printf("BLUETOOTH is_existed :%d\n",is_existed(BLUETOOTH));

    while(1){

        printf("\n\n\n\n\n\n");
        // 包,发送 ,发送给 AUDIO的包
        bzero(&data,sizeof(data));
        strcpy((char *)&(data.context),"JSON package");
        strcpy((char *)&(data.msg),"Extra string information");
        data.count = ++count;
        data.pid_from = getpid();

        while(findpidbyname(AUDIO) <=0 || findpidbyname(AUDIO) == getpid()){
            if(findpidbyname(AUDIO) <=0)
                printf(WARN"AUDIO is not on line\n"NONE);
            else if(findpidbyname(AUDIO) == getpid()){
                printf(WARN"can't send msg to myself, i am %s,going to block\n"NONE,whoami(process_type));
                while(1);
            }
            sleep(1);
        }

        data.pid_to = findpidbyname(AUDIO);
        printf(INFO"%s is  on-line\n\n\n"NONE,whoami(AUDIO));

        ret = pkt_send(&data,sizeof(data));
        if( ret < 0 ){
            printf("error happed\n");
        }

        for(i = 0;i<30000;i++)
            for(j = 0; j < 10000;j++);
    }

    return 0;
}
