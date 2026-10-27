# MedForge UI design system (M0 deliverable)

## Experience principles
Professional, information-dense, predictable, accessible desktop workstation. Prioritize scan/geometry fidelity and safe, reversible interactions over decoration. Research-only label remains visible in About and exported reports.

## Application anatomy
- Top: native menu, compact task toolbar, dataset/status context.
- Left dock: studies, volumes, segmentations, models (hierarchical browser).
- Center: configurable 1x1 and 2x2 viewports; MPR + 3D when available.
- Right dock: contextual tool settings, properties, measurements.
- Bottom: status/progress, units and coordinate reference, cancellable task feedback.
- Panels: keyboard accessible, dockable/resizable, persist/reset layout; no critical controls hidden below minimum window size.

## Tokens (initial baseline; validate with actual screenshots)
- Palette: background #15191F; raised #20262E; panel #252D36; border #3A4652; primary text #E9EFF5; secondary #AFBBC8; accent #4BA3C7; selected #234B60; warning #E7B75D; error #E67E80; success #65BFA1.
- Typography: system UI font (Segoe UI on Windows); base 12–13 px at 100% scaling; section headings 14–16 px; tabular numerals for numeric measurements. Qt should use DPI-aware logical sizing.
- Spacing: 4 px base grid; 4/8/12/16/24 px rhythm; compact toolbar buttons around 32 logical px with accessible hit targets.
- Icons: consistent single vector icon family; semantic labels/tooltips; do not rely on color alone.
- States: default, hover, focus, selected, disabled, loading, error; clear empty states and destructive-operation confirmation.

## Reusable component gallery (must ship in M0)
Show enabled/disabled buttons, tool buttons, segmented control, labeled numeric editor with units, search field, hierarchical browser row, property group, dock header, progress with cancel, notification/banner, empty state, validation error, and modal dialog. Gallery may be developer-only, but must launch from an explicit dev command/flag.

## Visual reference and review
- M0: produce a main-window wireframe/reference (Qt mockup or annotated image) and component gallery screenshots; obtain one design baseline decision before M1.
- Store reviewed references in `docs/ui/reference/`, observed screenshots in `docs/ui/evidence/MX/` and an issue checklist in milestone verification.
- At minimum review 1280x800, 1920x1080, 2560x1440; 100% and 150% scaling when available; document unsupported environments.
- Check viewport proportions, typography, truncation, contrast, toolbar density, resize behavior, state feedback, and keyboard focus. Visual reviews require actual screenshot inspection.
- Golden image tests are optional for stable widget regions, with documented tolerance; do not rely on exact pixel equality for VTK/GPU scenes.
