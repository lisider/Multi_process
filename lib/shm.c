/*************************************************************************
  > File Name: shm.c
  > Author: Sues
  > Mail: sumory.kaka@foxmail.com 
  > Created Time: Mon 02 Apr 2018 02:58:27 PM CST
 ************************************************************************/

#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "common.h"
#include <errno.h>
#include <stdio.h>
#include <sys/sem.h>
#include "print_color.h"
#include <strings.h>
#include <stdlib.h>
#include "read_write_state_api.h"
#include "mysem.h"

//这里包含了 systemV 信号量的操作,待分离

extern struct shm *shms;
extern int shmid;



int shm_init(void){

    key_t key;//key定义
    key_t key2;//key定义
    int fisttime = 0;
    int ret = 0;
    int nattch = 0;
    int maxid,id;
    struct shmid_ds shmseg;
    struct shm_info shm_info;
    int tmp_shmid;
    int i = 0;

    //  int access(const char *filename, int mode);                                  
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

        printf(INFO"i am 1st\n"NONE);
        bzero(shms,sizeof(struct shm)); //清 0 共享内存
        init_status((char *)&(shms->read_write_state));
        shms->unwriteable_times_send=0;
        //sem_init((sem_t *)&(shms->sem), 0, 1); 
        union semun arg;

        key2 = ftok("/tmp", 0x66 ) ;
        if ( key2 < 0 )
        {
            perror("ftok key2 error") ;
            return -1 ;
        }
        /***本程序创建了4个信号量**/ //一个用于内存的互斥,另外三个分别用于3个链表的互斥
        shms->semid = semget(key2,NUMBEROFSR,IPC_CREAT|0600);
        if (shms->semid == -1)
        {
            perror("create semget error");
            return -1;
        }

        arg.val = 1;
        for ( i = 0; i < NUMBEROFSR; i++ ) {
            /***对0号信号量设置初始值***/
            ret =semctl(shms->semid,i,SETVAL,arg);
            if (ret < 0 )
            {
                perror("ctl sem error");
                semctl(shms->semid,0,IPC_RMID,arg);
                return -1 ;
            }
        }

    }else{
        maxid = shmctl(0, SHM_INFO, (struct shmid_ds *) (void *) &shm_info);
        for (id = maxid; id >= 0; id--) { 
            tmp_shmid = shmctl(id, SHM_STAT, &shmseg);
            if(tmp_shmid == shmid)
            {
                nattch = shmseg.shm_nattch;
                //printf("%d,%ld\t",tmp_shmid,(long) shmseg.shm_nattch);
                break;
            }
        }

        printf(INFO"i am %dth\n"NONE,nattch);

    }
    return 0;
}

