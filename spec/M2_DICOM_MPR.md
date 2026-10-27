# M2 — DICOM import and multiplanar viewer

## Goal
Load CT series and navigate three synchronized orthogonal slice views in physical coordinates.

## Prerequisites
M1 complete

## In scope
- Local file/folder import, series discovery, axial/sagittal/coronal views.
- DICOM metadata validation and basic HU display.

## Functional and technical requirements
- Select series using SeriesInstanceUID and geometry consistency checks.
- Decode supported uncompressed CT and explicitly enumerate supported compressed transfer syntaxes.
- Apply rescale slope/intercept to HU where applicable; support window/level presets and manual controls.
- Construct voxel-to-LPS-mm transform using image position, orientation and spacing.
- Show linked crosshair, scroll navigation, cursor coordinates and physical position.
- Handle anisotropic spacing, oblique orientations, missing/duplicate slices with warnings or rejection.
- Run import asynchronously, with progress and cancellation.

## UI quality requirements
- Define and test empty study, import progress, invalid series, and populated 2x2 MPR workspace states.
- Test slice synchronization, orientation labels, keyboard navigation, and window/level control behavior before implementation.
- Inspect screenshots for correct labels, usable contrast, viewport balance, and clipping across target resolutions.

## Verification and tests
- Synthetic DICOM phantom: known HU values, spacing and orientation.
- Round-trip voxel↔physical transform within documented tolerance.
- Reject malformed and inconsistent series gracefully.
- Check crosshair consistency across all views.

## Acceptance / exit gate
- Import a documented de-identified CT series.
- Three views navigate correctly with linked crosshairs and accurate units.
- UI stays responsive during import.

## Explicit exclusions
Do not implement features belonging to later milestones. No clinical-use claims, diagnosis, or treatment recommendations.

## Delivery artifacts
- Compiling source and updated dependency manifest.
- Unit/integration tests and CI results.
- `docs/M2-verification.md` with commands, test evidence, known limitations.
- A concise screen recording or reproducible demo procedure.
