# Decision 032: Reproducible Scaling and Release Evidence

Status: accepted  
Plan version: 1.0.15

## Decision

BLITZAR performance and release statements are generated from a versioned
workload contract rather than ad hoc command lines. A small internal scaling
harness calls the existing `Simulation` execution path and emits a stable
key-value record for every process. A Python runner owns expansion, command
execution, environment capture, rank aggregation, validation, and report
writing.

Strong scaling keeps the global particle count fixed while rank count changes.
Weak scaling keeps particles per rank fixed while the global count changes.
Migration and overlap are separate workload families because they validate
communication behavior rather than solver throughput. The Direct CPU solver is
the oracle for hierarchical CPU runs; GPU execution is reported separately from
CPU parity and fallback.

Generated logs and numeric tables live in an external evidence directory. The
repository versions only the workload contract, report schema, and procedure,
so a local build cannot silently become release evidence for another machine.

## Consequences

- Every result records command, revision, toolchain, topology, precision, seed,
  tolerance, backend, and rank scope.
- A multi-rank run on one host is never labelled multi-node.
- Missing `mpiexec`, HIP, CUDA, or a physical device is explicit evidence state,
  not an implicit pass.
- Raw rank logs remain available for diagnosing aggregation or parity errors.
