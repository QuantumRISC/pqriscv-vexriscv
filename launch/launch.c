
#define _XOPEN_SOURCE
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <poll.h>

char *ptsname(int fd);
int grantpt(int fd);
int unlockpt(int fd);

int bakin,bakout;
 
int subprocess(char *cmd[], int *pid, int *infd, int *outfd, int masterfd) {

    int p1[2],p2[2];
    if(pipe(p1) == -1){ perror("pipe"); return 1;}
    if(pipe(p2) == -1){ perror("pipe"); return 1;}
    if((*pid=fork()) == -1){ perror("fork"); return 1;}
    if(*pid) {
        *infd = p1[1];
        *outfd = p2[0];
        close(p1[0]);
        close(p2[1]);
        return 0;
    } else {
        close(masterfd);
        dup2(p1[0],0);
        dup2(p2[1],1);
        close(p1[0]);
        close(p1[1]);
        close(p2[0]);
        close(p2[1]);
        execl(cmd[0],cmd[0],cmd[1],(char*)NULL);
    }
    return 0;
}
int nreadline(int fd, char *buf, int len) {
  for (int i = 0; i < len; i++) {
    if(read(fd,&buf[i],1) != 1) {
      break;
    }
    if(buf[i] == '\n') {
      return i;
    }
  }
  return 0;
}

int forkstream(int fdin, int fdout,char *flag) {
  int pid=0;
  if((pid=fork())) {
    return pid;
  } else {
    char byte;
    for(;;) {
      read(fdin,&byte,1);
      write(1,flag,1);
      write(fdout,&byte,1);
      fsync(fdout);
    }
  }
}

int main(int c, char **v, char*const*venv) {
  int status;
  int masterfd = open("/dev/ptmx",O_RDWR);
  char *slavename;
  grantpt(masterfd);
  unlockpt(masterfd);
  slavename = ptsname(masterfd);
  int infd,outfd,pid;
  if(subprocess(v+1, &pid, &infd, &outfd, masterfd)) {
      perror("subprocess");
  }
  char buf[512];
  int ret = 0;
  while(1) {
      ret=nreadline(outfd,buf,512);
      buf[511] = 0x00;

      if(strncmp("WAITING FOR TCP JTAG CONNECTION",buf,31) == 0) {
        buf[32] = 0;
        printf("%s",buf);
        break;
      }
  }
  ret=nreadline(outfd,buf,512);
  printf("Connect to %s for serial in/out.\n",slavename);
  int pidin = forkstream(outfd,masterfd,"<");
  int pidout = forkstream(masterfd,infd,">");
  waitpid(pid,&status,0);
  return 0;
}


//int main(int c, char **v, char*const*venv) {
//  int masterfd = open("/dev/ptmx",O_RDWR);
//  char *slavename;
//  grantpt(masterfd);
//  unlockpt(masterfd);
//  slavename = ptsname(masterfd);
//  open(slavename,O_RDWR);
//  printf("Connect to %s for serial in/out.\n",slavename);
//  printf("Press key to continue.\n");
//  getchar();
//  int fd[2];
//  int status;
//  pipe(fd);
//  pid_t pid = fork();
//  if (pid) {
//    close(masterfd);
//    waitpid(pid,&status,0);
//  } else {
//    grantpt(masterfd);
//    unlockpt(masterfd);
//    dup2(masterfd,fileno(stdout));
//    dup2(masterfd,fileno(stdin));
//    close(masterfd);
//    printf("test\n");
//    execl(v[1],v[2],(char*)NULL);
//  }
//  return 0;
//}
