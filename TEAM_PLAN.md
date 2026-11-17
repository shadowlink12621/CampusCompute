# CampusCompute Team Plan

**Course:** Operating Systems (UE25CS242A), Level 3 Mini-Project  
**Team:** Sridatta and Saideep  
**Target:** Finish the Linux verification and submission evidence today.

## Current project state

- The supplied updated archive has been unpacked into this project folder. It includes `CampusCompute.c`, `Makefile`, `README.md`, `sample_jobs.txt`, and `bad_jobs.txt`.
- The updated implementation is reported as compiled and tested by the person who prepared the archive. Both teammates still need to build and run it in the Linux VM and use their actual output for evidence.
- The code covers the required schedulers (FCFS, Round Robin, Priority), input validation, pthread bounded-queue logger, shared-memory/semaphore IPC, Banker's safe/unsafe cases, worker exit-status reporting, CPU resource limit, cleanup, CLI, and integrated run.
- The assignment PDFs require genuine terminal screenshots S01-S24 with the submitting student's complete SRN visible, one explanatory sentence per screenshot, and one screenshot PDF named with that student's SRN. Do not fabricate or edit terminal evidence. A separate report is not required.
- A viva study PDF is separate optional preparation material; it does not replace the required screenshot PDF.

## Ownership for today's work

### Sridatta — build, scheduling, and submission assembly

1. Clone/pull the public repository in the Linux VM; check that `sample_jobs.txt` and `bad_jobs.txt` are present.
2. Run `make clean && make`; resolve any Linux-specific build issues with Saideep before screenshots.
3. Run and verify `help`, FCFS, Round Robin, Priority, `compare`, and invalid command/input behavior. Confirm metrics from actual output rather than relying only on expected values.
4. Capture S01-S09 and S19 with the complete real SRN prompt. S03 should show files before build; S23 should show the final files and executable.
5. Collect Saideep's screenshots, add one-line captions, assemble the required SRN-named evidence PDF, and prepare/check the final source ZIP for submission.

### Saideep — concurrency, IPC, resource safety, and process checks

1. Independently inspect the scheduling results and review the changed code with Sridatta.
2. Run the logger twice as needed; verify the bounded queue behavior, log file, and joined threads.
3. Run IPC and verify the shared-memory message, semaphore coordination, and that only CampusCompute's named resources are cleaned.
4. Run Banker's safe and unsafe cases; verify normal and simulated-failure workers; run the CPU-limit demonstration and cleanup checks.
5. Capture S10-S18 and S20-S22 with the complete real SRN prompt. For S22, show the generated log file and a sample.
6. Send original screenshots and observed output notes to Sridatta; do not alter screenshots.

## Shared final checks

- Both teammates independently run `./campuscompute all` (capture the beginning/end as required for S21) and explain the output to each other.
- Each teammate should be ready to explain every module, not only their assigned tasks. Rehearse adding a job to a copied job file, changing the RR quantum at runtime, changing a priority, and explaining the resulting output.
- Live-demo commands: `./campuscompute rr 3 sample_jobs.txt`, `./campuscompute rr 2 sample_jobs.txt`, and `./campuscompute compare sample_jobs.txt`.
- To add a live job without changing the checked-in sample, copy `sample_jobs.txt` to `live_jobs.txt`, append a valid record such as `5 7 3 1`, then run FCFS/compare with that file.
- Confirm S01-S24 are all present once, readable, genuine, and captioned. S24 should show archive listing and a successful integrity check.
- Ensure `make clean` is run before creating the submission ZIP if the instructor expects source/build files without generated binary/log output.
- Both teammates review the final evidence PDF and ZIP. One designated teammate submits the team's copy; both retain a copy and prepare for viva.

## Status checklist

- [x] Read the assignment and identify required features and evidence.
- [x] Prepare the updated source, Makefile, README, and job-file examples.
- [x] Team plan is published in the public GitHub repository.
- [ ] Replace the older source/README in GitHub with this updated package and add `sample_jobs.txt` and `bad_jobs.txt`; verify the files are visible.
- [ ] Build the repository version in Linux and resolve any discrepancies.
- [ ] Run every required command and capture authentic S01-S24 evidence.
- [ ] Assemble, inspect, and name the screenshot PDF using the submitting student's actual SRN.
- [ ] Create and integrity-check the final submission ZIP.
- [ ] Both teammates rehearse the viva using the implementation and their own outputs.

## Linux quick start

```sh
git clone https://github.com/shadowlink12621/CampusCompute.git
cd CampusCompute
make clean && make
./campuscompute --help
```

Before screenshots, set the terminal prompt with the student's own full SRN as directed in `README.md`. The exact output and observed Linux behavior are the evidence of record.
