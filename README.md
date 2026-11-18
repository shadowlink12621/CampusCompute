# CampusCompute

Operating Systems (UE25CS242A) Level 3 mini-project. The C program is split into `main.c`, `scheduler.c`, `logger.c`, `ipc.c`, `banker.c`, and `workers.c`, with shared declarations in `campuscompute.h`. It demonstrates CPU scheduling, a POSIX-thread producer-consumer logger, POSIX shared-memory IPC coordinated with a named semaphore, Banker's safety analysis, child-worker status reporting, a CPU resource limit, and cleanup.

**Team:** Sridatta and Saideep (two members). Either teammate may be the single submitter; the required screenshot PDF filename uses that submitter's own full SRN.

## Prerequisites and build

The assignment specifies Linux/Ubuntu or WSL Ubuntu. On Ubuntu/WSL, install GCC, Make, zip, and unzip with `sudo apt install build-essential zip unzip`. On CachyOS, install the equivalent tools with `sudo pacman -S --needed base-devel zip unzip`. Check with the instructor whether CachyOS screenshots satisfy the course's Ubuntu/WSL wording.

```sh
make clean && make
./campuscompute help
```

The Makefile compiles each module as C11 with `-Wall -Wextra -Wpedantic` and links pthreads and librt. A successful build should show no warnings.

## Commands

```sh
./campuscompute help
./campuscompute fcfs [jobfile]
./campuscompute rr [quantum] [jobfile]
./campuscompute priority [jobfile]
./campuscompute compare [jobfile]
./campuscompute logger
./campuscompute ipc
./campuscompute banker safe
./campuscompute banker unsafe
./campuscompute worker normal
./campuscompute worker fail
./campuscompute limit
./campuscompute cleanup
./campuscompute all
```

Without a job file, the schedulers use the built-in repeatable workload. A job file has one record per line in this order: `job_id arrival_time cpu_burst priority`. Blank lines and lines beginning with `#` are ignored. IDs must be positive and unique; arrival and priority cannot be negative; burst must be positive; each data line must contain exactly four integers. `sample_jobs.txt` is the valid example; `bad_jobs.txt` intentionally has a negative burst.

FCFS uses earlier arrival, then lower job ID. Priority is non-preemptive; lower numeric priority runs first, then earlier arrival, then lower ID. Round Robin uses a FIFO ready queue with default quantum 3; pass a positive integer to change it. Arrivals during a time slice are queued before the unfinished job is requeued. Turnaround is `completion - arrival`; waiting is `turnaround - burst`.

The logger has two producers, one consumer, and a bounded queue of three events. A mutex protects queue state; condition variables make producers wait when the queue is full and the consumer wait when it is empty. It writes ordered events to `campuscompute.log` and joins the threads.

IPC uses one named shared-memory object and one named semaphore. The child writes a message and posts the semaphore; the parent waits, reads the message, reaps the child, and unlinks the two names it created. `cleanup` checks/removes only these two program-specific names if a previous interrupted run left them.

The Banker's demonstration calculates `Need = Maximum - Allocation`. Available `[3 3 2]` is a safe state with a valid sequence; Available `[1 1 0]` is deliberately unsafe. `worker fail` is a controlled simulation: the child exits with status 7; it is not an actual OS failure. `limit` sets `RLIMIT_CPU` in a child only and demonstrates the CPU-time limit; the parent process is unaffected. `all` runs the feature demonstrations together. `make clean` removes the executable and generated log.

## Live viva practice

Run Round Robin twice using the same jobs, changing only the quantum:

```sh
./campuscompute rr 3 sample_jobs.txt
./campuscompute rr 2 sample_jobs.txt
```

To add three jobs and run FCFS, create a separate small input file:

```sh
printf '1 0 5 2\n2 1 3 1\n3 2 2 3\n' > live_jobs.txt
./campuscompute fcfs live_jobs.txt
```

To add a job to the checked-in sample without changing it, copy the file and append one record:

```sh
cp sample_jobs.txt live_jobs.txt
printf '5 7 3 1\n' >> live_jobs.txt
./campuscompute fcfs live_jobs.txt
./campuscompute compare live_jobs.txt
```

Edit one priority in `live_jobs.txt` and rerun `./campuscompute priority live_jobs.txt` to demonstrate the changed order. Explain the output your own run produces.

## Required evidence: S01-S24

Set the shell prompt using the submitting student's actual full SRN. Do not leave the example placeholder in the prompt or evidence. For Bash:

```sh
export SRN='YOUR_ACTUAL_FULL_SRN'
export PS1='[$SRN] \w\$ '
```

For zsh, use `export SRN='YOUR_ACTUAL_FULL_SRN'; export PROMPT="[$SRN] %~ %# "`. For Fish, use:

```fish
set -gx SRN YOUR_ACTUAL_FULL_SRN
function fish_prompt; printf '[%s] %s > ' $SRN (prompt_pwd); end
```

The Fish prompt may shorten the displayed directory; use `pwd` to show the full path. Keep the same shell open. Capture genuine terminal screenshots showing the command, full SRN prompt, and meaningful output. Do not edit or fabricate output. Add the matching short caption below each screenshot and combine them in one PDF named exactly `ACTUALSRN.pdf`. Do not submit a separate project report. Optional viva preparation is separate and does not replace this screenshot PDF.

Run the commands one at a time from the project folder. For S19, these deliberately return errors; run each and retain the messages. S21 writes the actual run output to a temporary text file, then displays its beginning and ending to fit the evidence in one terminal view.

| ID  | Command                                                                                                                                                                                                                                    | Caption                                                                                                  |            |                                                |
| --- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ | -------------------------------------------------------------------------------------------------------- | ---------- | ---------------------------------------------- |
| S01 | `grep '^PRETTY_NAME=' /etc/os-release; pwd`                                                                                                                                                                               | S01 - Environment: The Linux terminal shows my full SRN prompt and project directory.                    |            |                                                |
| S02 | `gcc --version \| head -n 1; make --version \| head -n 1` | S02 - Build tools: GCC and Make are available. |
| S03 | `ls -la`                                                                                                                                                                                                                                   | S03 - Project files: The modules, header, Makefile, README, and job examples are present.                |            |                                                |
| S04 | `make clean && make`                                                                                                                                                                                                                       | S04 - Clean build: CampusCompute compiles from source without warnings.                                  |            |                                                |
| S05 | `./campuscompute help`                                                                                                                                                                                                                     | S05 - CLI help: The usage output lists the available project commands.                                   |            |                                                |
| S06 | `./campuscompute fcfs sample_jobs.txt`                                                                                                                                                                                                     | S06 - FCFS: The run shows its timeline and calculated scheduling metrics.                                |            |                                                |
| S07 | `./campuscompute rr 3 sample_jobs.txt`                                                                                                                                                                                                     | S07 - Round Robin: The run shows quantum 3, FIFO order, and scheduling metrics.                          |            |                                                |
| S08 | `./campuscompute priority sample_jobs.txt`                                                                                                                                                                                                 | S08 - Priority: The run shows the priority rule, tie-break, timeline, and metrics.                       |            |                                                |
| S09 | `./campuscompute compare sample_jobs.txt`                                                                                                                                                                                                  | S09 - Comparison: All three policies use the same jobs and report their averages.                        |            |                                                |
| S10 | `./campuscompute logger`                                                                                                                                                                                                                   | S10 - Producer-consumer: Producers and consumer exchange events through the bounded queue.               |            |                                                |
| S11 | `./campuscompute logger; tail -n 10 campuscompute.log`                                                                                                                                                                                     | S11 - Thread completion: The logger joins its threads and writes readable log entries.                   |            |                                                |
| S12 | `./campuscompute ipc`                                                                                                                                                                                                                      | S12 - Shared memory: The child writes a message that the parent reads.                                   |            |                                                |
| S13 | `./campuscompute ipc && ./campuscompute cleanup`                                                                                                                                                                                           | S13 - Semaphore and cleanup: The parent synchronizes with the child and verifies IPC cleanup.            |            |                                                |
| S14 | `./campuscompute banker safe`                                                                                                                                                                                                              | S14 - Safe state: Banker's algorithm prints a valid safe sequence.                                       |            |                                                |
| S15 | `./campuscompute banker unsafe`                                                                                                                                                                                                            | S15 - Unsafe state: Banker's algorithm reports that no safe sequence exists.                             |            |                                                |
| S16 | `./campuscompute worker normal`                                                                                                                                                                                                            | S16 - Normal worker: The parent reaps the child and reports status zero.                                 |            |                                                |
| S17 | `./campuscompute worker fail`                                                                                                                                                                                                              | S17 - Simulated failure: The parent detects the controlled exit status 7.                                |            |                                                |
| S18 | `./campuscompute limit`                                                                                                                                                                                                                    | S18 - Resource limit: The child reaches the CPU limit and the parent reports the result.                 |            |                                                |
| S19 | `./campuscompute bogus; ./campuscompute fcfs bad_jobs.txt; ./campuscompute rr 0`                                                                                                                                                           | S19 - Error handling: Invalid command, job data, and quantum produce useful errors.                      |            |                                                |
| S20 | `./campuscompute cleanup; if pgrep -ax campuscompute; then true; else echo 'No CampusCompute process remains'; end; find /dev/shm -maxdepth 1 -iname '*campuscompute*' -print`                                                                                 | S20 - Cleanup verification: The check reports no remaining program process or named IPC object.          |            |                                                |
| S21 | `./campuscompute all > /tmp/campuscompute_all.txt; printf '%s\n' '--- beginning ---'; head -n 8 /tmp/campuscompute_all.txt; printf '%s\n' '--- ending ---'; tail -n 12 /tmp/campuscompute_all.txt`                                         | S21 - Integrated run: The beginning and completion summary show the features running together.           |            |                                                |
| S22 | `ls -l campuscompute.log; head -n 15 campuscompute.log`                                                                                                                                                                                    | S22 - Generated log: The file contains ordered events written by the logger.                             |            |                                                |
| S23 | `ls -l main.c scheduler.c logger.c ipc.c banker.c workers.c campuscompute.h Makefile sample_jobs.txt bad_jobs.txt campuscompute`                                                                                                           | S23 - Final files: The modules, header, build files, job examples, and executable are present.           |            |                                                |
| S24 | `make clean && zip -r -FS "$SRN"_CampusCompute.zip Makefile campuscompute.h main.c scheduler.c logger.c ipc.c banker.c workers.c sample_jobs.txt bad_jobs.txt && unzip -l "$SRN"_CampusCompute.zip && unzip -t "$SRN"_CampusCompute.zip` | S24 - ZIP check: The archive contains code, build files, and job examples and passes its integrity test. |            |                                                |

S01-S24 follow the instructor's checklist. The assignment says Linux/Ubuntu or WSL Ubuntu, and its screenshot examples specifically mention Ubuntu/WSL; confirm CachyOS is accepted before capturing the final evidence. The course portal determines whether a ZIP is required and any required archive naming.

## Team split

See `TEAM_PLAN.md`. The two teammates can split initial run-throughs, but both must understand and explain every feature in the viva.
