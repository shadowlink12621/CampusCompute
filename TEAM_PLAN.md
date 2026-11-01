# CampusCompute Team Plan

**Course:** Operating Systems (UE25CS242A), Level 3 Mini-Project  
**Project:** CampusCompute: Intelligent Job Scheduling and Resource Management  
**Team:** Sridatta and Saideep  
**Target:** Finish the implementation checks and submission evidence today.

## Work completed

- Read the two supplied assignment PDFs and identified the required features and S01–S24 evidence checklist.
- Implemented the CampusCompute C command-line program: FCFS, Round Robin, non-preemptive Priority, producer-consumer logging, shared-memory IPC with a named semaphore, Banker's safe/unsafe analysis, worker exit-status reporting, CPU resource limit, cleanup checks, and command help.
- Added a Linux Makefile, .gitignore, README with build/run commands and the screenshot checklist, and a source ZIP.
- Created the public GitHub repository: https://github.com/shadowlink12621/CampusCompute.
- Added CampusCompute.c, Makefile, and .gitignore to the repository. This team plan is being added now.

## Ownership for the remaining work

### Sridatta — scheduling, CLI, integration, and evidence assembly

- On the Linux VM, clone the repository and build with make clean && make.
- Verify --help, FCFS, Round Robin (quantum 3), Priority (lower number wins), policy comparison, malformed command handling, and reported metrics.
- Check that jobs finish and that the same workload is used for policy comparison.
- Capture S01–S09 and S19, showing the complete real SRN in the terminal prompt.
- Collect both teammates' screenshots and combine the genuine screenshots and one-line captions into the single SRN-named PDF.
- Prepare the final source/build ZIP and verify its contents and integrity (S24).

### Saideep — concurrency, IPC, safety, workers, and resource limit

- Independently review and run the producer-consumer logger; confirm both producers' events are processed, threads join, and campuscompute.log is readable.
- Run shared-memory IPC and confirm the child-to-parent message uses the named semaphore and resources are cleaned up.
- Run Banker's safe and unsafe cases and verify the reported safe sequence and unsafe result.
- Run normal and simulated-failure workers; confirm the parent reaps both and reports their statuses accurately.
- Run the CPU resource-limit demonstration and cleanup check; note the observed signal/status on this Linux VM.
- Capture S10–S18 and S22, showing the complete real SRN in the terminal prompt.

## Shared tasks to finish today

1. **Coordinate the VM and repository.** Sridatta adds Saideep as a GitHub collaborator; Saideep accepts the invitation and clones the repo. Keep the local project folder as a fallback if access takes time.
2. **Build and verify first.** Run make clean && make, then each assigned command from the README. Fix any actual compiler/runtime defects and push the fixes before capturing final evidence.
3. **Capture genuine screenshots.** Both teammates use their own full SRN in their terminal prompt and capture only real output. Number screenshots once, S01–S24, and write one short explanation per screenshot. Do not edit/fabricate terminal evidence.
4. **Integrated run and final review.** Together run ./campuscompute all, inspect the generated log, run ./campuscompute cleanup, review all screenshots, and ensure no assigned feature is missing.
5. **Package submission.** Make one PDF named exactly with the submitting student's SRN, containing the numbered screenshots and captions. Make a ZIP with source/build files if the portal requires it, then check archive integrity. Do not create a separate report unless the instructor/portal asks for one.
6. **Viva preparation.** Each teammate explains their assigned areas and both review the full program together: scheduling metrics, synchronization, shared memory/semaphore, Banker's Need calculation, worker status, and resource limits.

## Evidence checklist

- [ ] S01 Ubuntu/WSL, full SRN prompt, working directory
- [ ] S02 GCC and Make versions
- [ ] S03 project files and Makefile
- [ ] S04 clean build
- [ ] S05 command help
- [ ] S06 FCFS metrics
- [ ] S07 Round Robin, quantum, and metrics
- [ ] S08 Priority scheduling and tie-breaking
- [ ] S09 policy comparison on the same jobs
- [ ] S10 producer-consumer activity and log
- [ ] S11 synchronized thread completion
- [ ] S12 shared-memory message exchange
- [ ] S13 semaphore coordination and IPC cleanup
- [ ] S14 Banker's safe state and sequence
- [ ] S15 Banker's unsafe state
- [ ] S16 normal worker completion
- [ ] S17 controlled simulated worker failure
- [ ] S18 CPU resource limit and observed result
- [ ] S19 invalid command and useful usage message
- [ ] S20 cleanup verification
- [ ] S21 integrated execution and summary
- [ ] S22 generated log sample
- [ ] S23 final source, Makefile, and executable
- [ ] S24 source ZIP listing and successful integrity check
- [ ] One PDF named with the submitter's actual SRN; all screenshots have one-line captions
- [ ] Submission ZIP prepared if required by the portal

## Notes

- GitHub is a collaboration/download convenience, not a course submission requirement.
- The source is implemented, but it has not yet been built or run on the target Linux VM. Treat all demonstrations as PENDING VERIFICATION until the team observes them there.
- The final SRN-named PDF cannot be completed until the team captures the required genuine screenshots. Do not invent SRNs or screenshots.
- Only Sridatta is handling the GitHub collaborator invitation; no repository permissions are changed by this plan.
