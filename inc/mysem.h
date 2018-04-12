/*************************************************************************
  > File Name: inc/mysem.h
  > Author: Sues
  > Mail: sumory.kaka@foxmail.com 
  > Created Time: Thu 12 Apr 2018 11:05:31 AM CST
 ************************************************************************/

#ifndef __MYSEM__H_
#define __MYSEM__H_

extern int sem_P(int semid, int semnum);
extern int sem_V(int semid, int semnum);

#define MSG perror("P operate error") ; printf("sws : %s,%s,line = %d\n",__FILE__,__func__,__LINE__)

#define SEM_P_BASE(semid,semnum) do{ if(sem_P((semid),(semnum)) < 0){ MSG;

#define SEM_V_BASE(semid,semnum) do{ if(sem_V((semid),(semnum)) < 0){ MSG;




#define SEM_P(semid,semnum) SEM_P_BASE(semid,semnum)  return ;  }  }while(0)


#define SEM_V(semid,semnum)  SEM_V_BASE(semid,semnum)  return ;  }  }while(0)

#define SEM_P_INT(semid,semnum)  SEM_P_BASE(semid,semnum)  return -33;  }  }while(0)


#define SEM_V_INT(semid,semnum)  SEM_V_BASE(semid,semnum)  return -34;  }  }while(0)
#define SEM_P_NULL(semid,semnum)  SEM_P_BASE(semid,semnum)  return NULL;  }  }while(0)


#define SEM_V_NULL(semid,semnum)  SEM_V_BASE(semid,semnum) return NULL;  }  }while(0)

enum shared_resource{
    SHM_RES,
    LIST_TO_SEND,
    LIST_TODEL,
    LIST_DELED,
    NUMBEROFSR,
};

//SEM_P(shms->semid,SHM_RES);
//SEM_V(shms->semid,SHM_RES);
#endif
