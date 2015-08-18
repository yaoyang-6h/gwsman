/* 
 * File:   CPipeShell.cc
 * Author: wyy
 * 
 * Created on 2014年2月22日, 上午8:33
 */

#include <stdio.h>
#include <signal.h>
#include <stdarg.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/select.h>
#include "PipeShell.h"


BOOL        PipeCreate(PipeShell* ps);
void        PipeClose(PipeShell* ps);
pid_t       PipePID(PipeShell* ps);
pid_t       PipeFork(PipeShell* ps);
int         PipeKill(PipeShell* ps);
BOOL        PipeRedirectStdIO(PipeShell* ps);
int         PipeTryRead(PipeShell* ps,char* buff,int nMax);
int         PipeTryRead0(PipeShell* ps,int fd,char* buff,int nMax);
int         PipeShellCmd(PipeShell* ps,const char* sFile,char* argv[]);

void PipeShellInit(PipeShell* ps) {
    if (!ps) return;
    ps->m_pipe[0] = -1;
    ps->m_pipe[1] = -1;
    ps->m_child_pid = 0;
    PipeCreate(ps);
}

void PipeShellExit(PipeShell* ps) {
    PipeClose(ps);
}

BOOL PipeCreate(PipeShell* ps) {
    PipeClose(ps);
    if (pipe(ps->m_pipe) < 0) return FALSE;
    return TRUE;
}

void PipeClose(PipeShell* ps) {
    if (ps->m_pipe[0] >= 0) close(ps->m_pipe[0]);
    if (ps->m_pipe[1] >= 0) close(ps->m_pipe[1]);
    ps->m_pipe[0] = -1;
    ps->m_pipe[1] = -1;
}

#define PIPE_PARENT_READ        ps->m_pipe[0]
#define PIPE_CHILD_WRITE        ps->m_pipe[1]

BOOL PipeRedirectStdIO(PipeShell* ps) {
    if (PIPE_CHILD_WRITE > 1) {
	close(PIPE_PARENT_READ);
	dup2(PIPE_CHILD_WRITE, STDOUT_FILENO);
	dup2(PIPE_CHILD_WRITE, STDERR_FILENO);
        close(PIPE_CHILD_WRITE);
    } else return FALSE;
}

pid_t PipePID(PipeShell* ps) {
    return ps->m_child_pid;
}

pid_t PipeFork(PipeShell* ps) {
    PipeKill(ps);
    ps->m_child_pid = fork();
    return ps->m_child_pid;
}

int PipeKill(PipeShell* ps) {
    int nStatus = 0;
    if( ps->m_child_pid > 0 ) {
        kill(ps->m_child_pid,SIGKILL);
        waitpid(ps->m_child_pid,&nStatus,0);
        ps->m_child_pid = 0;
    }
    return nStatus;
}

int PipeTryRead(PipeShell* ps,char* buff,int nMax) {
    if (PIPE_PARENT_READ > 0) {
        return PipeTryRead0(ps,PIPE_PARENT_READ, buff, nMax);
    } else {
        return 0;
    }
}

int PipeTryRead0(PipeShell* ps,int fd,char* buff,int nMax) {
    if (fd >= 0 && NULL != buff && nMax > 0) {
        fd_set rdfd;
        struct timeval tv;

        FD_ZERO(&rdfd);
        FD_SET(fd,&rdfd);
        tv.tv_sec=0;
        tv.tv_usec=50000;
        int ret = select(fd+1,&rdfd,NULL,NULL,&tv);
        if (ret > 0) {
            if(FD_ISSET(fd,&rdfd))
            return read( fd, buff, nMax );
        } else return 0;
    } else return 0;
}

int PipeShellCommand(PipeShell* ps,const char* sFile,...) {
    char*       argv[10] = {NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};
    int         i = 0;
    
    va_list ap;
    va_start(ap, sFile);
    
    argv[0] = (char*) sFile;
    for (i = 1; i < 10; i ++) {
        if ( NULL == (argv[i] = va_arg(ap, char*))) break;
    }
    va_end(ap);
    return PipeShellCmd(ps,sFile, argv);
}

int PipeShellCmd(PipeShell* ps,const char* sFile,char* argv[]) {
//    ps->m_CreatePipe();
    if (0 == PipeFork(ps)) {      //in the CHILDREN process
        PipeRedirectStdIO(ps);
        if (execvp(sFile,argv) < 0) {
            printf("***Failed on execute \"%s %s\"  (error code=%d) ***",sFile,argv,errno);
            fflush(stdout);
            kill(getpid(),SIGKILL);
        }
    } else if (PipePID(ps) > 0) {         //in the PARENT process
        return TRUE;
    } else {    //child_id == -1,error on creating child process
        printf("****************Error on exec command*****************");
        fflush(stdin);
        return FALSE;
    }
}