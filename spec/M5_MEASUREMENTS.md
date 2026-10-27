# M5 — Measurements and anatomical landmarks

## Goal
Make reconstructed anatomy useful for reproducible engineering measurements.

## Prerequisites
M4 complete

## In scope
- 3D landmarks, point distances, angles, cross-sectional planes and reports.

## Functional and technical requirements
- Place, select, move and delete landmarks in 2D/3D with stable identifiers.
- Implement Euclidean distance (mm), three-point angle (degrees), and plane-based cross-section area (mm²).
- Support optional surface snapping with explicit tolerance and fallback.
- Show annotated overlays, selected entity properties and measurement table.
- Store measurement definitions and derived values separately; recalculate when geometry changes.
- Export CSV and human-readable report with units, dataset provenance, research-only disclaimer.
- Define measurement uncertainty limitations: CT resolution, segmentation and landmark placement.

## UI quality requirements
- Test measurement tool activation, units, selection, edit/delete, undo/redo, and report export before UI implementation.
- Inspect annotation legibility over light/dark scan content, inspector precision, keyboard editing, and narrow window layouts.
- Keep research-only and unvalidated measurement notices clear without obstructing the workflow.

## Verification and tests
- Golden tests for distances, angles and planar areas on synthetic geometry.
- Coordinate-transform invariance tests.
- Save/reload landmarks and verify derived values.
- Manual 2D↔3D alignment test.

## Acceptance / exit gate
- Demonstrate landmark placement and at least three measurements on a bone mesh.
- Reports export and reload with correct units and references.

## Explicit exclusions
Do not implement features belonging to later milestones. No clinical-use claims, diagnosis, or treatment recommendations.

## Delivery artifacts
- Compiling source and updated dependency manifest.
- Unit/integration tests and CI results.
- `docs/M5-verification.md` with commands, test evidence, known limitations.
- A concise screen recording or reproducible demo procedure.
