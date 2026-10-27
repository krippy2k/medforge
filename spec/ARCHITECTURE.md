# Architecture and Engineering Decisions

## Technology baseline
- **C++20 / Qt 6 Widgets** for desktop UI; CMake and CTest for builds/tests.
- **VTK** for slice/volume visualization, surface extraction, rendering, mesh utilities.
- **ITK** for segmentation/image processing when M4 requires it.
- **DCMTK** for robust DICOM metadata/series import, if selected at M2; evaluate GDCM/ITK integration before committing.
- **Open CASCADE** at M7 for B-Rep, solid operations and STEP, isolated behind an adapter.
- **CGAL** optional at M6 for difficult mesh repair; do not add unless VTK and focused algorithms are inadequate.
- Use dependency versions pinned via vcpkg manifest or equivalent lockable mechanism; document licensing/redistribution obligations.

## Module boundaries
- `core`: physical units, LPS patient/world/voxel coordinate transforms, immutable identifiers, document model, command history.
- `imaging`: DICOM loading, image volumes, resampling, segmentation masks.
- `geometry`: surface meshes, validation, measurements, algorithms.
- `rendering`: VTK pipeline and view synchronization; graphics objects never leak into domain API.
- `cad`: optional external CAD kernel, implant and guide operations.
- `ui`: Qt windows, tools, commands, task progress, error presentation.

## Data model
- `Study` → `Series` → `Volume` with spacing, origin, direction matrix, intensity calibration and provenance.
- `Segmentation` stores a labelmap and references a source volume.
- `MeshAsset` references source segmentation and processing history.
- `Landmark`, `Measurement`, `Plane`, `ImportedCADAsset` are document entities with stable IDs.
- Project document serializes references, transforms, tool state and asset versions; binary assets stored beside manifest.

## Coordinate convention
- Use patient LPS millimeters as canonical physical space internally, with explicit transformations from DICOM voxel indices and any RAS/world/render coordinates.
- Never assume axial slices are axis-aligned, isotropic, or uniformly oriented without validating metadata.
- Validate orientation, spacing, image position and series consistency; reject or clearly warn on unsupported geometries.
- Mesh/export transform and units must be explicit; STL has no intrinsic unit metadata.

## Concurrency
- Use worker tasks (QThreadPool or equivalent) with cancellation tokens, progress callbacks, and ownership-safe handoff of completed results.
- Rendering/UI mutation only on the GUI thread. Preserve previous document state on cancellation/failure.

## Persistence and safety
- Versioned project manifest, atomic saves, relative paths, checksums for imported data references.
- Avoid embedding PHI in logs and exports; display provenance and anonymization caveats.
- No clinical validity claims; do not infer accuracy from attractive renderings.

## Testing strategy
- Unit: geometry, coordinate transforms, calibration, commands, serialization.
- Integration: DICOM load, mesh pipeline, save/reload, export/reimport.
- GUI smoke: start application, open synthetic sample, navigate views, cancel task.
- Performance: reproducible dataset dimensions, runtime, peak memory, UI responsiveness.

## UI architecture and verification
- Keep design tokens and reusable Qt Widgets controls in a shared UI foundation; UI view models expose testable state independent of rendering.
- Use Qt Test for interaction/state tests, deterministic synthetic fixtures for image/geometry correctness, and screenshot inspection for visual QA.
- Store approved references under `docs/ui/reference/` and actual evidence under `docs/ui/evidence/MX/`.
- Follow `DESIGN_SYSTEM.md` and `QUALITY_GATES.md` throughout implementation.
