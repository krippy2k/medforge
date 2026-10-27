# Milestone-wide quality gates

Every milestone must provide `docs/MX-verification.md` with: changed scope, tests written first and observed expected failures, implementation/test commands and outcomes, regression results, UI evidence where applicable, known limitations, and acceptance checklist.

## Mandatory gates
1. Red-green-refactor evidence for each testable behavior; document exceptions for exploratory graphics/spikes.
2. Build and relevant automated tests pass on available target platform; CI results recorded if available.
3. UI state/interaction tests via Qt Test for changed screens; smoke-launch and workflow checks.
4. Screenshots captured AND inspected at 1280x800, 1920x1080, 2560x1440 when supported; include DPI/resize checks. Mark unsupported checks NOT VERIFIED.
5. No dead controls, critical clipping/overlap, unreadable text, or broken focus/keyboard paths.
6. Responsive UI under long operations; cancellation and errors verified when relevant.
7. No identifiable patient data, no clinical validation claims; synthetic/de-identified fixtures only.
8. Requirements, screenshots, decisions, and demo instructions updated. Do not declare complete with unresolved mandatory gates.

Use status PASS / FAIL / NOT VERIFIED for each gate. NOT VERIFIED is not PASS.
