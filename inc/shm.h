/*************************************************************************
    > File Name: shm.h
    > Author: Sues
    > Mail: sumory.kaka@foxmail.com 
    > Created Time: Mon 02 Apr 2018 04:04:47 AM PDT
 ************************************************************************/


#ifndef __SHM_H_
#define __SHM_H_

int sem_P(int semid, int semnum);
int sem_V(int semid, int semnum);
int shm_init(void);
#endif
