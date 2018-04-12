/*************************************************************************
  > File Name: lib/sem.c
  > Author: Sues
  > Mail: sumory.kaka@foxmail.com 
  > Created Time: Thu 12 Apr 2018 11:04:37 AM CST
 ************************************************************************/

#include <stdio.h>
#include <sys/sem.h>
#include "common.h"
#include "mysem.h"
#include <sys/stat.h>
#include <sys/types.h>
#include "print_color.h"



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


int my_sem_init(char * path,int * semid,int number){

    key_t key2;//key定义
    int i;
    union semun arg;
    int ret = 0;

    if(access(path,F_OK) != 0)                                                   
    {                                                                                
        printf("mkdir SHM_PATH\n");                                                  
        ret = mkdir(path, 777);
        if(ret < 0){
            printf(RED"mkdir %s failed \n"NONE,path);
        }
        //system("mkdir " path);                                                   
    }   

    key2 = ftok(path, 0x66 ) ;
    if ( key2 < 0 )
    {
        perror("ftok key2 error") ;
        return -1 ;
    }
    /***本程序创建了4个信号量**/ //一个用于内存的互斥,另外三个分别用于3个链表的互斥
    *semid = semget(key2,number,IPC_CREAT|0600);
    if (*semid == -1)
    {
        perror("create semget error");
        return -1;
    }

    arg.val = 1;
    for ( i = 0; i < number; i++ ) {
        /***对0号信号量设置初始值***/
        ret =semctl(*semid,i,SETVAL,arg);
        if (ret < 0 )
        {
            perror("ctl sem error");
            semctl(*semid,0,IPC_RMID,arg);
            return -1 ;
        }
    }
    return 0;
}
