# CampusCompute

OS mini-project (UE25CS242A, Level 3) in C: CPU scheduling, pthread producer/consumer logger,
POSIX shared memory + named semaphore IPC, Banker's safety algorithm, fork/waitpid worker
failure detection, and setrlimit resource limits.

## Prerequisites
Ubuntu / WSL Ubuntu with `gcc` and `make`:
    sudo apt update && sudo apt install -y build-essential

## Build
    make            # builds ./campuscompute
    make clean      # removes the executable and campuscompute.log

## Run
    ./campuscompute help
    ./campuscompute fcfs [jobfile]
    ./campuscompute rr [quantum] [jobfile]
    ./campuscompute priority [jobfile]
    ./campuscompute compare [jobfile]
    ./campuscompute logger
    ./campuscompute ipc
    ./campuscompute banker safe | unsafe
    ./campuscompute worker normal | fail
    ./campuscompute limit
    ./campuscompute cleanup
    ./campuscompute all          # integrated demonstration (or: make run)

### Job file format (optional)
One job per line: `id arrival burst priority`. Lines starting with `#` are ignored.
Lower priority number = more urgent. See `sample_jobs.txt`; `bad_jobs.txt` is deliberately invalid.

## Rules used
- FCFS and Round Robin order equal-time arrivals by lower job ID. Priority chooses the lowest numeric priority among arrived jobs, then earlier arrival and lower job ID.
- Round Robin default time quantum: 3; pass a positive integer to change it (for example, `./campuscompute rr 2`). It uses a FIFO ready queue; jobs that arrive during a slice are queued before the preempted job.
- Priority scheduling is non-preemptive.
- Worker failure is SIMULATED (child calls _exit(7)); it is not a real OS fault.
- Resource limit: RLIMIT_CPU soft limit of 1 CPU-second set in a child. Linux commonly enforces this with SIGXCPU; record the observed signal and timing from your VM.

## Cleanup
The program removes the IPC objects it creates (`/campuscompute_demo_shm`,
`/campuscompute_demo_sem`) itself. If a run was killed midway, run `./campuscompute cleanup`
(it only touches those two names). `make clean` removes the binary and log.

## Required submission evidence

Capture genuine screenshots from the team's Linux/Ubuntu VM. Every terminal screenshot must show the submitting student's complete actual SRN in the shell prompt, the command, and meaningful output. Add one short caption per screenshot and combine them into a single PDF named exactly with the submitting student's SRN (for example, `PES1UGxxxxxxx.pdf`). Do not fabricate or edit terminal output; trust the output observed on your VM. The assignment does not require a separate report. GitHub is optional and does not replace submission evidence.

| ID | Evidence |
|---|---|
| S01-S05 | SRN prompt/environment, GCC and Make, project files, clean build, help |
| S06-S09 | FCFS, Round Robin, Priority, and same-workload comparison |
| S10-S13 | Logger activity/log, thread completion, shared-memory IPC, semaphore and cleanup |
| S14-S15 | Banker's safe sequence and unsafe case |
| S16-S18 | Normal worker, simulated worker failure, CPU resource-limit result |
| S19-S20 | Invalid command/input and targeted cleanup verification |
| S21-S22 | Integrated run and generated log file |
| S23-S24 | Final project files/executable and submission ZIP listing/integrity check |

The full two-person task split is in `TEAM_PLAN.md`. The assignment PDFs supplied with the project contain the detailed screenshot checklist. Both teammates should run the program and be ready to explain their own observed results in the viva.

## Live demonstration practice

Run Round Robin with two quanta on the same jobs and compare the timelines:

```sh
./campuscompute rr 3 sample_jobs.txt
./campuscompute rr 2 sample_jobs.txt
```

To add a job for the FCFS practical, make a working copy of the sample file and append a valid record (ID, arrival, burst, priority):

```sh
cp sample_jobs.txt live_jobs.txt
printf '5 7 3 1\n' >> live_jobs.txt
./campuscompute fcfs live_jobs.txt
./campuscompute compare live_jobs.txt
```

Then change a priority value in `live_jobs.txt` with a text editor and run `./campuscompute priority live_jobs.txt` again. Explain the actual timeline and metrics printed by your VM. The original sample stays unchanged.
