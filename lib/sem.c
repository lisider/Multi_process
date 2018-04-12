/*************************************************************************
  > File Name: lib/sem.c
  > Author: Sues
  > Mail: sumory.kaka@foxmail.com 
  > Created Time: Thu 12 Apr 2018 11:04:37 AM CST
 ************************************************************************/

#include <stdio.h>
#include <sys/sem.h>


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

