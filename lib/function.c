/*************************************************************************
    > File Name: other.c
    > Author: Sues
    > Mail: sumory.kaka@foxmail.com 
    > Created Time: Mon 02 Apr 2018 03:05:59 PM CST
 ************************************************************************/

#include <stdio.h>
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
#include "read_write_state_api.h"




void todel(list_xxx_t* list_todel_head);
void waitfor(data_t *data);



char * whoami(process_type_t process_type){
    switch(process_type){
        case BLUETOOTH:
            return "bluetooth";
            break;
        case AUDIO:
            return "audio";
            break;
        default:
            return "unkown";
            break;
    }
}

/***对信号量数组semnum编号的信号量做P操作***/





void sig_handler(int arg){

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
    int i;
            int flag = 0;

    switch(arg){

        case SIGINT: //退出信号

            //printf("going to exit\n");

            //注销信息
            flag = sem_P(shms->semid,0)  ;
            if ( flag )
            {
                perror("P operate error") ;
                return ;
            }
            for(i=0;i<ARRAY_SIZE(shms->process_register);i++){
                if(shms->process_register[i].process_type == process_type)
                    bzero(&(shms->process_register[i]),sizeof(shms->process_register[i]));
            }
            if (sem_V(shms->semid, 0) < 0)
            {
                perror("V operate error") ;
                return ;
            }



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
                printf(YELLOW"del the shm memory\n"NONE);
                shmdt(shms);
                shmctl(shmid, IPC_RMID, NULL);
            }else{
                printf(YELLOW"not del the shm memory\n"NONE);
                shmdt(shms);
            }


            //反注册




            exit(0);

            break;

        case SIGUSR1: 
            printf(REVERSE "a msg receive\n"NONE);
            pthread_cond_signal(&cond1);
            break;
        default:
            ;
    }
    return ;
}


int pkt_send(data_t * send_pkt_p,int size){

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
    while(!is_writeable(shms->read_write_state)){
        printf(WARN"sws : %s,%s,line = %d,unwriteable_times_send : %d\n"NONE,__FILE__,__func__,__LINE__,shms->unwriteable_times_send);

        if(++shms->unwriteable_times_send > 5)
        {
            shms->unwriteable_times_send = 0;

            if (sem_V(shms->semid, 0) < 0)
            {
                perror("V operate error") ;
                return -1 ;
            }
            printf(RED"unwriteable_times_send = 5 .force to 0.sws : %s,%s,line = %d\n",__FILE__,__func__,__LINE__);
            break;
        }

        if (sem_V(shms->semid, 0) < 0)
        {
            perror("V operate error") ;
            return -1 ;
        }
        usleep(1000);
        flag = sem_P(shms->semid,0)  ;
        if ( flag )
        {
            perror("P operate error") ;
            return -1 ;
        }

    }

    tmp_tosend_node = (list_xxx_t *)malloc(sizeof(list_xxx_t));
    bzero((void *)&(tmp_tosend_node->data),sizeof(data_t));                       
    memcpy((char *)&(tmp_tosend_node->data),send_pkt_p,size);

    list_add_tail(&(tmp_tosend_node->list),&list_tosend_head.list);



    printf(REVERSE" a msg send\n"NONE);
    printf(ACTION"send msg from %d to %d\n"NONE,tmp_tosend_node->data.pid_from,tmp_tosend_node->data.pid_to);
    printf(INFO"msg is :\t%s\tname :\t%s\n"NONE,tmp_tosend_node->data.context,tmp_tosend_node->data.msg);
    printf("\n\n");

    //处理数据
    //printf(YELLOW "%s send %d data to ws_client\n" NONE,processtype(send_pkt_p->msg_info.process_type),count); 
    memcpy((char *)&(shms->data),send_pkt_p,size);
    //kill(shms->data.pid_to,SIGUSR1);

    kill(shms->data.pid_to,SIGUSR1);

    //处理同步问题
    disable_writeable(&(shms->read_write_state));
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

    printf(INFO"%d process exist,pid :",j);
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



int register_process(process_msg_t *p){
    int i=0;
    int flag = 0;


    flag = sem_P(shms->semid,0)  ;
    if ( flag )
    {
        perror("P operate error") ;
        return -1;
    }

    for(i=0;i<ARRAY_SIZE(shms->process_register);i++){
        if(shms->process_register[i].pid == 0){
            memcpy((char *)&(shms->process_register[i]),p,sizeof(shms->process_register[i]));
            break;
        }
    }

    //这里面加一个注册函数的初始化
    shms->process_register[i].msg_del_method.init(NULL);

    if (sem_V(shms->semid, 0) < 0)
    {
        perror("V operate error") ;
        return -1;
    }

    return 0;
}



int findpidbyname(process_type_t process_type){

    int i = 0;
    int flag = 0;
    int pid = -1;

    if (process_type >=ARRAY_SIZE(shms->process_register))
        return -2;

    flag = sem_P(shms->semid,0)  ;
    if ( flag )
    {
        perror("P operate error") ;
        return -1 ;
    }

    if(shms->process_register[process_type].pid != 0)
        pid = shms->process_register[process_type].pid;


    if (sem_V(shms->semid, 0) < 0)
    {
        perror("V operate error") ;
        return -1 ;
    }

    return pid;
}

