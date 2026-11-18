#include "campuscompute.h"
/* FR6: Worker processes - fork() / waitpid() failure detection        */
/* ------------------------------------------------------------------ */

void worker(bool fail) {
    fflush(stdout);                                          /* avoid child duplicating buffered output */
    pid_t p = fork();
    if (p < 0) { perror("fork"); return; }
    if (!p) {   /* child: the "worker" */
        printf("Worker child PID %ld started (%s demo).\n", (long)getpid(), fail ? "SIMULATED failure" : "normal");
        fflush(stdout);
        _exit(fail ? 7 : 0);    /* exit code 7 is a deliberate, simulated failure - not a real OS fault */
    }
    int st;
    if (waitpid(p, &st, 0) < 0) { perror("waitpid"); return; }
    if (WIFEXITED(st))
        printf("Parent reaped PID %ld: exit status %d (%s).\n", (long)p, WEXITSTATUS(st),
               WEXITSTATUS(st) == 0 ? "normal completion" : fail ? "controlled SIMULATED failure" : "unexpected non-zero status");
    else if (WIFSIGNALED(st))
        printf("Parent reaped PID %ld: terminated by signal %d (%s).\n", (long)p, WTERMSIG(st), strsignal(WTERMSIG(st)));
}

/* ------------------------------------------------------------------ */
/* FR7: Resource limits (RLIMIT_CPU) and cleanup                       */
/* ------------------------------------------------------------------ */

void limit_demo(void) {
    struct rlimit old, lim;
    if (getrlimit(RLIMIT_CPU, &old) < 0) { perror("getrlimit"); return; }
    lim = old;
    lim.rlim_cur = 1;   /* soft limit: 1 CPU-second; hard limit left unchanged */
    if (old.rlim_max != RLIM_INFINITY && lim.rlim_cur > old.rlim_max) lim.rlim_cur = old.rlim_max;
    printf("Child CPU soft limit: %llu second(s) (hard limit unchanged).\n", (unsigned long long)lim.rlim_cur);

    time_t t0 = time(NULL);
    fflush(stdout);                                          /* avoid child duplicating buffered output */
    pid_t p = fork();
    if (p < 0) { perror("fork"); return; }
    if (!p) {   /* limit is set only in the child, so the parent is unaffected */
        if (setrlimit(RLIMIT_CPU, &lim) < 0) { perror("setrlimit"); _exit(2); }
        volatile unsigned long x = 0;
        for (;;) {                                             /* burn CPU until the kernel sends SIGXCPU */
            if (++x == ULONG_MAX) x = 0;                       /* read the counter; keep the loop observable */
        }
    }
    int st;
    waitpid(p, &st, 0);
    if (WIFSIGNALED(st))
        printf("Parent detected child %ld killed by signal %d (%s) after ~%ld s of CPU: the limit worked.\n",
               (long)p, WTERMSIG(st), strsignal(WTERMSIG(st)), (long)(time(NULL) - t0));
    else
        printf("Child exited with status %d (limit applied; signal behaviour may be kernel/config dependent).\n",
               WIFEXITED(st) ? WEXITSTATUS(st) : -1);
}

/* Remove only this program's two IPC names (if a crashed run left them) and verify none remain */
int cleanup_check(void) {
    int result = 0;
    int fd = shm_open(SHM_NAME, O_RDWR, 0);
    if (fd >= 0) { close(fd); shm_unlink(SHM_NAME); puts("Found stale shared-memory name from an earlier run: removed."); }
    else if (errno != ENOENT) { perror("shm_open cleanup check"); result = 1; }
    sem_t *s = sem_open(SEM_NAME, 0);
    if (s != SEM_FAILED) { sem_close(s); sem_unlink(SEM_NAME); puts("Found stale semaphore name from an earlier run: removed."); }
    else if (errno != ENOENT) { perror("sem_open cleanup check"); result = 1; }

    fd = shm_open(SHM_NAME, O_RDWR, 0);
    if (fd >= 0) { close(fd); puts("Shared-memory name still exists!"); result = 1; }
    else if (errno == ENOENT) puts("Verified: no CampusCompute shared-memory name remains.");
    else { perror("shm_open verification"); result = 1; }
    s = sem_open(SEM_NAME, 0);
    if (s != SEM_FAILED) { sem_close(s); puts("Semaphore name still exists!"); result = 1; }
    else if (errno == ENOENT) puts("Verified: no CampusCompute semaphore name remains.");
    else { perror("sem_open verification"); result = 1; }
    puts("Threads are joined and children are reaped (waitpid) inside each demonstration before it returns.");
    return result;
}

