#include <stddef.h>
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include "shmem.h"

P_GWS_KPI OpenShm(int create,int* pId) {
//    key_t k_id = ftok(GWS_SHMEM_ID,'a');
    static P_GWS_KPI pKpi = NULL;
    int nFlag = create ? IPC_CREAT|0600 : 0600;
    
//    if (-1 == k_id) {
//        printf("\n%s::Error on create k_id.\n",__func__);
//        return NULL;
//    }
    if (NULL == pId) return NULL;
    if ((*pId = shmget(GWS_SHMEM_ID,sizeof(GWS_KPI),nFlag)) >= 0) {
        if ((pKpi = (GWS_KPI*) shmat(*pId,NULL,0)) != NULL) {
//            printf("\n%s() Init Share memory OK\n",__func__);
            return pKpi;
        } else {
            printf("\n%s::Error on shmat shm_id:%d.\n",__func__,*pId);
            return NULL;
        }
    } else {
        printf("\n%s::Error on %s shm_id:%d.\n",__func__,*pId,create ? "create" : "connect");
        return NULL;
    }
}

//create share memory for KPI,if the shm already exist,return NULL;
P_GWS_KPI CreateShm(int* identifier) { return OpenShm(1,identifier); }
P_GWS_KPI ConnectShm(int* identifier) { return OpenShm(0,identifier); }

int CloseShm(P_GWS_KPI pKpi) {
    if (pKpi)
        return shmdt(pKpi);
    else return 0;
}

int DestoryShm(int identifier) {
    if (identifier >= 0) {
        shmctl(identifier,IPC_RMID,NULL);
        return -1;
    }
    return identifier;
}

int HoldShm(int identifier) {
    if (identifier >= 0) {
        return (0 == shmctl(identifier,SHM_LOCK,NULL));
    } else return 0;
}

int ReleaseShm(int identifier) {
    if (identifier >= 0) {
        return (0 == shmctl(identifier,SHM_UNLOCK,NULL));
    } else return 0;
}

int OpenMessageQueue(int server) {
    int nFlag = server ? IPC_CREAT|0666 : 0666;
//    int nFlag = server ? IPC_CREAT|IPC_EXCL|0666 : 0666;
//    int nFlag = IPC_CREAT|0666;
    
    printf("\n%s() Init message Queue.\n",__func__);
    return msgget(GWS_MSG_Q_ID,nFlag);
}

void CloseMessageQueue(int queueID) {
    if (queueID >= 0)
        msgctl(queueID,IPC_RMID,NULL);
}