#include "campuscompute.h"
/* FR5: Banker's safety algorithm                                      */
/* ------------------------------------------------------------------ */

#define NP 5      /* processes */
#define NR 3      /* resource types */

/* safe=true uses the classic textbook Available; safe=false uses a smaller Available
 * ({1,1,0}) with the same Allocation/Maximum, so no process's Need can be met. */
int banker_case(bool safe) {
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
