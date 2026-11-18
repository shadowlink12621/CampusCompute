#include "campuscompute.h"
/* FR4: IPC - POSIX shared memory + named semaphore                    */
/* ------------------------------------------------------------------ */

typedef struct { char text[128]; } Shared;

int ipc_demo(void) {
    int fd = -1;
    int result = 0;
    Shared *s = MAP_FAILED;
    sem_t *sem = SEM_FAILED;
    bool shm_created = false, sem_created = false;

    /* O_EXCL: fail if the name already exists, so we never touch someone else's object */
    fd = shm_open(SHM_NAME, O_CREAT | O_EXCL | O_RDWR, 0600);
    if (fd < 0) { perror("shm_open (stale name? run './campuscompute cleanup')"); return 1; }
    shm_created = true;
    if (ftruncate(fd, sizeof(Shared)) < 0) { perror("ftruncate"); goto done; }
    s = mmap(NULL, sizeof(Shared), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (s == MAP_FAILED) { perror("mmap"); goto done; }
    sem = sem_open(SEM_NAME, O_CREAT | O_EXCL, 0600, 0);    /* initial value 0: parent must wait */
    if (sem == SEM_FAILED) { perror("sem_open"); goto done; }
    sem_created = true;
    printf("Parent: created shared memory %s and semaphore %s (value 0).\n", SHM_NAME, SEM_NAME);

    fflush(stdout);                                          /* avoid child duplicating buffered output */
    pid_t pid = fork();
    if (pid < 0) { perror("fork"); goto done; }
    if (pid == 0) {                                          /* child = producer of the message */
        snprintf(s->text, sizeof s->text, "job J42 completed by child PID %ld", (long)getpid());
        printf("Child : wrote message to shared memory, posting semaphore.\n");
        fflush(stdout);
        if (sem_post(sem) < 0) { perror("sem_post"); _exit(3); } /* signal: data is ready */
        _exit(0);
    }
    printf("Parent: waiting on semaphore for child's data...\n");
    int wait_result;
    do { wait_result = sem_wait(sem); } while (wait_result < 0 && errno == EINTR);
    if (wait_result < 0) { perror("sem_wait"); result = 1; }
    else printf("Parent: received shared-memory message: %s\n", s->text);
    int child_status;
    pid_t waited;
    do { waited = waitpid(pid, &child_status, 0); } while (waited < 0 && errno == EINTR);
    if (waited < 0) { perror("waitpid"); result = 1; }
    else if (!WIFEXITED(child_status) || WEXITSTATUS(child_status) != 0) {
        fprintf(stderr, "IPC child did not finish successfully.\n"); result = 1;
    }

done:
    if (s != MAP_FAILED) munmap(s, sizeof(Shared));
    if (fd >= 0) close(fd);
    if (sem != SEM_FAILED) sem_close(sem);
    if (shm_created) shm_unlink(SHM_NAME);                   /* remove ONLY what we created */
    if (sem_created) sem_unlink(SEM_NAME);
    puts("Resources created by this run cleaned up (munmap, close, shm_unlink, sem_unlink).");
    return result;
}

/* ------------------------------------------------------------------ */
