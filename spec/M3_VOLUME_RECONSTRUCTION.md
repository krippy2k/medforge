# M3 — Volume rendering and surface reconstruction

## Goal
Visualize 3D CT data and extract an initial anatomy mesh.

## Prerequisites
M2 complete

## In scope
- Volume rendering, transfer-function presets, clipping, isosurface extraction.

## Functional and technical requirements
- Add 3D volume ray casting through VTK GPU pipeline with CPU fallback/error messaging.
- Define opacity/color transfer functions for CT intensities.
- Generate isosurface mesh via Flying Edges or Marching Cubes.
- Correct physical-space mesh placement and anisotropic spacing.
- Add clip plane, camera presets, visibility toggles and mesh statistics.
- Compute asynchronously; expose memory and timing telemetry.

## UI quality requirements
- Test visibility toggles, preset selection, processing progress/cancellation, and 3D camera controls first.
- Visually inspect volume, mesh, loading, and error states, including resizing and DPI changes.
- Use deterministic synthetic volumes for geometry regression; do not substitute screenshot similarity for numerical correctness.

## Verification and tests
- Synthetic sphere phantom yields expected approximate radius/volume.
- Check mesh coordinates and normals for anisotropic voxel spacing.
- Stress test cancellation and repeated dataset reload.

## Acceptance / exit gate
- User can inspect CT volume and create/export-previewable 3D anatomy surface.
- No coordinate shift between 2D and 3D views.

## Explicit exclusions
Do not implement features belonging to later milestones. No clinical-use claims, diagnosis, or treatment recommendations.

## Delivery artifacts
- Compiling source and updated dependency manifest.
- Unit/integration tests and CI results.
- `docs/M3-verification.md` with commands, test evidence, known limitations.
- A concise screen recording or reproducible demo procedure.
