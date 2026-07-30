# CMake Migration Design

This directory contains the authoritative design and execution record for the CMake migration.

## Documents

- [CMake Migration Design](CMakeMigration.md): goals, decisions, target organization, and migration phases.
- [Phase 0 - Parity Baseline](Phase0-ParityBaseline.md): MSBuild baseline, artifact contract, tests, and known failures.

## Document Convention

Each phase document records:

- Scope and completion criteria.
- Decisions made during the phase.
- Commands and environment used for verification.
- Results and known baseline failures.
- Work intentionally deferred to later phases.

Phase documents describe completed evidence separately from planned work. A baseline failure is not considered a CMake regression unless the corresponding MSBuild baseline succeeds under the same conditions.
