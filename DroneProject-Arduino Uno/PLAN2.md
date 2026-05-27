# Website Save Reliability + CH8 Link Modes (1000/1500/2000)

## Summary

Implement a robust writable settings flow that does not fail on missing fields, then add your CH8 3-position communication mode as runtime protection for web traffic.  
The mode map will be exactly as agreed:

- `<=1300` (`1000`): normal send/receive
- `1301..1699` (`1500`): broadcast-only (telemetry only, block all POST)
- `>=1700` (`2000`): receive-priority (allow POST, reduce telemetry load)
- CH8 stale/lost: fallback to normal (`1000` behavior)

### Key Implementation Changes

- Convert web settings endpoint from read-only to writable with **partial-safe merge**:
  start from current settings, apply only valid incoming fields, keep existing values for missing/invalid fields, sanitize, then save.
- Add structured validation result in response (`applied`, `ignored`, `warnings`) instead of hard “field error” for partial input.
- Add CH8 web-link mode in flight loop (decoded from RC channel 8 with `1300/1700` thresholds) and expose mode in telemetry/state JSON.
- Enforce mode in API:
  `1500` mode blocks all POST endpoints (`/api/settings`, `/api/calibrate-level`, `/api/home/*`);
  `1000` and `2000` allow POST (subject to existing safety checks, including “block save while armed”).
- In `2000` mode, reduce outgoing load without breaking website:
  slower state poll hint, reduced redraw frequency for heavy widgets (map/3D), keep command endpoints responsive.
- Update web UI to show current CH8 mode clearly and auto-lock/unlock controls:
  broadcast-only shows read-only badge and disables save/actions;
  receive-priority shows “reduced telemetry” badge and adjusts polling interval.

### API / Interface Updates

- `GET /api/state` adds web-link fields (mode + rx permission + poll hint).
- `POST /api/settings` changes to merge-and-validate behavior and returns detailed save outcome (not binary field failure).
- Internal telemetry struct gains CH8-derived web-link state fields.

### Test Plan

- Save-path tests:
  full payload, partial payload, wrong-type fields, unknown fields, empty payload.
- Mode-gating tests with CH8 values:
  `1000` allows POST, `1500` rejects POST, `2000` allows POST with reduced telemetry cadence.
- CH8 loss test: stale AUX2 falls back to normal mode.
- Safety tests: save while armed must be rejected in all modes.
- UI smoke tests:
  mode badge correctness, controls lock/unlock behavior, no website interruption in `2000`, no command acceptance in `1500`.

### Assumptions / Defaults

- This plan targets the current codebase where web settings write is being introduced.
- CH8 is `AUX2` (index 7), using agreed threshold bands (`1300/1700`).
- If your CH8 strategy causes instability in real testing, fallback path is to keep partial-safe merge but disable CH8 mode gating behind a compile-time flag.
