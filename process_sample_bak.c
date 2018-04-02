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
#include <errno.h>
#include <strings.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include "print_color.h"

#define SHM_PATH "/tmp/ws_client_shm"
#define ARRAY_SIZE(arr) (sizeof(arr)/sizeof((arr)[0]))

typedef enum {
    BLUETOOTH,
} process_type_t;

typedef void (*call_back_fun_t)(int count , char state);

typedef struct{
    call_back_fun_t callback_send; //websocket 进程特有的
    call_back_fun_t callback_ack; // 每个进程都有的,表示一次,通话结束
    call_back_fun_t callback_passive; //websocket 特有的
}msg_del_method_t;

typedef struct {
    pid_t pid; 
    process_type_t process_type;
    msg_del_method_t msg_del_method;
}process_msg_t;


typedef struct {
    char context[64*1024]; //内容
    char msg[32];
    int count;//表示第几次
    int pid_to; // 表示接收数据的进程
    int pid_from; //表示发送数据的进程

}data_t;

//注册的与 每个进程通信的回调函数

struct shm
{
    sem_t sem; 
    //进程属性
    process_msg_t process_register[64];
    //进程发送的数据
    data_t data; //目前的状态.

    //对进程发送数据的同步
    volatile char read_write_state; //对 发送buf 的 状态管理            
    volatile char unwriteable_times_send; // 想写 buff_to_send 的次数            
    volatile char unwriteable_times_recv; // 想写 buff_to_recv 的次数  //目前buff_to_recv 已经做到 管道里面了,但仍然想用 unwriteable_times_recv 来维护,但是还未做
};

struct shm *shms;
int shmid;


int shm_init(void){

    key_t key;//key定义
    int fisttime = 0;

    key = ftok(SHM_PATH,'r');//获取key
    if(-1 == key){
        perror("ftok");
        return -1; 
    }   
    shmid = shmget(key,sizeof(struct shm),IPC_CREAT|IPC_EXCL|0666);//共享内存的获取
    if(-1 == shmid){
        if(errno == EEXIST){
            fisttime = 1;
            shmid = shmget(key,sizeof(struct shm),0);
        }else{
            perror("shmget");
            return -1; 
        }   
    }   

    shms = shmat(shmid,NULL,0);//共享内存的映射
    if(-1 == *(int *)shms){
        perror("shmat");
        return -1; 
    }

    //如果是第一次创建,则初始化,并映射,如果不是,则只做映射
    if(fisttime){
        bzero(shms,sizeof(struct shm)); //清 0 共享内存
        init_status(&(shms->read_write_state));
        shms->unwriteable_times_send=0;
        sem_init((sem_t *)&(shms->sem), 0, 1); 
    }
    /** else{ */
    /**     sem_wait((sem_t *)&(shms->sem)); */
    /**  */
    /**     sem_post((sem_t *)&(shms->sem)); */
    /** } */
    return 0;
}



static void sig_handler(int arg){

    //char buf[40];
    char buf[32];	// 用来接收fifo 的 信息
    char buf_verify[32]; //用来解密消息,即将用count 替代
    //msg_type_t state;
    //int found = 0;
    //int i;
    int tmp_shmid =0;
    long int nattch = 0;
    int maxid,id;
    bzero(buf,sizeof(buf));
    struct shmid_ds shmseg;
    struct shm_info shm_info;

    switch(arg){

        case SIGINT: //退出信号

            printf("going to exit\n");

            // 3. 取消共享内存的映射
            // 判断自己是不是最后一个进程,如果是,则删除共享内存

            maxid = shmctl(0, SHM_INFO, (struct shmid_ds *) (void *) &shm_info);
            for (id = maxid; id >= 0; id--) { 
                tmp_shmid = shmctl(id, SHM_STAT, &shmseg);
                if(tmp_shmid == shmid)
                {
                    nattch = (long) shmseg.shm_nattch;
                    //printf("%d,%ld\t",tmp_shmid,(long) shmseg.shm_nattch);
                    break;
                }
            }

            if(nattch == 1)
            {
                shmdt(shms);
                shmctl(shmid, IPC_RMID, NULL);
            }else{
                shmdt(shms);
            }
            exit(0);
            break;
        default:
            ;
    }
    return ;
}


#if 0
int pkt_send(msg_send_t * send_pkt_p,int size,int count){

    int ret = 0;
    int i = 0;

    //处理同步问题

    //同步 1 信号量
    while(sem_trywait((sem_t *)&(shms->sem)) == -1){
        perror("sem_trywait");
        if(++i >10)
            return -1;
        usleep(1000);
    }

    //同步 2 读写标识
    while(!is_writeable_send(shms->read_write_state)){
        shms->unwriteable_times_send += 1;

        if(++shms->unwriteable_times_send > 5)
        {
            shms->unwriteable_times_send = 0;
            sem_post((sem_t *)&(shms->sem));
            return -2;
        }
        usleep(1000);
    }

    //处理数据
    printf(YELLOW "%s send %d data to ws_client\n" NONE,processtype(send_pkt_p->msg_info.process_type),count); 
    memcpy((char *)&(shms->buff_to_send),send_pkt_p,sizeof(msg_send_t));

    //处理同步问题
    disable_writeable_send(&(shms->read_write_state));
    shms->unwriteable_times_send = 0;
    sem_post((sem_t *)&(shms->sem));

    return  0;
}
#endif



int traverse_process(void){
    int i = 0;
    int j = 0;

    while(sem_trywait((sem_t *)&(shms->sem)) == -1){                            
        perror("sem_trywait");                                                  
        if(++i >10)                                                             
            return -1;                                                          
        usleep(1000);                                                           
    }

    //sem_wait((sem_t *)&(shms->sem));
    for(i=0;i<ARRAY_SIZE(shms->process_register);i++){
        if(shms->process_register[i].pid != 0){
            i++;
        }
    }

    printf(GREEN"%d process exist,");
    for (j=0;j<i;j++){
        printf("\t%d",shms->process_register[i].pid);
    }
    printf("\n"NONE);


    sem_post((sem_t *)&(shms->sem));
    return 0;

}


void register_process(process_msg_t *p){
    int i=0;
    //sem_wait((sem_t *)&(shms->sem));

    while(sem_trywait((sem_t *)&(shms->sem)) == -1){                            
        perror("sem_trywait");                                                  
        if(++i >10)                                                             
            return -1;                                                          
        usleep(1000);                                                           
    }


    printf("sws : %s,%s,line = %d\n",__FILE__,__func__,__LINE__);
    for(i=0;i<ARRAY_SIZE(shms->process_register);i++){
        if(shms->process_register[i].pid == 0){
            printf("sws : %s,%s,line = %d\n",__FILE__,__func__,__LINE__);
            memcpy((char *)&(shms->process_register[i]),p,sizeof(shms->process_register[i]));
            break;
        }
    }

    sem_post((sem_t *)&(shms->sem));
    return ;
}


//--------------------------


void call_back_SEND(int count,char state){                                          

    printf(YELLOW"call_back_SEND  count :%d, state:%d\n"NONE,count,state);          
    return ;                                                                        
}                                                                                   

void call_back_ACK(int count,char state){                                           

    printf(YELLOW"call_back_ACK  count :%d, state:%d\n"NONE,count,state);        
    return ;                                                                     
}      

void call_back_PASSIVE(int count,char state){                                     

    printf(YELLOW"call_back_ACTIVE  count :%d, state:%d\n"NONE,count,state);     
    return ;                                                                     
}    


int main(int argc,char ** argv){
    process_msg_t process_msg;
    shm_init();
    //开一个线程
    signal(SIGINT, sig_handler);

    printf("sws : %s,%s,line = %d\n",__FILE__,__func__,__LINE__);
    //注册
    process_msg.pid = getpid();
    process_msg.process_type  = BLUETOOTH;
    process_msg.msg_del_method.callback_send = call_back_SEND;
    process_msg.msg_del_method.callback_ack = call_back_ACK;
    process_msg.msg_del_method.callback_passive = call_back_PASSIVE;

    printf("sws : %s,%s,line = %d\n",__FILE__,__func__,__LINE__);
    register_process(&process_msg);

    printf("sws : %s,%s,line = %d\n",__FILE__,__func__,__LINE__);

    //看进程是否存在
    traverse_process();
    printf("done\n");



    //    pkt_send(&(tmp_xxx_node->data),sizeof(tmp_xxx_node->data),count);


    while(1);


    return 0;
}
