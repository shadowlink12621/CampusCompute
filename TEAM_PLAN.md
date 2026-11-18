# CampusCompute Team Plan

**Course:** Operating Systems (UE25CS242A), Level 3 Mini-Project  
**Team:** Sridatta and Saideep (exactly two students)

## Current version

The submission copy is a single-file C implementation in `CampusCompute.c`, built by the Makefile. It accepts job input files, validates records, implements FCFS/Round Robin/non-preemptive Priority, and includes the bounded logger, shared-memory/semaphore IPC, Banker's safety check, worker status checks, CPU limit, cleanup, and integrated `all` command. `sample_jobs.txt` is valid input; `bad_jobs.txt` is a deliberate validation case.

The code has been rebuilt and all documented feature paths have been run in the local Linux environment. Each teammate must rebuild and confirm behavior on their own Linux system; their own terminal output is the source for screenshots and any platform-specific observations.

## Work split

### Sridatta: scheduling, build, and evidence assembly

- Build from a clean state and verify GCC/Make, CLI help, FCFS, both Round Robin quanta, Priority, comparison, and invalid input.
- Rehearse creating a three-job input file and running FCFS. Rehearse adding a job and changing a priority in a working copy.
- Capture genuine S01-S09 and S19 screenshots with the submitting student's complete actual SRN in the prompt.
- Assemble the S01-S24 screenshot and caption evidence into the required SRN-named PDF. Check screenshot numbering, legibility, and filename.

### Saideep: synchronization, IPC, and process/resource behavior

- Review and run the producer-consumer logger, generated log, shared-memory/semaphore exchange, and cleanup check.
- Run both Banker's cases, both worker modes, CPU-limit demonstration, and integrated `all` run.
- Capture genuine S10-S18 and S20-S22 screenshots with the team's submitter SRN visible if sharing evidence. Send original screenshots to Sridatta without editing terminal output.

## Shared responsibilities and remaining work

- Both teammates must understand the full implementation, including each other's assigned demonstrations, and practice the teacher's 25 viva questions and live coding tasks.
- Both should independently rebuild and run the program. If their outputs or signal behavior differ from this environment, explain the results they observe on their own system.
- Confirm with the instructor whether CachyOS terminal evidence is accepted because the assignment says Linux/Ubuntu or WSL Ubuntu and its screenshot checklist specifies Ubuntu/WSL.
- Capture every required S01-S24 screenshot directly from the team's own terminal. Put one short explanation under each screenshot in a single PDF named exactly with the submitting student's full SRN. Do not fabricate screenshots or submit a separate report.
- Check whether the course portal requires the source ZIP and follow its archive naming/upload rules. The README includes an S24 archive integrity command.
- One teammate submits the team's copy; both retain it and prepare for the viva.

## Live-demonstration commands

```sh
./campuscompute rr 3 sample_jobs.txt
./campuscompute rr 2 sample_jobs.txt
printf '1 0 5 2\n2 1 3 1\n3 2 2 3\n' > live_jobs.txt
./campuscompute fcfs live_jobs.txt
cp sample_jobs.txt live_jobs.txt
printf '5 7 3 1\n' >> live_jobs.txt
./campuscompute priority live_jobs.txt
./campuscompute compare live_jobs.txt
./campuscompute logger
./campuscompute banker safe
./campuscompute banker unsafe
./campuscompute worker fail
```

## Checklist

- [x] Read the assignment and identify all functional and screenshot requirements.
- [x] Prepare the submission copy with job-file input and runtime Round Robin quantum.
- [x] Update README and this plan to describe the exact submission code.
- [x] Build with warning flags and run all documented feature paths locally.
- [ ] Practice the teacher's viva questions together during the live review tomorrow.
- [ ] Both teammates repeat the build and feature checks on their own system.
- [ ] Capture genuine S01-S24 screenshots and assemble the required SRN-named evidence PDF.
- [ ] Verify the source ZIP if required by the course portal.
- [ ] Both teammates rehearse explaining and demonstrating all features.
