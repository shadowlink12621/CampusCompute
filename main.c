#include "campuscompute.h"
void usage(FILE *f) {
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

/* S21: integrated run of every feature */
static int all(void) {
    puts("=== CampusCompute integrated demonstration ===");
    int result = compare_all(NULL);
    result |= logger();
    result |= ipc_demo();
    result |= banker_case(true);
    result |= banker_case(false);
    worker(false);
    worker(true);
    limit_demo();
    result |= cleanup_check();
    puts(result ? "\n=== Integrated run finished with one or more errors ==="
                : "\n=== Integrated run complete: scheduling, logger, IPC, Banker's, workers, limits, cleanup all executed ===");
    return result;
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
        if (!strcmp(cmd, "logger")) return logger();
        else if (!strcmp(cmd, "ipc")) return ipc_demo();
        else if (!strcmp(cmd, "limit")) limit_demo();
        else if (!strcmp(cmd, "cleanup")) return cleanup_check();
        else return all();
        return 0;
    }

    fprintf(stderr, "Unknown command: %s\n", cmd);
    usage(stderr);
    return 2;
}
