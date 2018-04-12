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
#include "mysem.h"


char process_names[64][32]={
    "bluetooth",
    "audio",
    "websocket",
    "state",
    "state2",
    "unkown"
};


void todel(list_xxx_t* list_todel_head);
void waitfor(data_t *data);



char * whoami(process_type_t process_type){
    if(process_type <= 0 || process_type > 64) 
        return NULL;
    return process_names[process_type - 1];
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
    list_xxx_t *tmp_xxx_node;
    struct list_head *pos,*pos2,*n;
    int ret = 0;
    int semid_interprocess = 0;

    switch(arg){

        case SIGINT: //退出信号


            //-1. 杀死线程

            //0.注销 进程内信号量
            ret = semctl(semid,0,IPC_RMID);
            if (ret == -1){
                perror("semctl IPC_RMID");
                printf(RED"semctl failed\n"NONE);
            }
            printf(YELLOW"\ndestory Semaphore Within the process\n"NONE);

            SEM_P(shms->semid,SHM_RES);
            semid_interprocess = shms->semid;

            //1.注销进程信息
            for(i=0;i<ARRAY_SIZE(shms->process_register);i++){
                if(shms->process_register[i].process_type == process_type){
                    bzero(&(shms->process_register[i]),sizeof(shms->process_register[i]));
                    printf(YELLOW"Destroy registration information\n"NONE);

                }
            }

            SEM_V(shms->semid,SHM_RES);



            // 3.取消共享内存的映射,判断自己是不是最后一个进程,如果是,则删除共享内存

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

            shmdt(shms);
            printf(YELLOW"Canceling mappings to the shm memory\n"NONE);

            if(nattch == 1)
            {
                //2.销毁sem
                ret = semctl(semid_interprocess,0,IPC_RMID);
                if (ret == -1){
                    perror("semctl IPC_RMID");
                    printf(RED"semctl failed\n"NONE);
                }
                printf(YELLOW"destory Semaphore Interprocess becaseof i am the last process\n"NONE);

                shmctl(shmid, IPC_RMID, NULL);
                printf(YELLOW"Destroy shared memory becaseof i am the last process\n"NONE);
            }


            //反注册
            //链表清空

            //此时信号量已经被销毁,所以不用互斥了

            list_for_each_safe(pos,n,&list_tosend_head.list){  		
                tmp_xxx_node = list_entry(pos,list_xxx_t,list);//得到外层的数据
                list_del(pos); // 注意,删除链表,是删除的list_head,还需要删除 外层的数据 ,删除一个节点之后,并没有破坏这个节点和外围数据的位置关系
                free(tmp_xxx_node);//释放数据
            }

            list_for_each_safe(pos,n,&list_todel_head.list){  		
                tmp_xxx_node = list_entry(pos,list_xxx_t,list);//得到外层的数据
                list_del(pos); // 注意,删除链表,是删除的list_head,还需要删除 外层的数据 ,删除一个节点之后,并没有破坏这个节点和外围数据的位置关系
                free(tmp_xxx_node);//释放数据
            }

            list_for_each_safe(pos,n,&list_deled_head.list){  		
                tmp_xxx_node = list_entry(pos,list_xxx_t,list);//得到外层的数据
                list_del(pos); // 注意,删除链表,是删除的list_head,还需要删除 外层的数据 ,删除一个节点之后,并没有破坏这个节点和外围数据的位置关系
                free(tmp_xxx_node);//释放数据
            }

            printf(YELLOW"Empty chain list\n"NONE);


            exit(0);

            break;

        case SIGUSR1: 
            printf(REVERSE "a msg receive\n"NONE);
            pthread_cond_signal(&cond1);
            break;

        case SIGALRM: //定时信号
            //定时删链表
            printf(GREEN "i am going to Traversing to_send list\n" NONE);

            if (list_empty(&list_tosend_head.list)){
                alarm(1);
                break;
            }

            SEM_P(semid,LIST_TO_SEND);
            list_for_each_safe(pos,n,&list_tosend_head.list){
                tmp_xxx_node = list_entry(pos,list_xxx_t,list);//得到外层的数据
                printf(GREEN "del with one subtraction ,count : %d, left dead_line :%d\n" NONE,tmp_xxx_node->data.count,tmp_xxx_node->data.deadline-1);
                if(--tmp_xxx_node->data.deadline == 0){//对链表中的数据进行判断,如果满足条件就删节点
                    list_del(pos); // 注意,删除链表,是删除的list_head,还需要删除 外层的数据 ,删除一个节点之后,并没有破坏这个节点和外围数据的位置关系
                    free(tmp_xxx_node);//释放数据
                    printf(YELLOW"remove node from list_xxx_head because of dead_line, count is %d\n"NONE,tmp_xxx_node->data.count);
                }
            }
            SEM_V(semid,LIST_TO_SEND);

            //循环链表,并删除剩余=0 的
            alarm(1);
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


    //0.修改 send_pkt_p

    send_pkt_p->deadline = 6;


    //处理同步问题
    //同步 1 信号量
    SEM_P_INT(shms->semid,SHM_RES);

    //同步 2 读写标识
    while(!is_writeable(shms->read_write_state)){
        printf(WARN"sws : %s,%s,line = %d,unwriteable_times_send : %d\n"NONE,__FILE__,__func__,__LINE__,shms->unwriteable_times_send);

        if(++shms->unwriteable_times_send > 5)
        {
            shms->unwriteable_times_send = 0;

            SEM_V_INT(shms->semid,SHM_RES);
            printf(RED"unwriteable_times_send = 5 .force to 0.sws : %s,%s,line = %d\n",__FILE__,__func__,__LINE__);
            break;
        }

        if (sem_V(shms->semid, 0) < 0)
        {
            perror("V operate error") ;
            return -1 ;
        }
        usleep(1000);
        SEM_P_INT(shms->semid,SHM_RES);

    }


    tmp_tosend_node = (list_xxx_t *)malloc(sizeof(list_xxx_t));
    bzero((void *)&(tmp_tosend_node->data),sizeof(data_t));                       
    memcpy((char *)&(tmp_tosend_node->data),send_pkt_p,size);

    if(tmp_tosend_node->data.data_state == SEND_NORMAL || tmp_tosend_node->data.data_state == SEND_WEBSOCKET){
        SEM_P_INT(semid,LIST_TO_SEND);
        list_add_tail(&(tmp_tosend_node->list),&list_tosend_head.list);
        SEM_V_INT(semid,LIST_TO_SEND);
    }

    printf(REVERSE" a msg send ,sha1 is %s\n"NONE,tmp_tosend_node->data.sha1);
    printf(ACTION"send msg from %d to %d\n"NONE,tmp_tosend_node->data.pid_from,tmp_tosend_node->data.pid_to);
    printf(INFO"msg is :\t%s\tname :\t%s\n"NONE,tmp_tosend_node->data.context,tmp_tosend_node->data.msg);
    printf("\n\n");

    //处理数据
    //printf(YELLOW "%s send %d data to ws_client\n" NONE,processtype(send_pkt_p->msg_info.process_type),count); 
    memcpy((char *)&(shms->data),send_pkt_p,size);
    //kill(shms->data.pid_to,SIGUSR1);

    kill(shms->data.pid_to,SIGUSR1);

    //处理同步问题
    disable_writeable((char *)&(shms->read_write_state));
    shms->unwriteable_times_send = 0;
    SEM_V_INT(shms->semid,SHM_RES);

    if(tmp_tosend_node->data.data_state == RECV_1 || tmp_tosend_node->data.data_state == RECV_2)
        free(tmp_tosend_node);
    return  0;
}



int is_existed(process_type_t process_type){
    int i = 0;
    int j = 0;

    int flag = 0;


    SEM_P_INT(shms->semid,SHM_RES);

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
        SEM_V_INT(shms->semid,SHM_RES);
        return 1;

    }
}


int traverse_process(void){
    int i = 0;
    int j = 0;



    SEM_P_INT(shms->semid,SHM_RES);
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

    SEM_V_INT(shms->semid,SHM_RES);

    return 0;
}



int register_process(process_msg_t *p){
    int i=0;
    SEM_P_INT(shms->semid,SHM_RES);

    for(i=0;i<ARRAY_SIZE(shms->process_register);i++){
        if (shms->process_register[i].process_type == p->process_type)
        {

            printf("process type has been registed or its type is zero,type is %d,in process_register[%d]\n",shms->process_register[i].process_type,i);

            if (sem_V(shms->semid, 0) < 0)
            {
                perror("V operate error") ;
                return -1;
            }

            return -2;
        }

        if(shms->process_register[i].pid == 0){
            memcpy((char *)&(shms->process_register[i]),p,sizeof(shms->process_register[i]));
            break;
        }
    }

    //这里面加一个注册函数的初始化
    shms->process_register[i].msg_del_method.init(NULL);

    SEM_V_INT(shms->semid,SHM_RES);

    return 0;
}



int findpidbyname(process_type_t process_type){

    int i = 0;
    int pid = -1;

    if (process_type >=ARRAY_SIZE(shms->process_register))
        return -2;

    SEM_P_INT(shms->semid,SHM_RES);


    for(i=0;i<ARRAY_SIZE(shms->process_register);i++){
        if(shms->process_register[i].process_type == process_type && shms->process_register[i].pid != 0){
            //memcpy((char *)&(shms->process_register[i]),p,sizeof(shms->process_register[i]));
            pid = shms->process_register[i].pid;
            break;
        }
    }

    /** if(shms->process_register[process_type].pid != 0) */
    /**     pid = shms->process_register[process_type].pid; */


    SEM_V_INT(shms->semid,SHM_RES);
    return pid;
}

int  process_init(char *s){

    int ret = -1;

    //注册信号
    signal(SIGINT, sig_handler);
    signal(SIGUSR1, sig_handler);
    signal(SIGALRM, sig_handler);

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

    ret = my_sem_init(s,&semid,3);
    if(ret < 0){
        printf(RED"my_sem_init failed 2\n"NONE);
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
