# CMake Migration Design

This directory contains the authoritative design and execution record for the CMake migration.

## Documents

- [CMake Migration Design](CMakeMigration.md): goals, decisions, target organization, and migration phases.
- [Phase 0 - Parity Baseline](Phase0-ParityBaseline.md): MSBuild baseline, artifact contract, tests, and known failures.
- [Phase 1 - CMake Infrastructure](Phase1-CMakeInfrastructure.md): root project, presets, options, shared policies, and verification.
- [Phase 2 - Foundation and Standalone Targets](Phase2-FoundationAndStandalone.md): foundation split, console, provider, tools, and multi-architecture verification.
- [Phase 3 - Service and SDK](Phase3-ServiceAndSdk.md): service split, Interprocess, ControlLib, wrappers, middleware, API DLLs, and Core.
- [Phase 4 - UI and Capture](Phase4-UiAndCapture.md): CEF, shaders, PresentMonUI, KernelProcess, and the remaining payload, deployment, and signing work.

## Document Convention

Each phase document records:

- Scope and completion criteria.
- Decisions made during the phase.
- Commands, expected behavior, and environment used for build, artifact, and
  phase-local behavioral verification.
- Verification results and known baseline failures.
- Work intentionally deferred to later phases.

Phase documents keep canonical current procedures and results. Raw command logs
and superseded implementation history belong in Git history.

Phase documents describe completed evidence separately from planned work. A baseline failure is not considered a CMake regression unless the corresponding MSBuild baseline succeeds under the same conditions.
