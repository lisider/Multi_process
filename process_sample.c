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



typedef struct {
    char context[64*1024]; //内容
    char msg[32];
    int count;//表示第几次
    int pid_to; // 表示接收数据的进程
    int pid_from; //表示发送数据的进程

}data_t;
typedef struct  _list_xxx_t{
    data_t data;
    struct list_head list;
}list_xxx_t;

typedef enum {
    BLUETOOTH,
} process_type_t;


typedef struct{
    call_back_fun_t callback_send; //websocket 进程特有的
    call_back_fun_t callback_ack; // 每个进程都有的,表示一次,通话结束
    call_back_fun_t callback_passive; //websocket 特有的
    init_t  init;
    void (*todel)(list_xxx_t* list_todel_head);
    void (*waitfor)(void)
}msg_del_method_t;

typedef struct {
    pid_t pid; 
    process_type_t process_type;
    msg_del_method_t msg_del_method;

}process_msg_t;


//注册的与 每个进程通信的回调函数


struct shm
{
    // sem_t sem; 
    int semid;
    //进程属性
    process_msg_t process_register[64];
    //进程发送的数据
    data_t data; //目前的状态.

    //对进程发送数据的同步
    volatile char read_write_state; //对 发送buf 的 状态管理            
    volatile char unwriteable_times_send; // 想写 buff_to_send 的次数            
    volatile char unwriteable_times_recv; // 想写 buff_to_recv 的次数  //目前buff_to_recv 已经做到 管道里面了,但仍然想用 unwriteable_times_recv 来维护,但是还未做
};


union semun {
    int val; /* value for SETVAL */
    struct semid_ds *buf; /* buffer for IPC_STAT, IPC_SET */
    unsigned short *array; /* array for GETALL, SETALL */
    struct seminfo *__buf; /* buffer for IPC_INFO */
};


/**************************************************************************************
 * 全局变量定义区
 **************************************************************************************/

list_xxx_t list_tosend_head;
list_xxx_t list_todel_head;
list_xxx_t list_deled_head;
struct shm *shms;
int shmid;

/**************************************************************************************
 * 函数声明区
 **************************************************************************************/
void todel(list_xxx_t* list_todel_head);
void waitfor(void);

/**************************************************************************************
 * 函数定义区
 **************************************************************************************/

/***对信号量数组semnum编号的信号量做P操作***/
int sem_P(int semid, int semnum)
{
    struct sembuf sops={semnum,-1, SEM_UNDO};
    return (semop(semid,&sops,1));
}
/***对信号量数组semnum编号的信号量做V操作***/
int sem_V(int semid, int semnum)
{
    struct sembuf sops={semnum,+1, SEM_UNDO};
    return (semop(semid,&sops,1));
}



int shm_init(void){

    key_t key;//key定义
    key_t key2;//key定义
    int fisttime = 0;
    int ret = 0;

    int access(const char *filename, int mode);                                  
    if(access(SHM_PATH,F_OK) != 0)                                                   
    {                                                                                
        printf("mkdir SHM_PATH\n");                                                  
        system("mkdir " SHM_PATH);                                                   
    }   
    key = ftok(SHM_PATH,'r');//获取key
    if(-1 == key){
        perror("ftok");
        return -1; 
    }   

    shmid = shmget(key,sizeof(struct shm),IPC_CREAT|IPC_EXCL|0666);//共享内存的获取
    if(-1 == shmid){
        if(errno == EEXIST){
            shmid = shmget(key,sizeof(struct shm),0);
        }else{
            perror("shmget");
            return -1; 
        }   
    } else
        fisttime = 1;

    shms = shmat(shmid,NULL,0);//共享内存的映射
    if(-1 == *(int *)shms){
        perror("shmat");
        return -1; 
    }

    //如果是第一次创建,则初始化,并映射,如果不是,则只做映射
    if(fisttime){

        printf("first time\n");
        bzero(shms,sizeof(struct shm)); //清 0 共享内存
        init_status(&(shms->read_write_state));
        shms->unwriteable_times_send=0;
        //sem_init((sem_t *)&(shms->sem), 0, 1); 
        union semun arg;

        key2 = ftok("/tmp", 0x66 ) ;
        if ( key2 < 0 )
        {
            perror("ftok key2 error") ;
            return -1 ;
        }
        /***本程序创建了一个信号量**/
        shms->semid = semget(key2,1,IPC_CREAT|0600);
        if (shms->semid == -1)
        {
            perror("create semget error");
            return -1;
        }
        arg.val = 1;
        /***对0号信号量设置初始值***/
        ret =semctl(shms->semid,0,SETVAL,arg);
        if (ret < 0 )
        {
            perror("ctl sem error");
            semctl(shms->semid,0,IPC_RMID,arg);
            return -1 ;
        }
    }
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

        case SIGUSR1: 
            break;
        default:
            ;
    }
    return ;
}


int pkt_send(data_t * send_pkt_p,int size){

    printf("sws : %s,%s,line = %d\n",__FILE__,__func__,__LINE__);
    int ret = 0;
    int i = 0;
    list_xxx_t *tmp_tosend_node;

    //处理同步问题
    //同步 1 信号量
    int flag = 0;
    flag = sem_P(shms->semid,0)  ;
    if ( flag )
    {
        perror("P operate error") ;
        return -1 ;
    }

    //同步 2 读写标识
    while(!is_writeable_send(shms->read_write_state)){
        shms->unwriteable_times_send += 1;

        if(++shms->unwriteable_times_send > 5)
        {
            shms->unwriteable_times_send = 0;

            if (sem_V(shms->semid, 0) < 0)
            {
                perror("V operate error") ;
                return -1 ;
            }
            printf("sws : %s,%s,line = %d\n",__FILE__,__func__,__LINE__);
            return -2;
        }
        usleep(1000);
    }

    printf("sws : %s,%s,line = %d\n",__FILE__,__func__,__LINE__);
    tmp_tosend_node = (list_xxx_t *)malloc(sizeof(list_xxx_t));
    bzero((void *)&(tmp_tosend_node->data),sizeof(data_t));                       
    memcpy((char *)&(tmp_tosend_node->data),send_pkt_p,size);

    list_add_tail(&(tmp_tosend_node->list),&list_tosend_head.list);

    printf("sws : %s,%s,line = %d\n",__FILE__,__func__,__LINE__);

    list_for_each_entry(tmp_tosend_node,&list_tosend_head.list,list){
        
        printf("traversal2 num :\t%s\tname :\t%s\n",tmp_tosend_node->data.context,tmp_tosend_node->data.msg);

    }

    //处理数据
    //printf(YELLOW "%s send %d data to ws_client\n" NONE,processtype(send_pkt_p->msg_info.process_type),count); 
    memcpy((char *)&(shms->data),send_pkt_p,size);
    //kill(shms->data.pid_to,SIGUSR1);

    //处理同步问题
    disable_writeable_send(&(shms->read_write_state));
    shms->unwriteable_times_send = 0;
    if (sem_V(shms->semid, 0) < 0)
    {
        perror("V operate error") ;
        return -1 ;
    }

    return  0;
}


int is_existed(process_type_t process_type){
    int i = 0;
    int j = 0;

    int flag = 0;


    flag = sem_P(shms->semid,0)  ;
    if ( flag )
    {
        perror("P operate error") ;
        return -1 ;
    }

    for(i=0;i<ARRAY_SIZE(shms->process_register);i++){
        if(shms->process_register[i].process_type == process_type)
            break;
    }

    if (i == ARRAY_SIZE(shms->process_register))
    {
        if (sem_V(shms->semid, 0) < 0)
        {
            perror("V operate error") ;
            return -1 ;
        }

        return 0;

    }
    else
    {
        if (sem_V(shms->semid, 0) < 0)
        {
            perror("V operate error") ;
            return -1 ;
        }

        return 1;

    }
}


int traverse_process(void){
    int i = 0;
    int j = 0;

    int flag = 0;


    flag = sem_P(shms->semid,0)  ;
    if ( flag )
    {
        perror("P operate error") ;
        return -1 ;
    }
    //sem_wait((sem_t *)&(shms->sem));
    for(i=0;i<ARRAY_SIZE(shms->process_register);i++){
        if(shms->process_register[i].pid != 0){
            j++;
        }
    }

    printf(GREEN"%d process exist,",j);
    for (i=0;i<j;i++){
        printf("\t%d",shms->process_register[i].pid);
    }
    printf("\n"NONE);


    if (sem_V(shms->semid, 0) < 0)
    {
        perror("V operate error") ;
        return -1 ;
    }
    return 0;
}



void register_process(process_msg_t *p){
    int i=0;
    int flag = 0;


    flag = sem_P(shms->semid,0)  ;
    if ( flag )
    {
        perror("P operate error") ;
        return ;
    }

    printf("sws : %s,%s,line = %d\n",__FILE__,__func__,__LINE__);
    for(i=0;i<ARRAY_SIZE(shms->process_register);i++){
        if(shms->process_register[i].pid == 0){
            printf("sws : %s,%s,line = %d\n",__FILE__,__func__,__LINE__);
            memcpy((char *)&(shms->process_register[i]),p,sizeof(shms->process_register[i]));
            break;
        }
    }

    //这里面加一个注册函数的初始化
    shms->process_register[i].msg_del_method.init(NULL);

    if (sem_V(shms->semid, 0) < 0)
    {
        perror("V operate error") ;
        return ;
    }

    return ;
}





void * recv_thread_1(void *arg){
    

    return NULL;
}

void * recv_thread_2(void *arg){
    
    todel(&list_todel_head);

    return NULL;
}
void * recv_thread_3(void *arg){
    
    waitfor();

    return NULL;
}
/**************************************************************************************
 * 全局变量定义区
 **************************************************************************************/

void init(void * arg){
    printf("my init\n");
}

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


int findpidbyname(process_type_t process_type){

    return 1;
}




void todel(list_xxx_t* list_todel_head){

}
void waitfor(void){

}

int main(int argc,char ** argv){

    //通用初始化
    pthread_t       pthid1;
    pthread_t       pthid2;
    pthread_t       pthid3;
    process_msg_t process_msg;
    data_t data;
    list_xxx_t *tmp_tosend_node;
    int ret = 0;
    INIT_LIST_HEAD(&list_tosend_head.list);  
    shm_init();

    //通用开线程
    if(0 != pthread_create(&pthid1,NULL,recv_thread_1,NULL)){                            
        perror("pthid1");                                                   
        return -1;                                                          
    }     

    if(0 != pthread_create(&pthid2,NULL,recv_thread_2,NULL)){                            
        perror("pthid1");                                                   
        return -1;                                                          
    }     

    if(0 != pthread_create(&pthid3,NULL,recv_thread_3,NULL)){                            
        perror("pthid1");                                                   
        return -1;                                                          
    }     
    signal(SIGINT, sig_handler);

    printf("sws : %s,%s,line = %d\n",__FILE__,__func__,__LINE__);

    //注册 并对注册函数初始化
    process_msg.pid = getpid();
    process_msg.process_type  = BLUETOOTH;
    process_msg.msg_del_method.callback_send = call_back_SEND;
    process_msg.msg_del_method.callback_ack = call_back_ACK;
    process_msg.msg_del_method.callback_passive = call_back_PASSIVE;
    process_msg.msg_del_method.init = init;
    process_msg.msg_del_method.todel = todel;
    process_msg.msg_del_method.waitfor = waitfor;

    printf("sws : %s,%s,line = %d\n",__FILE__,__func__,__LINE__);
    register_process(&process_msg);

    printf("sws : %s,%s,line = %d\n",__FILE__,__func__,__LINE__);

    //看进程是否存在
    traverse_process();

    printf("%d\n",is_existed(BLUETOOTH));


    // 包,发送
    bzero(&data,sizeof(data));
    strcpy((char *)&(data.context),"111111111411111111111111111");
    strcpy((char *)&(data.msg),"222");
    data.count = 1;
    data.pid_to = findpidbyname(BLUETOOTH);
    data.pid_from = getpid();

    printf("sws : %s,%s,line = %d\n",__FILE__,__func__,__LINE__);
    ret = pkt_send(&data,sizeof(data));
    if( ret < 0 ){
        printf("error happed\n");
    }

    printf("sws : %s,%s,line = %d\n",__FILE__,__func__,__LINE__);
    list_for_each_entry(tmp_tosend_node,&list_tosend_head.list,list){

        printf("traversal2 num :\t%s\tname :\t%s\n",tmp_tosend_node->data.context,tmp_tosend_node->data.msg);

    }

    while(1);

    return 0;
}
