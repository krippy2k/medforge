# M0 — Foundation and toolchain

## Goal
Establish a clean, repeatable C++/Qt application platform.

## Prerequisites
None

## In scope
- CMake superproject, desktop executable, library targets, tests, CI, formatting, static analysis.
- Windows 11 and Linux are both configure/build/test targets. Windows remains the primary UI target.

## Functional and technical requirements
- Pin C++20, Qt 6, compiler/toolchain and dependency versions.
- Build debug and release configurations with CMake presets.
- Define module interfaces and dependency direction; no cyclic dependencies.
- Configure CTest, unit-test framework, clang-format, clang-tidy baseline.
- Add GitHub Actions Windows build/test and optional Linux matrix.
- Add synthetic test-data generator and third-party license inventory.

## Design-system and agent-workflow foundation (mandatory)
- Create `.cursor/rules/` project rules, `DESIGN_SYSTEM.md`, and `QUALITY_GATES.md`; verify the rules are loaded in Cursor and that terminal permissions are configured separately.
- Implement a runnable Qt component gallery (developer-only mode) containing the controls and states listed in `DESIGN_SYSTEM.md`. Do not implement M1's full workspace or VTK viewport.
- Produce an annotated main-window wireframe/reference for M1, a tokenized QSS/style layer, and a shared component library foundation.
- Add Qt Test examples demonstrating red → green → refactor for one widget state and one core coordinate conversion. Capture observed expected failures before implementation.
- Add a repeatable screenshot capture and inspection procedure (Windows primary) and store actual inspected gallery screenshots; if unavailable, document a manual review protocol and mark visual gate NOT VERIFIED.
- Exit gate: component gallery launches, tests pass, token reuse is demonstrated, reference is reviewed, and M0 quality-gate report exists.

## Verification and tests
- Fresh clone configure/build/test with documented commands.
- CI verifies clean build, unit test sample, formatting checks.
- Minimal sample test for mm/voxel conversion.

## Acceptance / exit gate
- Clean build and CI pass.
- `README.md` describes architecture, development commands, limitations.

## Explicit exclusions
Do not implement features belonging to later milestones. No clinical-use claims, diagnosis, or treatment recommendations.

## Delivery artifacts
- Compiling source and updated dependency manifest.
- Unit/integration tests and CI results.
- `docs/M0-verification.md` with commands, test evidence, known limitations.
- A concise screen recording or reproducible demo procedure.
