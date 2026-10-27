# M7 — Experimental guide and implant CAD planning

## Goal
Demonstrate CAD operations and 3D placement without suggesting clinical suitability.

## Prerequisites
M6 complete

## In scope
- Imported implant CAD models, 6-DOF transforms, basic clearance, prototype guide geometry.

## Functional and technical requirements
- Add Open CASCADE behind an isolated CAD adapter with documented supported STEP/IGES formats.
- Import CAD implant, display with anatomy, edit transform numerically and via 3D gizmo.
- Calculate visual overlap and approximate clearance with method/tolerance disclosure.
- Prototype guide workflow: select contact patch, create offset shell, add cylindrical hole or slot, preview Boolean result.
- Validate watertightness/intersection failures and reject failed geometry operations.
- Export prototype STEP when geometry is a valid B-Rep, STL otherwise; do not imply arbitrary triangle mesh→exact STEP conversion.
- Watermark outputs research-only / not-for-manufacture disclaimer.

## UI quality requirements
- Test tool selection, parameter validation, transforms, preview/apply/revert, and warnings before implementation.
- Inspect drill/guide/implant overlays, dimension labels, invalid-geometry warnings, and editing affordances.
- Clearly identify prototypes as non-clinical and not suitable for manufacturing or patient use without independent validation.

## Verification and tests
- CAD primitive Boolean and transform unit tests.
- Import/export known STEP solids and compare bounds/volume.
- Failure tests for invalid geometry and unsatisfied Boolean operations.
- Repeatability of guide construction from stored parameters.

## Acceptance / exit gate
- Demonstrate positioning an implant or generating a simple guide on synthetic anatomy.
- Exported artifact can be reopened and passes geometric sanity checks.

## Explicit exclusions
Do not implement features belonging to later milestones. No clinical-use claims, diagnosis, or treatment recommendations.

## Delivery artifacts
- Compiling source and updated dependency manifest.
- Unit/integration tests and CI results.
- `docs/M7-verification.md` with commands, test evidence, known limitations.
- A concise screen recording or reproducible demo procedure.
