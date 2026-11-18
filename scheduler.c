#include "campuscompute.h"
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


/* Sort by arrival time; ties broken by lower job ID */
static int cmp_arrival(const void *a, const void *b) {
    const Job *x = a, *y = b;
    if (x->arrival != y->arrival) return x->arrival < y->arrival ? -1 : 1;
    if (x->id != y->id) return x->id < y->id ? -1 : 1;
    return 0;
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
int schedule(const char *mode, const char *jobfile, int quantum, Summary *sum) {
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
    for (size_t i = 0; i < n; i++) { j[i].remaining = j[i].burst; j[i].status = ST_NEW; }

    char timeline[2048] = "";
    int t = 0;

    if (!strcmp(mode, "fcfs") || !strcmp(mode, "priority")) {
        bool use_prio = !strcmp(mode, "priority");
        bool done[MAX_JOBS] = {0};
        for (size_t k = 0; k < n; k++) {
            /* choose the best job among those that have already arrived */
            int pick = -1;
            for (size_t i = 0; i < n; i++)
                if (!done[i] && j[i].arrival <= t) j[i].status = ST_READY;
            for (size_t i = 0; i < n; i++)
                if (j[i].status == ST_READY && (pick < 0 || runs_before(&j[i], &j[pick], use_prio)))
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
            while (next < n && j[next].arrival <= t) {
                j[next].status = ST_READY;
                ready[(rh + rc++) % MAX_JOBS] = (int)next++;
            }
            if (rc == 0) { t = j[next].arrival; continue; }     /* idle: jump to next arrival */
            int i = ready[rh]; rh = (rh + 1) % MAX_JOBS; rc--;
            int run = j[i].remaining < quantum ? j[i].remaining : quantum;
            int start = t;
            j[i].status = ST_RUNNING;
            t += run; j[i].remaining -= run;
            timeline_add(timeline, sizeof timeline, j[i].id, start, t);
            while (next < n && j[next].arrival <= t) {
                j[next].status = ST_READY;
                ready[(rh + rc++) % MAX_JOBS] = (int)next++;
            }
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
int compare_all(const char *jobfile) {
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
