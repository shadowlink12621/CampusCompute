#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_JOBS 32
#define MAX_RES 5
#define QUEUE_CAP 8
#define LOG_PATH "campuscompute.log"

typedef struct { int id, arrival, burst, priority, remaining, completion, status; } Job;
static Job demo_jobs[] = {{1,0,7,2,0,0,0},{2,2,4,1,0,0,0},{3,4,1,3,0,0,0},{4,5,4,2,0,0,0}};
static const size_t demo_count = sizeof(demo_jobs)/sizeof(demo_jobs[0]);

static void usage(FILE *f) {
    fprintf(f,"CampusCompute - OS concepts demonstration (Linux)\n"
      "Usage: ./campuscompute <command>\n"
      "  all             Run every demonstration\n"
      "  fcfs|rr|priority|compare   CPU scheduling (built-in repeatable jobs)\n"
      "  logger          Bounded POSIX-thread producer/consumer and file log\n"
      "  ipc             POSIX shared memory + named semaphore exchange\n"
      "  banker [safe|unsafe]       Banker's safety algorithm\n"
      "  worker [normal|fail]       fork/waitpid worker status\n"
      "  limit           Demonstrate a child CPU-time resource limit\n"
      "  cleanup         Verify no IPC names from the last run remain\n");
}
static int cmp_arrival(const void *a,const void *b) { const Job*x=a,*y=b; if(x->arrival!=y->arrival)return x->arrival-y->arrival; return x->id-y->id; }
static void print_metrics(Job *j,size_t n,const char *name,const char *timeline) {
    double sw=0,st=0; printf("\n%s\nTimeline: %s\nTie break: earlier arrival, then lower job ID.\n",name,timeline);
    puts("Job  Arrival  Burst  Priority  Status     Completion  Turnaround  Waiting");
    for(size_t i=0;i<n;i++){int tat=j[i].completion-j[i].arrival,wait=tat-j[i].burst; sw+=wait;st+=tat;printf("J%-3d %-8d %-6d %-9d %-10s %-11d %-11d %d\n",j[i].id,j[i].arrival,j[i].burst,j[i].priority,j[i].status?"COMPLETED":"PENDING",j[i].completion,tat,wait);}
    printf("Average turnaround: %.2f | Average waiting: %.2f\n",st/n,sw/n);
}
static void schedule(const char *mode) {
    Job j[MAX_JOBS]; size_t n=demo_count; memcpy(j,demo_jobs,n*sizeof(Job)); qsort(j,n,sizeof(Job),cmp_arrival);
    char timeline[2048]=""; int t=0;
    if(!strcmp(mode,"fcfs")||!strcmp(mode,"priority")) {
      bool done[MAX_JOBS]={0};
      for(size_t k=0;k<n;){int pick=-1;for(size_t i=0;i<n;i++)if(!done[i]&&j[i].arrival<=t&&(pick<0||(strcmp(mode,"priority")==0?(j[i].priority<j[pick].priority||(j[i].priority==j[pick].priority&&(j[i].arrival<j[pick].arrival||(j[i].arrival==j[pick].arrival&&j[i].id<j[pick].id)))):(j[i].arrival<j[pick].arrival||(j[i].arrival==j[pick].arrival&&j[i].id<j[pick].id)))))pick=(int)i;
        if(pick<0){int next=0;while(next<(int)n&&done[next])next++;t=j[next].arrival;continue;} if(t<j[pick].arrival)t=j[pick].arrival; int start=t;t+=j[pick].burst;j[pick].completion=t;j[pick].status=1;done[pick]=true;char part[80];snprintf(part,sizeof(part),"J%d[%d-%d] ",j[pick].id,start,t);strcat(timeline,part);k++;}
      print_metrics(j,n,!strcmp(mode,"fcfs")?"FCFS":"Non-preemptive Priority",timeline);
    } else {
      for(size_t i=0;i<n;i++)j[i].remaining=j[i].burst; int left=(int)n,quantum=3; printf("Round Robin time quantum: %d\n",quantum);
      while(left){bool progressed=false;for(size_t i=0;i<n;i++)if(j[i].remaining>0&&j[i].arrival<=t){progressed=true;int start=t,run=j[i].remaining<quantum?j[i].remaining:quantum;t+=run;j[i].remaining-=run;char part[80];snprintf(part,sizeof(part),"J%d[%d-%d] ",j[i].id,start,t);strcat(timeline,part);if(!j[i].remaining){j[i].completion=t;j[i].status=1;left--;}}
        if(!progressed){int next=2147483647;for(size_t i=0;i<n;i++)if(j[i].remaining>0&&j[i].arrival<next)next=j[i].arrival;if(next==2147483647)break;t=next;}}
      print_metrics(j,n,"Round Robin",timeline);
    }
}

typedef struct { int value; } Event;
static Event q[QUEUE_CAP]; static int qhead,qtail,qcount,producers_left=2; static pthread_mutex_t qlock=PTHREAD_MUTEX_INITIALIZER; static pthread_cond_t qnotempty=PTHREAD_COND_INITIALIZER,qnotfull=PTHREAD_COND_INITIALIZER; static FILE *logfile;
static void logline(const char *s){printf("%s\n",s);if(logfile){fprintf(logfile,"%s\n",s);fflush(logfile);}}
static void *producer(void *arg){int id=(int)(long)arg;for(int i=1;i<=5;i++){pthread_mutex_lock(&qlock);while(qcount==QUEUE_CAP)pthread_cond_wait(&qnotfull,&qlock);q[qtail].value=id*100+i;qtail=(qtail+1)%QUEUE_CAP;qcount++;char b[96];snprintf(b,sizeof b,"producer %d queued event %d",id,i);logline(b);pthread_cond_signal(&qnotempty);pthread_mutex_unlock(&qlock);}pthread_mutex_lock(&qlock);producers_left--;pthread_cond_broadcast(&qnotempty);pthread_mutex_unlock(&qlock);return NULL;}
static void *consumer(void *arg){(void)arg;for(;;){pthread_mutex_lock(&qlock);while(!qcount&&producers_left)pthread_cond_wait(&qnotempty,&qlock);if(!qcount&&!producers_left){pthread_mutex_unlock(&qlock);break;}Event e=q[qhead];qhead=(qhead+1)%QUEUE_CAP;qcount--;char b[96];snprintf(b,sizeof b,"consumer processed event %d",e.value);logline(b);pthread_cond_signal(&qnotfull);pthread_mutex_unlock(&qlock);}return NULL;}
static void logger(void){logfile=fopen(LOG_PATH,"w");if(!logfile){perror("open log");return;}pthread_t p[2],c;qhead=qtail=qcount=0;producers_left=2;int rc=pthread_create(&c,NULL,consumer,NULL);if(rc){fprintf(stderr,"pthread_create consumer: %s\n",strerror(rc));fclose(logfile);logfile=NULL;return;}int started=0;for(long i=0;i<2;i++){rc=pthread_create(&p[i],NULL,producer,(void*)(i+1));if(rc){fprintf(stderr,"pthread_create producer: %s\n",strerror(rc));pthread_mutex_lock(&qlock);producers_left--;pthread_cond_broadcast(&qnotempty);pthread_mutex_unlock(&qlock);}else started++;}for(int i=0;i<started;i++)pthread_join(p[i],NULL);pthread_join(c,NULL);pthread_mutex_destroy(&qlock);pthread_cond_destroy(&qnotempty);pthread_cond_destroy(&qnotfull);fclose(logfile);logfile=NULL;puts("All producer and consumer threads joined; log written to campuscompute.log.");}

typedef struct { char text[128]; } Shared;
static const char *shm_name="/campuscompute_demo_shm",*sem_name="/campuscompute_demo_sem";
static void ipc_demo(void){int fd=-1;Shared*s=MAP_FAILED;sem_t*sem=SEM_FAILED;bool shm_created=false,sem_created=false;fd=shm_open(shm_name,O_CREAT|O_EXCL|O_RDWR,0600);if(fd<0){perror("shm_open (a previous demo may have left this name)");return;}shm_created=true;if(ftruncate(fd,sizeof(Shared))<0){perror("ftruncate");goto done;}s=mmap(NULL,sizeof(Shared),PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);if(s==MAP_FAILED){perror("mmap");goto done;}sem=sem_open(sem_name,O_CREAT|O_EXCL,0600,0);if(sem==SEM_FAILED){perror("sem_open");goto done;}sem_created=true;pid_t pid=fork();if(pid<0){perror("fork");goto done;}if(pid==0){snprintf(s->text,sizeof s->text,"job J42 completed by child PID %ld",(long)getpid());sem_post(sem);_exit(0);}if(sem_wait(sem)<0)perror("sem_wait");else printf("Parent received shared-memory message: %s\n",s->text);waitpid(pid,NULL,0);
done:if(s!=MAP_FAILED)munmap(s,sizeof(Shared));if(fd>=0)close(fd);if(sem!=SEM_FAILED)sem_close(sem);if(shm_created)shm_unlink(shm_name);if(sem_created)sem_unlink(sem_name);puts("Resources created by this run cleaned up.");}

#define P 5
static int banker_case(bool safe){int avail[MAX_RES]={3,3,2};int alloc[P][MAX_RES]={{0,1,0},{2,0,0},{3,0,2},{2,1,1},{0,0,2}};int maxm[P][MAX_RES]={{7,5,3},{3,2,2},{9,0,2},{2,2,2},{4,3,3}};if(!safe){avail[0]=0;avail[1]=0;avail[2]=0;}
 int need[P][MAX_RES],work[MAX_RES];bool finish[P]={0};int seq[P],count=0;for(int r=0;r<3;r++)work[r]=avail[r];puts("Available / Allocation / Maximum / Need (Maximum - Allocation):");
 for(int p=0;p<P;p++){printf("P%d alloc=[",p);for(int r=0;r<3;r++){need[p][r]=maxm[p][r]-alloc[p][r];printf("%s%d",r?" ":"",alloc[p][r]);}printf("] max=[");for(int r=0;r<3;r++)printf("%s%d",r?" ":"",maxm[p][r]);printf("] need=[");for(int r=0;r<3;r++)printf("%s%d",r?" ":"",need[p][r]);puts("]");}
 printf("Available=[%d %d %d]\n",work[0],work[1],work[2]);bool progress=true;while(progress){progress=false;for(int p=0;p<P;p++)if(!finish[p]&&need[p][0]<=work[0]&&need[p][1]<=work[1]&&need[p][2]<=work[2]){for(int r=0;r<3;r++)work[r]+=alloc[p][r];finish[p]=true;seq[count++]=p;progress=true;}}
 if(count==P){printf("SAFE; safe sequence: ");for(int i=0;i<P;i++)printf("P%d%s",seq[i],i==P-1?"\n":" -> ");return 0;}puts("UNSAFE: no sequence can finish every process.");return 0;}
static void worker(bool fail){pid_t p=fork();if(p<0){perror("fork");return;}if(!p){printf("Worker child PID %ld started (%s demo).\n",(long)getpid(),fail?"simulated failure":"normal");fflush(stdout);_exit(fail?7:0);}int st;if(waitpid(p,&st,0)<0){perror("waitpid");return;}if(WIFEXITED(st))printf("Parent reaped PID %ld: exit status %d (%s).\n",(long)p,WEXITSTATUS(st),WEXITSTATUS(st)==0?"normal completion":fail?"controlled simulated failure":"unexpected non-zero status");else if(WIFSIGNALED(st))printf("Parent reaped PID %ld: terminated by signal %d.\n",(long)p,WTERMSIG(st));}
static void limit_demo(void){struct rlimit old,lim;if(getrlimit(RLIMIT_CPU,&old)<0){perror("getrlimit");return;}lim=old;lim.rlim_cur=1;if(old.rlim_max!=RLIM_INFINITY&&lim.rlim_cur>old.rlim_max)lim.rlim_cur=old.rlim_max;printf("Child CPU soft limit: %llu seconds (hard limit unchanged).\n",(unsigned long long)lim.rlim_cur);pid_t p=fork();if(p<0){perror("fork");return;}if(!p){if(setrlimit(RLIMIT_CPU,&lim)<0){perror("setrlimit");_exit(2);}volatile unsigned long x=0;for(;;)x++;}int st;waitpid(p,&st,0);if(WIFSIGNALED(st))printf("Parent detected child stopped by signal %d after CPU limit.\n",WTERMSIG(st));else printf("Child exited with status %d (limit applied; a signal may be kernel/config dependent).\n",WIFEXITED(st)?WEXITSTATUS(st):-1);}
static void cleanup_check(void){int fd=shm_open(shm_name,O_RDWR,0);if(fd>=0){close(fd);puts("Shared-memory name still exists (not removed by this check).");}else puts("No CampusCompute shared-memory name remains.");sem_t*s=sem_open(sem_name,0);if(s!=SEM_FAILED){sem_close(s);puts("Semaphore name still exists (not removed by this check).");}else puts("No CampusCompute semaphore name remains.");puts("waitpid/join cleanup is performed by each demonstration before it returns.");}
static void all(void){puts("=== CampusCompute integrated demonstration ===");schedule("fcfs");schedule("rr");schedule("priority");logger();ipc_demo();banker_case(true);banker_case(false);worker(false);worker(true);limit_demo();cleanup_check();puts("=== Integrated run complete ===");}
int main(int argc,char**argv){if(argc!=2&&argc!=3){usage(stderr);return 2;}if(!strcmp(argv[1],"--help")||!strcmp(argv[1],"help")){usage(stdout);return 0;}if(!strcmp(argv[1],"fcfs")||!strcmp(argv[1],"rr")||!strcmp(argv[1],"priority")){schedule(argv[1]);return 0;}if(!strcmp(argv[1],"compare")){schedule("fcfs");schedule("rr");schedule("priority");return 0;}if(!strcmp(argv[1],"logger")){logger();return 0;}if(!strcmp(argv[1],"ipc")){ipc_demo();return 0;}if(!strcmp(argv[1],"banker")){if(argc==2||!strcmp(argv[2],"safe"))return banker_case(true);if(!strcmp(argv[2],"unsafe"))return banker_case(false);fprintf(stderr,"banker expects safe or unsafe\n");return 2;}if(!strcmp(argv[1],"worker")){if(argc==2||!strcmp(argv[2],"normal"))worker(false);else if(!strcmp(argv[2],"fail"))worker(true);else{fprintf(stderr,"worker expects normal or fail\n");return 2;}return 0;}if(!strcmp(argv[1],"limit")){limit_demo();return 0;}if(!strcmp(argv[1],"cleanup")){cleanup_check();return 0;}if(!strcmp(argv[1],"all")){all();return 0;}fprintf(stderr,"Unknown command: %s\n",argv[1]);usage(stderr);return 2;}
