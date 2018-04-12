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
#include "function.h"
#include "read_write_state_api.h"
#include <pthread.h>
#include "shm.h"
#include "mysem.h"



// printf("sws : %s,%s,line = %d\n",__FILE__,__func__,__LINE__);


void * recv_thread_1(void *arg){

    struct list_head *pos,*pos2,*n;
    list_xxx_t *tmp_xxx_node1;
    list_xxx_t *tmp_xxx_node;
    list_xxx_t *tmp_xxx_node2;
    int is_exist = 0;
    int i,j , k;
    while(1){
        pthread_mutex_lock(&mutex);
        pthread_cond_wait(&cond1,&mutex);

        SEM_P_NULL(shms->semid,SHM_RES);
        tmp_xxx_node1 = (list_xxx_t *)malloc(sizeof(list_xxx_t));
        bzero((void *)&(tmp_xxx_node1->data),sizeof(data_t));                       
        memcpy((char *)&(tmp_xxx_node1->data),(char *)&(shms->data),sizeof(data_t));
        enable_writeable((char *)&(shms->read_write_state));
        SEM_V_NULL(shms->semid,SHM_RES);

		//2.判断信息类型
		switch(tmp_xxx_node1->data.data_state){
			case SEND_NORMAL:
			case SEND_WEBSOCKET:
				VIEWLIST;
                SEM_P_NULL(shms->semid,LIST_TODEL);
				list_add_tail(&(tmp_xxx_node1->list),&list_todel_head.list);
                SEM_V_NULL(shms->semid,LIST_TODEL);
				printf(INFO"msg is request,"NONE ACTION" insert the msg into list_todel_head\n"NONE);
				VIEWLIST;
				printf("\n\n");
				pthread_cond_signal(&cond2);
				break;

		  case RECV_1:

                SEM_P_NULL(shms->semid,LIST_TO_SEND);
				list_for_each_safe(pos,n,&list_tosend_head.list){
					tmp_xxx_node2 = list_entry(pos,list_xxx_t,list);//得到外层的数据
					if(tmp_xxx_node1->data.sha1[0] != 0 && !strcmp(tmp_xxx_node1->data.sha1,tmp_xxx_node2->data.sha1) && tmp_xxx_node1->data.count == tmp_xxx_node2->data.count){//对链表中的数据进行判断,如果满足条件就删节点
						is_exist = 1;
						break;
					}
				}


				if(is_exist){
					printf(INFO"msg is ack1,"NONE ACTION" Identity state RECV_1\n"NONE);

					tmp_xxx_node2->data.data_state = RECV_1;

					//修改 tosend list 的标识 为 recv_1

				}else{
					//调用回调,说不存在
					//已经不用调用回调了,因为链表已经不存在了,标识着已经做过回调了
				}
                SEM_V_NULL(shms->semid,LIST_TO_SEND);

				break;

		  case RECV_2:
                SEM_P_NULL(shms->semid,LIST_TO_SEND);
				list_for_each_safe(pos,n,&list_tosend_head.list){
					tmp_xxx_node2 = list_entry(pos,list_xxx_t,list);//得到外层的数据
					if(tmp_xxx_node1->data.sha1[0] != 0 &&  !strcmp(tmp_xxx_node1->data.sha1,tmp_xxx_node2->data.sha1) && tmp_xxx_node1->data.count == tmp_xxx_node2->data.count){//对链表中的数据进行判断,如果满足条件就删节点
						is_exist = 1;
						break;
					}
				}

				if(is_exist){
					//删链表
					VIEWLIST;
					printf(INFO"msg is ack2,"NONE ACTION" Deleted Corresponding message in list_tosend_head\n"NONE);
					list_del(pos); // 注意,删除链表,是删除的list_head,还需要删除 外层的数据 ,删除一个节点之后,并没有破坏这个节点和外围数据的位置关系
					//这里需要判断是什么ack ,因为ack 分为两种,一种是send 的ack
					//一种是 得到数据的ack.
					//得到 第一种ack 不能删,得到第二种ack 可以删. 
					free(tmp_xxx_node2);//释放数据
					VIEWLIST;
					printf("\n\n");

				}else{
					//调用回调,说不存在
					//已经不用调用回调了,因为链表已经不存在了,标识着已经做过回调了
				}
                SEM_V_NULL(shms->semid,LIST_TO_SEND);

				break;

			default:
				break;

		}

        pthread_mutex_unlock(&mutex);//TODO
    }

    return NULL;
}

void * recv_thread_2(void *arg){

    struct list_head *pos,*pos2,*n;
    list_xxx_t *tmp_xxx_node1;
    list_xxx_t *tmp_xxx_node;
    int flag = 0;
    int i , j , k;
    void (*todel)(list_xxx_t* list_todel_head) = NULL;

    while(1){

        pthread_mutex_lock(&mutex);
        pthread_cond_wait(&cond2,&mutex);

		//得到数据
        SEM_P_NULL(shms->semid,LIST_TODEL);
		list_for_each_safe(pos,n,&list_todel_head.list){  		
			tmp_xxx_node1 = list_entry(pos,list_xxx_t,list);//得到外层的数据

			if(process_type == WEBSOCKET){//注意,需要考虑,这里面是处理一个数据还是处理链表中所有的数据

					//得到函数
        SEM_P_NULL(shms->semid,SHM_RES);


					for(i=0;i<ARRAY_SIZE(shms->process_register);i++){
						if(shms->process_register[i].process_type == process_type && shms->process_register[i].pid != 0){
							//memcpy((char *)&(shms->process_register[i]),p,sizeof(shms->process_register[i]));
							todel = shms->process_register[i].msg_del_method.todel;
							break;
						}
					}

        SEM_V_NULL(shms->semid,SHM_RES);

					if (todel)
						todel(&list_todel_head);//这里面要处理给服务器发包 ,给进程发包,给进程的发包的时候需要置位 RECV_1;
					else
						printf(ERROR"error happend 23\n"NONE);


			}else{

				if(todel)
					printf(ERROR"error happend 34\n"NONE);
				else
					printf(INFO"nothing to del\n"NONE);

				VIEWLIST;
				list_del(pos); // 注意,删除链表,是删除的list_head,还需要删除 外层的数据 ,删除一个节点之后,并没有破坏这个节点和外围数据的位置关系
				list_add_tail(&(tmp_xxx_node1->list),&list_deled_head.list);
				printf(ACTION"move the msg to list_deled_head\n"NONE);
				VIEWLIST;
				printf("\n\n");

				pthread_cond_signal(&cond3);

			}
		}
        SEM_V_NULL(shms->semid,LIST_TODEL);
		pthread_mutex_unlock(&mutex);
	}
	return NULL;
}

void * recv_thread_3(void *arg){

	struct list_head *pos,*pos2,*n;
	list_xxx_t *tmp_xxx_node;
	list_xxx_t *tmp_xxx_node1;
	list_xxx_t *tmp_xxx_node2;
	int tmp;
	data_t data;
	int flag = 0;
	int ret = 0;
	void (*waitfor)(data_t *data) = NULL;
	int i = 0 , j  = 0 ,  k = 0;
	pid_t pid_tmp = -1;

	//得到函数

	if(process_type == WEBSOCKET){  //websocket 进程的处理函数

		//得到函数
        SEM_P_NULL(shms->semid,SHM_RES);
		for(i=0;i<ARRAY_SIZE(shms->process_register);i++){
			if(shms->process_register[i].process_type == process_type && shms->process_register[i].pid != 0){
				waitfor = shms->process_register[i].msg_del_method.waitfor;
				break;
			}
		}
        SEM_V_NULL(shms->semid,SHM_RES);

		while(1){

			//阻塞
			waitfor(&data);

			data.data_state = RECV_2;

			ret = pkt_send(&data,sizeof(data));
			if( ret < 0 ){
				printf("error happed\n");
			}

            SEM_P_NULL(shms->semid,LIST_DELED);

			list_for_each_safe(pos,n,&list_deled_head.list){  		
				tmp_xxx_node = list_entry(pos,list_xxx_t,list);//得到外层的数据
				if(1){//对链表中的数据进行判断,如果满足条件就删节点
					VIEWLIST;
					list_del(pos); // 注意,删除链表,是删除的list_head,还需要删除 外层的数据 ,删除一个节点之后,并没有破坏这个节点和外围数据的位置关系
					free(tmp_xxx_node);//释放数据
					VIEWLIST;
					printf("\n\n");
				}
			}
            SEM_V_NULL(shms->semid,LIST_DELED);
		}
	}

	//非 webscoket 进程
	while(1){
		pthread_mutex_lock(&mutex);
		pthread_cond_wait(&cond3,&mutex);

        SEM_P_NULL(shms->semid,LIST_DELED);
		list_for_each_safe(pos,n,&list_deled_head.list){  		
			tmp_xxx_node1 = list_entry(pos,list_xxx_t,list);//得到外层的数据
			//printf("one delte in list_deled_head \n");

			switch(tmp_xxx_node1->data.data_state){
				case SEND_NORMAL:
				case RECV_1:
					VIEWLIST;
					printf(ACTION"delete the msg from list_deled_head\n"NONE);
					list_del(pos); // 注意,删除链表,是删除的list_head,还需要删除 外层的数据 ,删除一个节点之后,并没有破坏这个节点和外围数据的位置关系
					VIEWLIST;
					printf("\n\n");

					tmp_xxx_node1->data.data_state = RECV_2;
					tmp = tmp_xxx_node1->data.pid_to;
					tmp_xxx_node1->data.pid_to = tmp_xxx_node1->data.pid_from;
					tmp_xxx_node1->data.pid_from = tmp;
					//发数据
					ret = pkt_send(&(tmp_xxx_node1->data),sizeof(tmp_xxx_node1->data));
					if( ret < 0 ){
						printf("error happed\n");
					}
					//
					//free
					free(tmp_xxx_node1);//释放数据
					pthread_mutex_unlock(&mutex);
					break;

				default:
					break;
			}

		}
        SEM_V_NULL(shms->semid,LIST_DELED);
	}
	return NULL;
}

