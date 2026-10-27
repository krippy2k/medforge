# M6 — Mesh processing, repair and export

## Goal
Prepare reliable triangle meshes for downstream CAD workflows.

## Prerequisites
M5 complete

## In scope
- Topology inspection, smoothing, hole handling, decimation, STL export.

## Functional and technical requirements
- Report mesh bounds, surface area, volume when watertight, component count, open/non-manifold edges.
- Add smoothing and decimation as previewable, non-destructive operations.
- Implement bounded small-hole filling with preview; refuse unsafe repairs rather than silently altering anatomy.
- Preserve coordinate system, scale and provenance; explicitly label STL units as millimeters in UI/report.
- Support binary STL export and reimport validation; optional OBJ/PLY.
- Maintain undo/redo and cancellation for heavy mesh operations.

## UI quality requirements
- Test repair preview/apply/cancel, mesh diagnostics, export options, and error states first.
- Inspect before/after geometry presentation, long-operation progress, and export feedback.
- Ensure repair is reversible and does not silently replace source meshes.

## Verification and tests
- Closed cube/sphere and intentionally damaged mesh fixtures.
- Compare surface area/volume to analytical geometry within tolerance.
- STL export/reimport preserves bounding box and geometry within tolerance.
- Invalid mesh detection tests and cancellation tests.

## Acceptance / exit gate
- User can inspect, repair a known test defect, and export a validated mesh.
- No silent geometry changes or unit scaling errors.

## Explicit exclusions
Do not implement features belonging to later milestones. No clinical-use claims, diagnosis, or treatment recommendations.

## Delivery artifacts
- Compiling source and updated dependency manifest.
- Unit/integration tests and CI results.
- `docs/M6-verification.md` with commands, test evidence, known limitations.
- A concise screen recording or reproducible demo procedure.
