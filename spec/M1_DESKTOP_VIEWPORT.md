# M1 — Qt desktop shell and 3D viewport

## Goal
Deliver a polished navigable desktop with rendering integration.

## Prerequisites
M0 complete

## In scope
- Main window, panels, menus, status bar, workspace layout.
- Embedded VTK 3D view with sample geometry.

## Functional and technical requirements
- Qt Widgets UI with dockable dataset, properties and task panels.
- Integrate VTK with QVTKOpenGLNativeWidget or compatible Qt 6 adapter.
- Render test cube/sphere, axes, grid, orbit/pan/zoom, reset camera.
- Add action/command infrastructure and error notification.
- Persist UI layout preferences separately from project data.
- Enforce render-thread/GUI-thread ownership.

## UI quality requirements
- Build the workspace to the approved M0 reference: top toolbar, study browser, central viewport, contextual inspector, status/progress bar.
- Reuse M0 components/tokens; implement empty/loading/error states and dock layout reset.
- Write Qt Test cases first for dock state persistence, action enablement, focus and reset camera behavior where automatable.
- Capture and inspect screenshots for empty, sample geometry, resized, and restored layouts; record supported resolutions/DPI checks.

## Verification and tests
- GUI smoke launch and sample rendering test.
- Manual viewport input checks at different DPI scales.
- Restore dock layout and reset camera checks.

## Acceptance / exit gate
- App launches and renders interactively without errors.
- Docking, camera controls, and settings restore work reliably.

## Explicit exclusions
Do not implement features belonging to later milestones. No clinical-use claims, diagnosis, or treatment recommendations.

## Delivery artifacts
- Compiling source and updated dependency manifest.
- Unit/integration tests and CI results.
- `docs/M1-verification.md` with commands, test evidence, known limitations.
- A concise screen recording or reproducible demo procedure.
