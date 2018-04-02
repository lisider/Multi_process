/*************************************************************************
  > File Name: other2.c
  > Author: Sues
  > Mail: sumory.kaka@foxmail.com 
  > Created Time: Mon 02 Apr 2018 03:07:13 PM CST
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


#define VIEWLIST \
    do{printf("-----------------------------------\n");\
    i = 0;j = 0; k = 0;\
    list_for_each_entry(tmp_xxx_node,&list_tosend_head.list,list)\
    i++;\
    list_for_each_entry(tmp_xxx_node,&list_todel_head.list,list)\
    j++;\
    list_for_each_entry(tmp_xxx_node,&list_deled_head.list,list)\
    k++;\
        printf("sws : %s,%s,line = %d\n",__FILE__,__func__,__LINE__);\
        printf(GREEN "list_tosend_head : %d,list_todel_head : %d,list_deled_head : %d\n" NONE,i,j,k);\
    printf("-----------------------------------\n");}while(0)



void * recv_thread_1(void *arg){

    struct list_head *pos,*pos2,*n;
    list_xxx_t *tmp_xxx_node1;
    list_xxx_t *tmp_xxx_node;
    list_xxx_t *tmp_xxx_node2;
    int is_ack = 0;
    int i,j , k;
    while(1){
        pthread_mutex_lock(&mutex);
        pthread_cond_wait(&cond1,&mutex);


        int flag = 0;
        flag = sem_P(shms->semid,0)  ;
        if ( flag )
        {
            perror("P operate error") ;
            return -1 ;
        }

        tmp_xxx_node1 = (list_xxx_t *)malloc(sizeof(list_xxx_t));
        bzero((void *)&(tmp_xxx_node1->data),sizeof(data_t));                       
        memcpy((char *)&(tmp_xxx_node1->data),(char *)&(shms->data),sizeof(data_t));

        enable_writeable_send(&(shms->read_write_state));

        list_for_each_safe(pos,n,&list_tosend_head.list){  		
            tmp_xxx_node2 = list_entry(pos,list_xxx_t,list);//得到外层的数据
            if(tmp_xxx_node1->data.count == tmp_xxx_node2->data.count){//对链表中的数据进行判断,如果满足条件就删节点
                VIEWLIST;
                printf(ACTION"is ack ,deleted Corresponding message in list_tosend_head\n"NONE);
                list_del(pos); // 注意,删除链表,是删除的list_head,还需要删除 外层的数据 ,删除一个节点之后,并没有破坏这个节点和外围数据的位置关系
                free(tmp_xxx_node2);//释放数据
                VIEWLIST;
                is_ack = 1;
                break;
            }
        }



        if(!is_ack){
            VIEWLIST;
            list_add_tail(&(tmp_xxx_node1->list),&list_todel_head.list);
            VIEWLIST;
            printf(ACTION"recv_thread_1 recv data.make recv_thread_2 run\n"NONE);
            pthread_cond_signal(&cond2);
        }

        if (sem_V(shms->semid, 0) < 0)
        {
            perror("V operate error") ;
            return -1 ;
        }

        pthread_mutex_unlock(&mutex);
    }

    return NULL;
}

void * recv_thread_2(void *arg){

    struct list_head *pos,*pos2,*n;
    list_xxx_t *tmp_xxx_node1;
    list_xxx_t *tmp_xxx_node;
    list_xxx_t *tmp_xxx_node2;
    int flag = 0;
    int i , j , k;

    while(1){
        pthread_mutex_lock(&mutex);
        pthread_cond_wait(&cond2,&mutex);


        list_for_each_safe(pos,n,&list_todel_head.list){  		
            tmp_xxx_node2 = list_entry(pos,list_xxx_t,list);//得到外层的数据


            flag = sem_P(shms->semid,0);
            if ( flag )
            {
                perror("P operate error") ;
                return -1 ;
            }

            if(shms->process_register[process_type].msg_del_method.todel != NULL)
                shms->process_register[process_type].msg_del_method.todel(&list_todel_head);

            if (sem_V(shms->semid, 0) < 0)
            {
                perror("V operate error") ;
                return -1 ;
            }

            VIEWLIST;
            list_del(pos); // 注意,删除链表,是删除的list_head,还需要删除 外层的数据 ,删除一个节点之后,并没有破坏这个节点和外围数据的位置关系
            list_add_tail(&(tmp_xxx_node2->list),&list_deled_head.list);
            VIEWLIST;

            pthread_cond_signal(&cond3);
        }



        pthread_mutex_unlock(&mutex);
        printf(GREEN"recv_thread_2 done\n"NONE);
    }
    return NULL;
}
void * recv_thread_3(void *arg){

    struct list_head *pos,*pos2,*n;
    list_xxx_t *tmp_xxx_node;
    list_xxx_t *tmp_xxx_node1;
    list_xxx_t *tmp_xxx_node2;
    data_t data;
    int flag = 0;
    int ret = 0;
    void (*waitfor)(data_t *data);
    int i = 0 , j  = 0 ,  k = 0;
    pid_t pid_tmp = -1;

    flag = sem_P(shms->semid,0)  ;
    if ( flag )
    {
        perror("P operate error") ;
        return -1 ;
    }
    waitfor = shms->process_register[process_type].msg_del_method.waitfor;
    if (sem_V(shms->semid, 0) < 0)
    {
        perror("V operate error") ;
        return -1 ;
    }


    if(waitfor != NULL){ //普通进程

        while(1){
            waitfor(&data);

            ret = pkt_send(&data,sizeof(data));
            if( ret < 0 ){
                printf("error happed\n");
            }

            list_for_each_safe(pos,n,&list_deled_head.list){  		
                tmp_xxx_node = list_entry(pos,list_xxx_t,list);//得到外层的数据
                if(1){//对链表中的数据进行判断,如果满足条件就删节点
                    VIEWLIST;
                    list_del(pos); // 注意,删除链表,是删除的list_head,还需要删除 外层的数据 ,删除一个节点之后,并没有破坏这个节点和外围数据的位置关系
                    free(tmp_xxx_node);//释放数据
                    VIEWLIST;
                }
            }
        }
    }else
        while(1){
            pthread_mutex_lock(&mutex);
            pthread_cond_wait(&cond3,&mutex);


            list_for_each_safe(pos,n,&list_deled_head.list){  		
                tmp_xxx_node1 = list_entry(pos,list_xxx_t,list);//得到外层的数据
                if(1){//对链表中的数据进行判断,如果满足条件就删节点
                    //printf("one delte in list_deled_head \n");
                    VIEWLIST;
                    list_del(pos); // 注意,删除链表,是删除的list_head,还需要删除 外层的数据 ,删除一个节点之后,并没有破坏这个节点和外围数据的位置关系
                    VIEWLIST;



                    printf("sws : %s,%s,line = %d\n",__FILE__,__func__,__LINE__);

                    flag = sem_P(shms->semid,0)  ;
                    if ( flag )
                    {
                        perror("P operate error") ;
                        return -1 ;
                    }

                    //同步 2 读写标识
                    while(!is_writeable_send(shms->read_write_state)){


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
                            //return -2;
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


                    printf("sws : %s,%s,line = %d\n",__FILE__,__func__,__LINE__);

                    //重写数据 
                    pid_tmp = tmp_xxx_node1->data.pid_to;
                    tmp_xxx_node1->data.pid_to = tmp_xxx_node1->data.pid_from;
                    tmp_xxx_node1->data.pid_from =  pid_tmp;


                    printf("sws : %s,%s,line = %d\n",__FILE__,__func__,__LINE__);
                    memcpy((char *)&(shms->data),&(tmp_xxx_node1->data),sizeof(data_t));
                    //kill(shms->data.pid_to,SIGUSR1);

                    kill(tmp_xxx_node1->data.pid_to,SIGUSR1);
                    printf(YELLOW"send signal to %d\n" NONE,tmp_xxx_node1->data.pid_to);

                    printf("sws : %s,%s,line = %d\n",__FILE__,__func__,__LINE__);

                    //处理同步问题
                    disable_writeable_send(&(shms->read_write_state));
                    shms->unwriteable_times_send = 0;
                    if (sem_V(shms->semid, 0) < 0)
                    {
                        perror("V operate error") ;
                        return -1 ;
                    }

                    printf("sws : %s,%s,line = %d\n",__FILE__,__func__,__LINE__);

                    free(tmp_xxx_node1);//释放数据
                }
            }
            pthread_mutex_unlock(&mutex);
        }

    return NULL;
}
