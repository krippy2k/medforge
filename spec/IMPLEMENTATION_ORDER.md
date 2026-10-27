# Execution plan and interview strategy

## Suggested order
- **Phase A (foundation):** M0, M1. Demonstrate CMake/Qt/VTK architecture and polished UI.
- **Phase B (imaging):** M2, M3. Reach a usable CT viewer with 3D reconstruction.
- **Phase C (differentiation):** M4, M5. Segmentation and precise geometry/measurements are the interview sweet spot.
- **Phase D (CAD):** M6, M7. Mesh repair and optional experimental surgical guide or implant prototype.
- **Phase E (release):** M8. Note that basic smoke testing, profiling, documentation and project persistence should be developed incrementally, not deferred entirely to M8.

## Demo sequence (5–7 minutes)
1. Show source architecture and automated tests briefly.
2. Import a de-identified CT series; inspect three orthogonal planes.
3. Show synchronized 3D volume and a threshold-based anatomy mesh.
4. Segment a single bone, refine it, and regenerate the mesh.
5. Place landmarks, measure distance/angle/area, and export a report.
6. Optional: inspect/repair mesh and export STL, or position CAD implant.
7. Show cancellation/progress and explain LPS coordinates, physical units, Qt thread affinity, ownership, and numerical validation.

## Scope control
Each milestone must be independently demonstrable. Prioritize correct coordinate transforms, physical units, and robust error paths over adding more visual effects. Treat all geometry as unvalidated research output.

## Mandatory process at every milestone
1. Read design system, quality gates, and current milestone.
2. Plan test cases and UI states; write failing tests first where testable.
3. Implement/refactor with autonomous routine decisions.
4. Run build/tests and screenshot capture + actual visual inspection.
5. Publish PASS/FAIL/NOT VERIFIED quality-gate report before proceeding.

M0 now includes a developer component gallery and approved M1 workspace reference; do not skip it to accelerate feature development.
