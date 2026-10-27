# M4 — Interactive segmentation

## Goal
Produce editable labelmaps for isolated anatomy and derived meshes.

## Prerequisites
M3 complete

## In scope
- Threshold, connected-region selection, paint/erase, undo/redo, mask overlays.

## Functional and technical requirements
- Implement labeled voxel masks as first-class document assets.
- Threshold tool with HU range and live preview.
- Connected component / seed-based region growing; prevent leakage by selectable connectivity and bounds.
- Brush painting and erasing on 2D slices; label overlays in all MPR views.
- Non-destructive editing history, tool parameter persistence and cancelable processing.
- Rebuild mesh from current labelmap and maintain source provenance.
- Optionally add morphology/smoothing through ITK with documented kernel sizes.

## UI quality requirements
- Test brush/erase mode transitions, undo/redo, selected label, progress/cancel, and unsaved-change indicators first.
- Capture and inspect segmentation overlays in all slice planes and 3D, including selected/unselected and error states.
- Ensure editing controls remain understandable without relying only on color.

## Verification and tests
- Synthetic disconnected objects: region grow selects expected component.
- Brush operation tests at slice boundaries and nonisotropic spacing.
- Undo/redo restores exact labelmaps.
- Save/reload mask with no voxel changes.

## Acceptance / exit gate
- User isolates one bone/region, refines it and generates its 3D mesh.
- All edits are reversible and survive project reload.

## Explicit exclusions
Do not implement features belonging to later milestones. No clinical-use claims, diagnosis, or treatment recommendations.

## Delivery artifacts
- Compiling source and updated dependency manifest.
- Unit/integration tests and CI results.
- `docs/M4-verification.md` with commands, test evidence, known limitations.
- A concise screen recording or reproducible demo procedure.
