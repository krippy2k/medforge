# Product Requirements Document — MedForge

## Problem
A CT scan contains volumetric anatomy but is not directly useful as an editable engineering model. MedForge demonstrates a reproducible research workflow from CT data to segmented anatomy, measurable meshes, and experimental CAD planning geometry.

## Target users
- Medical-device software engineers and computational-geometry developers
- Imaging research and educational users
- Interviewers evaluating C++, Qt, imaging, and CAD proficiency

## Core workflow
1. Open de-identified DICOM CT series.
2. Inspect synchronized axial, sagittal, and coronal slices.
3. Visualize volume and isolate anatomy with segmentation tools.
4. Generate and inspect a surface mesh.
5. Place landmarks and perform measurements in physical units.
6. Clean mesh, export STL, and generate measurement report.
7. Optionally prototype a surface-fitted guide or import a CAD implant.

## Functional requirements
- Dataset browser with series selection and metadata summary.
- 2D/3D linked visualization with predictable camera and coordinate behavior.
- Segmentation editing with undo/redo and persisted project state.
- Measurement tools with units, uncertainty caveats, and repeatable results.
- Non-destructive geometry processing and CAD interoperability.
- Background processing, cancellation, error handling, and progress.

## Nonfunctional requirements
- C++20, Qt 6, CMake, modular architecture, reproducible dependency management.
- Cross-platform design. Windows 11 and Linux are both supported build and test targets. Windows remains the primary workstation UI target.
- UI never blocks on DICOM loading, segmentation, or mesh operations.
- No external upload or telemetry by default.
- Explicit coordinate-space conventions and physical-unit conversions.
- Testability: synthetic volumes and known geometry with expected answers.
- Visible 'Research/demo only — not for clinical use' notice.

## Out of scope
- Diagnosis, treatment recommendations, clinical safety claims, regulatory certification.
- Production surgical navigation, patient-specific manufacture approval, robotic actuation.
- PACS integration, cloud synchronization, multiuser collaboration, AI diagnosis.

## Success criteria
- Demonstrate an end-to-end workflow with a de-identified dataset.
- Save/reopen project and reproduce segmented meshes and measurements.
- Export a valid STL mesh; compare measurements against synthetic ground truth.
- Explain architectural choices and show tests/performance traces during interview.

## Cross-cutting engineering and experience requirements
- All milestones follow `QUALITY_GATES.md`, including test-first evidence and explicit visual QA status.
- UI implementations conform to `DESIGN_SYSTEM.md` and reuse approved components and tokens.
- M0 provides the design foundation, component gallery, and workspace reference before feature screens are developed.
- Agent autonomy is bounded by `.cursor/rules/autonomy.mdc` and actual editor permissions.
