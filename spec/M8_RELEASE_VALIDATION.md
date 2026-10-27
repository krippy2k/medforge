# M8 — Performance, verification and release

## Goal
Deliver a robust, reproducible interview demo and portfolio release.

## Prerequisites
M7 complete

## In scope
- Profiling, memory/error handling, project format, packaging, demo assets and documentation.

## Functional and technical requirements
- Versioned project persistence with atomic save and migration/error strategy.
- Profile import, reconstruction, segmentation and export on specified reference datasets.
- Add cancellation/retry/failure-path checks, resource lifetime auditing and sanitizers where supported.
- Package signed/notarized only if infrastructure available; otherwise unsigned release clearly labeled.
- Publish build instructions, dependency license notices, architecture diagram, screenshots and demo script.
- Include safe public synthetic/de-identified sample or documented download with license.
- Show research-only disclaimer in app and exports.

## UI quality requirements
- Audit the complete design system and interaction consistency across every screen; fix visual regressions.
- Execute screenshot review matrix on supported Windows display/DPI combinations and document Linux differences if tested.
- Verify keyboard navigation, contrast, panel persistence, focus, loading/error/empty states, and accessible names.
- Package a reproducible UI evidence gallery and list any NOT VERIFIED environments explicitly.

## Verification and tests
- Clean install/uninstall smoke test.
- Fresh-machine or clean-VM build and demo rehearsal.
- Project save/reopen/export regression tests.
- Memory/resource leak checks and documented benchmark results.

## Acceptance / exit gate
- End-to-end demo: import → MPR → segment → mesh → measure → export.
- Release package and repeatable benchmark/test report available.

## Explicit exclusions
Do not implement features belonging to later milestones. No clinical-use claims, diagnosis, or treatment recommendations.

## Delivery artifacts
- Compiling source and updated dependency manifest.
- Unit/integration tests and CI results.
- `docs/M8-verification.md` with commands, test evidence, known limitations.
- A concise screen recording or reproducible demo procedure.
