/* 
 * File:   CPipeShell.h
 * Author: wyy
 *
 * Created on 2014年2月22日, 上午8:33
 */

#ifndef CPIPESHELL_H
#define	CPIPESHELL_H

#ifdef	__cplusplus
extern "C" {
#endif

#define BOOL    int
#define TRUE    1
#define FALSE   0

typedef struct __pipe_shell {
    pid_t       m_child_pid;
    int         m_pipe[2];
} PipeShell;

void PipeShellInit(PipeShell* ps);
void PipeShellExit(PipeShell* ps);
int PipeShellCommand(PipeShell* ps,const char* sFile,...);

#ifdef	__cplusplus
}
#endif

#endif	/* CPIPESHELL_H */

