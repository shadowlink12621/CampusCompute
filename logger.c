#include "campuscompute.h"
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

int logger(void) {
    logfile = fopen(LOG_PATH, "w");
    if (!logfile) { perror("open log"); return 1; }
    pthread_t p[2], started_threads[2], c;
    qhead = qtail = qcount = log_seq = 0;
    producers_left = 2;
    int rc = pthread_create(&c, NULL, consumer, NULL);
    if (rc) { fprintf(stderr, "pthread_create consumer: %s\n", strerror(rc)); fclose(logfile); logfile = NULL; return 1; }
    int started = 0;
    int result = 0;
    for (long i = 0; i < 2; i++) {
        rc = pthread_create(&p[i], NULL, producer, (void *)(i + 1));
        if (rc) {   /* a producer failed to start: account for it so the consumer can still finish */
            fprintf(stderr, "pthread_create producer: %s\n", strerror(rc));
            pthread_mutex_lock(&qlock); producers_left--; pthread_cond_broadcast(&qnotempty); pthread_mutex_unlock(&qlock);
        } else started_threads[started++] = p[i];
    }
    for (int i = 0; i < started; i++) {
        rc = pthread_join(started_threads[i], NULL);
        if (rc) { fprintf(stderr, "pthread_join producer: %s\n", strerror(rc)); result = 1; }
    }
    rc = pthread_join(c, NULL);
    if (rc) { fprintf(stderr, "pthread_join consumer: %s\n", strerror(rc)); result = 1; }
    pthread_mutex_destroy(&qlock); pthread_cond_destroy(&qnotempty); pthread_cond_destroy(&qnotfull);
    if (fclose(logfile) != 0) { perror("close log"); result = 1; }
    logfile = NULL;
    printf("All producer and consumer threads joined; log written to %s.\n", LOG_PATH);
    return result;
}

/* ------------------------------------------------------------------ */
