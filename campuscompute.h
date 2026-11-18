#ifndef CAMPUSCOMPUTE_H
#define CAMPUSCOMPUTE_H
#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
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
#include <time.h>
#include <unistd.h>
#define MAX_JOBS 32
#define MAX_RES 5
#define QUEUE_CAP 3
#define LOG_PATH "campuscompute.log"
#define RR_QUANTUM 3
#define SHM_NAME "/campuscompute_demo_shm"
#define SEM_NAME "/campuscompute_demo_sem"
typedef enum { ST_NEW, ST_READY, ST_RUNNING, ST_DONE } Status;
typedef struct { double avg_wait, avg_tat; } Summary;
int schedule(const char *, const char *, int, Summary *);
int compare_all(const char *);
int logger(void);
int ipc_demo(void);
int banker_case(bool);
void worker(bool);
void limit_demo(void);
int cleanup_check(void);
void usage(FILE *);
#endif
