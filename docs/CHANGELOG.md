# Changelog

## v1.3.0 — 2025-11-28
- Stability: Reduced workers to 2, disabled brownout detector, thermal limit at 70°C.
- Performance: Tweaked batch sizes (SW 32K, HW 128K) and aggressive compiler optimizations.
- Metrics: Added 1h rolling average KH/s and total uptime average KH/s to API/UI.
- API: `/api/stats` now includes `avghashrate`, `totalavghashrate`, `avgwindowseconds`, per-worker rates, temperature, and pool info.
- UI: Responsive dashboard for mobile/desktop; improved layout and typography.
- Tooling: Release v1.3.0 published with firmware asset via GitHub CLI.

