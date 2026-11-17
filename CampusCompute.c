/*
 * CampusCompute - Intelligent Job Scheduling and Resource Management
 * Operating Systems (UE25CS242A) Level 3 Mini-Project
 *
 * Demonstrates: CPU scheduling, pthread producer/consumer logger,
 * POSIX shared memory + named semaphore IPC, Banker's safety algorithm,
 * fork()/waitpid() worker monitoring, and setrlimit() resource limits.
 *
 * Build: make        Run: ./campuscompute help
 */
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

#define MAX_JOBS   32
#define MAX_RES    5
#define QUEUE_CAP  3            /* small on purpose so "queue full" is visible */
#define LOG_PATH   "campuscompute.log"
#define RR_QUANTUM 3

/* ------------------------------------------------------------------ */
/* FR1: Job management and lifecycle                                   */
/* ------------------------------------------------------------------ */

/* Lifecycle states a job moves through: NEW -> READY -> RUNNING -> DONE */
typedef enum { ST_NEW, ST_READY, ST_RUNNING, ST_DONE } Status;
static const char *status_name[] = { "NEW", "READY", "RUNNING", "COMPLETED" };

typedef struct {
    int id, arrival, burst, priority;   /* input fields (lower priority number = more urgent) */
    int remaining, completion;          /* filled in during scheduling */
    Status status;
} Job;

/* Built-in repeatable job set (used when no job file is given) */
static const Job demo_jobs[] = {
    {1, 0, 7, 2, 0, 0, ST_NEW}, {2, 2, 4, 1, 0, 0, ST_NEW},
    {3, 4, 1, 3, 0, 0, ST_NEW}, {4, 5, 4, 2, 0, 0, ST_NEW}
};
static const size_t demo_count = sizeof(demo_jobs) / sizeof(demo_jobs[0]);

static void usage(FILE *f) {
    fprintf(f,
        "CampusCompute - OS concepts demonstration (Linux)\n"
        "Usage: ./campuscompute <command> [arguments]\n"
        "  all                          Run every demonstration\n"
        "  fcfs|priority [jobfile]    CPU scheduling (built-in jobs or a job file)\n"
        "  rr [quantum] [jobfile]      Round Robin (default quantum: 3)\n"
        "  compare [jobfile]           Compare all policies (RR default quantum: 3)\n"
        "                               job file: one job per line -> id arrival burst priority\n"
        "  logger                       Bounded POSIX-thread producer/consumer + file log\n"
        "  ipc                          POSIX shared memory + named semaphore exchange\n"
        "  banker [safe|unsafe]         Banker's safety algorithm\n"
        "  worker [normal|fail]         fork/waitpid worker status\n"
        "  limit                        Demonstrate a child CPU-time resource limit\n"
        "  cleanup                      Remove/verify this program's IPC names\n"
        "  help                         Show this message\n");
}

/* Validate one job; prints a reason and returns false if malformed */
static bool validate_job(const Job *j, const Job *all, size_t n, int line) {
    if (j->id <= 0)       { fprintf(stderr, "Line %d: job ID must be > 0\n", line); return false; }
    if (j->arrival < 0)   { fprintf(stderr, "Line %d: arrival time must be >= 0\n", line); return false; }
    if (j->burst <= 0)    { fprintf(stderr, "Line %d: CPU burst must be > 0\n", line); return false; }
    if (j->priority < 0)  { fprintf(stderr, "Line %d: priority must be >= 0\n", line); return false; }
    for (size_t i = 0; i < n; i++)
        if (all[i].id == j->id) { fprintf(stderr, "Line %d: duplicate job ID %d\n", line, j->id); return false; }
    return true;
}

/* Load jobs from a text file; returns job count, or -1 on any error */
static int load_jobs(const char *path, Job *out) {
    FILE *f = fopen(path, "r");
    if (!f) { fprintf(stderr, "Cannot open job file '%s': %s\n", path, strerror(errno)); return -1; }
    char buf[128];
    int line = 0;
    size_t n = 0;
    while (fgets(buf, sizeof buf, f)) {
        line++;
        if (buf[0] == '#' || buf[0] == '\n') continue;      /* comments / blank lines */
        Job j = {0};
        char extra;
        /* exactly four integers expected; a fifth token is an error */
        if (sscanf(buf, "%d %d %d %d %c", &j.id, &j.arrival, &j.burst, &j.priority, &extra) != 4) {
            fprintf(stderr, "Line %d: malformed job (expected: id arrival burst priority)\n", line);
            fclose(f); return -1;
        }
        if (n >= MAX_JOBS) { fprintf(stderr, "Too many jobs (max %d)\n", MAX_JOBS); fclose(f); return -1; }
        if (!validate_job(&j, out, n, line)) { fclose(f); return -1; }
        j.status = ST_NEW;
        out[n++] = j;
    }
    fclose(f);
    if (n == 0) { fprintf(stderr, "Job file '%s' contains no jobs\n", path); return -1; }
    return (int)n;
}

/* ------------------------------------------------------------------ */
/* FR2: CPU scheduling (FCFS, Round Robin, non-preemptive Priority)    */
/* ------------------------------------------------------------------ */

typedef struct { double avg_wait, avg_tat; } Summary;

/* Sort by arrival time; ties broken by lower job ID */
static int cmp_arrival(const void *a, const void *b) {
    const Job *x = a, *y = b;
    if (x->arrival != y->arrival) return x->arrival - y->arrival;
    return x->id - y->id;
}

/* Append text to the timeline buffer without overflowing it */
static void timeline_add(char *tl, size_t cap, int id, int start, int end) {
    char part[48];
    snprintf(part, sizeof part, "J%d[%d-%d] ", id, start, end);
    strncat(tl, part, cap - strlen(tl) - 1);
}

/* Print per-job table and averages; fill the Summary for comparisons */
static void print_metrics(const Job *j, size_t n, const char *name, const char *tie_rule,
                          const char *tl, Summary *sum) {
    double sw = 0, st = 0;
    printf("\n=== %s ===\nTimeline: %s\n", name, tl);
    printf("Tie-break rule: %s\n", tie_rule);
    puts("Job  Arrival  Burst  Priority  Status     Completion  Turnaround  Waiting");
    for (size_t i = 0; i < n; i++) {
        int tat = j[i].completion - j[i].arrival;   /* turnaround = completion - arrival */
        int wait = tat - j[i].burst;                /* waiting    = turnaround - burst   */
        sw += wait; st += tat;
        printf("J%-3d %-8d %-6d %-9d %-10s %-11d %-11d %d\n", j[i].id, j[i].arrival, j[i].burst,
               j[i].priority, status_name[j[i].status], j[i].completion, tat, wait);
    }
    printf("Average turnaround: %.2f | Average waiting: %.2f\n", st / n, sw / n);
    if (sum) { sum->avg_tat = st / n; sum->avg_wait = sw / n; }
}

/* true if job a should run before job b under the given policy */
static bool runs_before(const Job *a, const Job *b, bool use_priority) {
    if (use_priority && a->priority != b->priority) return a->priority < b->priority;
    if (a->arrival != b->arrival) return a->arrival < b->arrival;
    return a->id < b->id;
}

/* mode = "fcfs" | "rr" | "priority"; jobfile may be NULL (use built-in jobs). Returns 0 on success. */
static int schedule(const char *mode, const char *jobfile, int quantum, Summary *sum) {
    Job j[MAX_JOBS];
    size_t n;
    if (jobfile) {
        int c = load_jobs(jobfile, j);
        if (c < 0) return 1;
        n = (size_t)c;
    } else {
        n = demo_count;
        memcpy(j, demo_jobs, n * sizeof(Job));
    }
    qsort(j, n, sizeof(Job), cmp_arrival);
    for (size_t i = 0; i < n; i++) { j[i].remaining = j[i].burst; j[i].status = ST_READY; }

    char timeline[2048] = "";
    int t = 0;

    if (!strcmp(mode, "fcfs") || !strcmp(mode, "priority")) {
        bool use_prio = !strcmp(mode, "priority");
        bool done[MAX_JOBS] = {0};
        for (size_t k = 0; k < n; k++) {
            /* choose the best job among those that have already arrived */
            int pick = -1;
            for (size_t i = 0; i < n; i++)
                if (!done[i] && j[i].arrival <= t && (pick < 0 || runs_before(&j[i], &j[pick], use_prio)))
                    pick = (int)i;
            if (pick < 0) {                      /* CPU idle: jump to the next arrival */
                for (size_t i = 0; i < n; i++)
                    if (!done[i]) { t = j[i].arrival; break; }   /* array is sorted by arrival */
                k--; continue;
            }
            int start = t;
            j[pick].status = ST_RUNNING;
            t += j[pick].burst;                  /* non-preemptive: run to completion */
            j[pick].completion = t;
            j[pick].status = ST_DONE;
            done[pick] = true;
            timeline_add(timeline, sizeof timeline, j[pick].id, start, t);
        }
        print_metrics(j, n, use_prio ? "Non-preemptive Priority" : "FCFS",
                      use_prio ? "lower numeric priority, then earlier arrival, then lower job ID"
                               : "earlier arrival, then lower job ID",
                      timeline, sum);
    } else {
        /* Round Robin with a real FIFO ready queue.
         * Jobs that arrive while another runs are queued BEFORE the preempted job is re-queued. */
        int ready[MAX_JOBS], rh = 0, rc = 0;
        size_t next = 0, finished = 0;
        printf("Round Robin time quantum: %d\n", quantum);
        while (finished < n) {
            while (next < n && j[next].arrival <= t) { ready[(rh + rc++) % MAX_JOBS] = (int)next++; }
            if (rc == 0) { t = j[next].arrival; continue; }     /* idle: jump to next arrival */
            int i = ready[rh]; rh = (rh + 1) % MAX_JOBS; rc--;
            int run = j[i].remaining < quantum ? j[i].remaining : quantum;
            int start = t;
            j[i].status = ST_RUNNING;
            t += run; j[i].remaining -= run;
            timeline_add(timeline, sizeof timeline, j[i].id, start, t);
            while (next < n && j[next].arrival <= t) { ready[(rh + rc++) % MAX_JOBS] = (int)next++; }
            if (j[i].remaining > 0) { j[i].status = ST_READY; ready[(rh + rc++) % MAX_JOBS] = i; }
            else { j[i].completion = t; j[i].status = ST_DONE; finished++; }
        }
        print_metrics(j, n, "Round Robin",
                      "FIFO ready queue; equal-time arrivals are ordered by lower job ID",
                      timeline, sum);
    }
    return 0;
}

/* S09: run all three policies on the same job set and compare averages */
static int compare_all(const char *jobfile) {
    const char *modes[3] = { "fcfs", "rr", "priority" };
    const char *names[3] = { "FCFS", "Round Robin (q=3)", "Priority (non-preemptive)" };
    Summary s[3];
    for (int i = 0; i < 3; i++)
        if (schedule(modes[i], jobfile, RR_QUANTUM, &s[i])) return 1;
    puts("\n=== Scheduling comparison (same job set) ===");
    puts("Policy                       Avg Waiting  Avg Turnaround");
    int bw = 0, bt = 0;
    for (int i = 0; i < 3; i++) {
        printf("%-28s %-12.2f %.2f\n", names[i], s[i].avg_wait, s[i].avg_tat);
        if (s[i].avg_wait < s[bw].avg_wait) bw = i;
        if (s[i].avg_tat < s[bt].avg_tat) bt = i;
    }
    printf("Lowest average waiting time: %s | Lowest average turnaround: %s\n", names[bw], names[bt]);
    return 0;
}

/* ------------------------------------------------------------------ */
/* FR3: Concurrent logger - bounded producer/consumer with pthreads    */
/* ------------------------------------------------------------------ */

typedef struct { int value; } Event;

static Event q[QUEUE_CAP];
static int qhead, qtail, qcount, producers_left = 2, log_seq;
static pthread_mutex_t qlock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t qnotempty = PTHREAD_COND_INITIALIZER, qnotfull = PTHREAD_COND_INITIALIZER;
static FILE *logfile;

/* Print and persist one log line. Caller must hold qlock (keeps lines ordered). */
static void logline(const char *s) {
    log_seq++;
    printf("[%02d] %s\n", log_seq, s);
    if (logfile) { fprintf(logfile, "[%02d] %s\n", log_seq, s); fflush(logfile); }
}

static void *producer(void *arg) {
    int id = (int)(long)arg;
    char b[96];
    for (int i = 1; i <= 5; i++) {
        pthread_mutex_lock(&qlock);
        if (qcount == QUEUE_CAP) {
            snprintf(b, sizeof b, "producer %d BLOCKED: queue full", id); logline(b);
        }
        while (qcount == QUEUE_CAP)                    /* while: guards against spurious wakeups */
            pthread_cond_wait(&qnotfull, &qlock);
        q[qtail].value = id * 100 + i;
        qtail = (qtail + 1) % QUEUE_CAP;
        qcount++;
        snprintf(b, sizeof b, "producer %d queued event %d (queue size %d)", id, i, qcount); logline(b);
        pthread_cond_signal(&qnotempty);
        pthread_mutex_unlock(&qlock);
    }
    pthread_mutex_lock(&qlock);
    producers_left--;                                  /* tell the consumer this producer is finished */
    pthread_cond_broadcast(&qnotempty);
    pthread_mutex_unlock(&qlock);
    return NULL;
}

static void *consumer(void *arg) {
    (void)arg;
    for (;;) {
        pthread_mutex_lock(&qlock);
        while (!qcount && producers_left)
            pthread_cond_wait(&qnotempty, &qlock);
        if (!qcount && !producers_left) { pthread_mutex_unlock(&qlock); break; }   /* all done */
        Event e = q[qhead];
        qhead = (qhead + 1) % QUEUE_CAP;
        qcount--;
        char b[96];
        snprintf(b, sizeof b, "consumer processed event %d (queue size %d)", e.value, qcount); logline(b);
        pthread_cond_signal(&qnotfull);
        pthread_mutex_unlock(&qlock);
        usleep(20000);   /* simulate slow consumer so the bounded queue actually fills up */
    }
    return NULL;
}

static void logger(void) {
    logfile = fopen(LOG_PATH, "w");
    if (!logfile) { perror("open log"); return; }
    pthread_t p[2], c;
    qhead = qtail = qcount = log_seq = 0;
    producers_left = 2;
    int rc = pthread_create(&c, NULL, consumer, NULL);
    if (rc) { fprintf(stderr, "pthread_create consumer: %s\n", strerror(rc)); fclose(logfile); logfile = NULL; return; }
    int started = 0;
    for (long i = 0; i < 2; i++) {
        rc = pthread_create(&p[i], NULL, producer, (void *)(i + 1));
        if (rc) {   /* a producer failed to start: account for it so the consumer can still finish */
            fprintf(stderr, "pthread_create producer: %s\n", strerror(rc));
            pthread_mutex_lock(&qlock); producers_left--; pthread_cond_broadcast(&qnotempty); pthread_mutex_unlock(&qlock);
        } else started++;
    }
    for (int i = 0; i < started; i++) pthread_join(p[i], NULL);   /* orderly termination */
    pthread_join(c, NULL);
    pthread_mutex_destroy(&qlock); pthread_cond_destroy(&qnotempty); pthread_cond_destroy(&qnotfull);
    fclose(logfile); logfile = NULL;
    printf("All producer and consumer threads joined; log written to %s.\n", LOG_PATH);
}

/* ------------------------------------------------------------------ */
/* FR4: IPC - POSIX shared memory + named semaphore                    */
/* ------------------------------------------------------------------ */

typedef struct { char text[128]; } Shared;
static const char *shm_name = "/campuscompute_demo_shm", *sem_name = "/campuscompute_demo_sem";

static void ipc_demo(void) {
    int fd = -1;
    Shared *s = MAP_FAILED;
    sem_t *sem = SEM_FAILED;
    bool shm_created = false, sem_created = false;

    /* O_EXCL: fail if the name already exists, so we never touch someone else's object */
    fd = shm_open(shm_name, O_CREAT | O_EXCL | O_RDWR, 0600);
    if (fd < 0) { perror("shm_open (stale name? run './campuscompute cleanup')"); return; }
    shm_created = true;
    if (ftruncate(fd, sizeof(Shared)) < 0) { perror("ftruncate"); goto done; }
    s = mmap(NULL, sizeof(Shared), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (s == MAP_FAILED) { perror("mmap"); goto done; }
    sem = sem_open(sem_name, O_CREAT | O_EXCL, 0600, 0);    /* initial value 0: parent must wait */
    if (sem == SEM_FAILED) { perror("sem_open"); goto done; }
    sem_created = true;
    printf("Parent: created shared memory %s and semaphore %s (value 0).\n", shm_name, sem_name);

    fflush(stdout);                                          /* avoid child duplicating buffered output */
    pid_t pid = fork();
    if (pid < 0) { perror("fork"); goto done; }
    if (pid == 0) {                                          /* child = producer of the message */
        snprintf(s->text, sizeof s->text, "job J42 completed by child PID %ld", (long)getpid());
        printf("Child : wrote message to shared memory, posting semaphore.\n");
        fflush(stdout);
        sem_post(sem);                                       /* signal: data is ready */
        _exit(0);
    }
    printf("Parent: waiting on semaphore for child's data...\n");
    if (sem_wait(sem) < 0) perror("sem_wait");
    else printf("Parent: received shared-memory message: %s\n", s->text);
    waitpid(pid, NULL, 0);                                   /* reap child: no zombie */

done:
    if (s != MAP_FAILED) munmap(s, sizeof(Shared));
    if (fd >= 0) close(fd);
    if (sem != SEM_FAILED) sem_close(sem);
    if (shm_created) shm_unlink(shm_name);                   /* remove ONLY what we created */
    if (sem_created) sem_unlink(sem_name);
    puts("Resources created by this run cleaned up (munmap, close, shm_unlink, sem_unlink).");
}

/* ------------------------------------------------------------------ */
/* FR5: Banker's safety algorithm                                      */
/* ------------------------------------------------------------------ */

#define NP 5      /* processes */
#define NR 3      /* resource types */

/* safe=true uses the classic textbook Available; safe=false uses a smaller Available
 * ({1,1,0}) with the same Allocation/Maximum, so no process's Need can be met. */
static int banker_case(bool safe) {
    int avail_safe[NR] = {3, 3, 2}, avail_unsafe[NR] = {1, 1, 0};
    int alloc[NP][NR] = {{0,1,0},{2,0,0},{3,0,2},{2,1,1},{0,0,2}};
    int maxm [NP][NR] = {{7,5,3},{3,2,2},{9,0,2},{2,2,2},{4,3,3}};
    int need[NP][NR], work[NR], seq[NP], count = 0;
    bool finish[NP] = {0};

    for (int r = 0; r < NR; r++) work[r] = safe ? avail_safe[r] : avail_unsafe[r];
    printf("\n=== Banker's algorithm: %s case ===\n", safe ? "SAFE" : "UNSAFE");
    puts("Process  Allocation  Maximum  Need (= Maximum - Allocation)");
    for (int p = 0; p < NP; p++) {
        printf("P%d       [", p);
        for (int r = 0; r < NR; r++) { need[p][r] = maxm[p][r] - alloc[p][r]; printf("%s%d", r ? " " : "", alloc[p][r]); }
        printf("]   [");
        for (int r = 0; r < NR; r++) printf("%s%d", r ? " " : "", maxm[p][r]);
        printf("]  [");
        for (int r = 0; r < NR; r++) printf("%s%d", r ? " " : "", need[p][r]);
        puts("]");
    }
    printf("Available = [%d %d %d]\n", work[0], work[1], work[2]);

    /* Safety algorithm: repeatedly find an unfinished process whose Need <= Work,
     * assume it finishes and releases its Allocation back into Work. */
    bool progress = true;
    while (progress) {
        progress = false;
        for (int p = 0; p < NP; p++) {
            if (finish[p]) continue;
            bool ok = true;
            for (int r = 0; r < NR; r++) if (need[p][r] > work[r]) ok = false;
            if (!ok) continue;
            for (int r = 0; r < NR; r++) work[r] += alloc[p][r];
            finish[p] = true; seq[count++] = p; progress = true;
            printf("  P%d can finish -> Work = [%d %d %d]\n", p, work[0], work[1], work[2]);
        }
    }
    if (count == NP) {
        printf("RESULT: SAFE state. Safe sequence: ");
        for (int i = 0; i < NP; i++) printf("P%d%s", seq[i], i == NP - 1 ? "\n" : " -> ");
        return 0;
    }
    printf("RESULT: UNSAFE state. Only %d of %d processes could finish; no safe sequence exists.\n", count, NP);
    return 0;
}

/* ------------------------------------------------------------------ */
/* FR6: Worker processes - fork() / waitpid() failure detection        */
/* ------------------------------------------------------------------ */

static void worker(bool fail) {
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

static void limit_demo(void) {
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
        for (;;) x++;                                         /* burn CPU until the kernel sends SIGXCPU */
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
static void cleanup_check(void) {
    int fd = shm_open(shm_name, O_RDWR, 0);
    if (fd >= 0) { close(fd); shm_unlink(shm_name); puts("Found stale shared-memory name from an earlier run: removed."); }
    sem_t *s = sem_open(sem_name, 0);
    if (s != SEM_FAILED) { sem_close(s); sem_unlink(sem_name); puts("Found stale semaphore name from an earlier run: removed."); }

    fd = shm_open(shm_name, O_RDWR, 0);
    if (fd >= 0) { close(fd); puts("Shared-memory name still exists!"); } else puts("Verified: no CampusCompute shared-memory name remains.");
    s = sem_open(sem_name, 0);
    if (s != SEM_FAILED) { sem_close(s); puts("Semaphore name still exists!"); } else puts("Verified: no CampusCompute semaphore name remains.");
    puts("Threads are joined and children are reaped (waitpid) inside each demonstration before it returns.");
}

/* S21: integrated run of every feature */
static void all(void) {
    puts("=== CampusCompute integrated demonstration ===");
    compare_all(NULL);
    logger();
    ipc_demo();
    banker_case(true);
    banker_case(false);
    worker(false);
    worker(true);
    limit_demo();
    cleanup_check();
    puts("\n=== Integrated run complete: scheduling, logger, IPC, Banker's, workers, limits, cleanup all executed ===");
}

/* ------------------------------------------------------------------ */
/* FR8: Command-line interface and error handling                      */
/* ------------------------------------------------------------------ */

int main(int argc, char **argv) {
    if (argc < 2) { usage(stderr); return 2; }
    const char *cmd = argv[1];

    if (!strcmp(cmd, "--help") || !strcmp(cmd, "help")) {
        if (argc != 2) { fprintf(stderr, "help takes no arguments\n"); return 2; }
        usage(stdout);
        return 0;
    }

    if (!strcmp(cmd, "fcfs") || !strcmp(cmd, "priority")) {
        if (argc > 3) { fprintf(stderr, "%s accepts at most one job file\n", cmd); return 2; }
        return schedule(cmd, argc == 3 ? argv[2] : NULL, RR_QUANTUM, NULL);
    }
    if (!strcmp(cmd, "rr")) {
        if (argc > 4) { fprintf(stderr, "rr accepts an optional quantum and job file\n"); return 2; }
        int quantum = RR_QUANTUM;
        const char *jobfile = NULL;
        if (argc >= 3) {
            char *end = NULL;
            errno = 0;
            long parsed = strtol(argv[2], &end, 10);
            bool is_integer = end != argv[2] && *end == '\0';
            if (is_integer) {
                if (errno == ERANGE || parsed < 1 || parsed > INT_MAX) {
                    fprintf(stderr, "Round Robin quantum must be a positive integer\n");
                    return 2;
                }
                quantum = (int)parsed;
                if (argc == 4) jobfile = argv[3];
            } else {
                if (argc == 4) { fprintf(stderr, "use: rr [quantum] [jobfile]\n"); return 2; }
                jobfile = argv[2];
            }
        }
        return schedule(cmd, jobfile, quantum, NULL);
    }
    if (!strcmp(cmd, "compare")) {
        if (argc > 3) { fprintf(stderr, "compare accepts at most one job file\n"); return 2; }
        return compare_all(argc == 3 ? argv[2] : NULL);
    }

    if (!strcmp(cmd, "banker")) {
        if (argc > 3) { fprintf(stderr, "banker accepts one optional case\n"); return 2; }
        const char *arg = argc == 3 ? argv[2] : NULL;
        if (!arg || !strcmp(arg, "safe")) return banker_case(true);
        if (!strcmp(arg, "unsafe")) return banker_case(false);
        fprintf(stderr, "banker expects 'safe' or 'unsafe', got '%s'\n", arg);
        return 2;
    }
    if (!strcmp(cmd, "worker")) {
        if (argc > 3) { fprintf(stderr, "worker accepts one optional mode\n"); return 2; }
        const char *arg = argc == 3 ? argv[2] : NULL;
        if (!arg || !strcmp(arg, "normal")) worker(false);
        else if (!strcmp(arg, "fail")) worker(true);
        else { fprintf(stderr, "worker expects 'normal' or 'fail', got '%s'\n", arg); return 2; }
        return 0;
    }

    /* remaining commands take no argument */
    if (!strcmp(cmd, "logger") || !strcmp(cmd, "ipc") || !strcmp(cmd, "limit") ||
        !strcmp(cmd, "cleanup") || !strcmp(cmd, "all")) {
        if (argc != 2) { fprintf(stderr, "'%s' takes no argument\n", cmd); return 2; }
        if (!strcmp(cmd, "logger")) logger();
        else if (!strcmp(cmd, "ipc")) ipc_demo();
        else if (!strcmp(cmd, "limit")) limit_demo();
        else if (!strcmp(cmd, "cleanup")) cleanup_check();
        else all();
        return 0;
    }

    fprintf(stderr, "Unknown command: %s\n", cmd);
    usage(stderr);
    return 2;
}
