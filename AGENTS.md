# XCSoar agent notes

Contributor policy: `docs/content/5.dev/04.policy.md`. Developer docs:
`docs/content/5.dev/00.index.md`. Do not copy `.cursor/rules/` into this
file. Open the matching rule when the work matches the table below.

## Docs

Always read `docs/content/5.dev/06.architecture.md` before changing layers,
threads, blackboards, device drivers, network/HTTP, or where a type belongs.
For UI colour, map overlays, dialogs, and in-flight safety, read **User
interface guidelines** on that page (NASA colour usage, FAA EFB
DOT/FAA/AR-03/67, ICAO Annex 4). Index of other developer docs:
`.cursor/rules/developer-docs.mdc`.

User-facing behaviour: update `docs/content/1.manual/` (not
`docs/content/5.dev/`) and `NEWS.txt`. See `.cursor/rules/user-manual.mdc`.

## Always

- Search before adding a helper, wrapper, parser, or enum. Prefer `src/util`,
  `Formatter/`, `Math/`, `Geo/`, and the same subsystem. Extend the existing
  class. No parallel API, and no extra files or abstraction layers for one
  call site.
- Keep the change as small as the problem.
- UTF-8 everywhere. Never truncate or measure strings at byte offsets. Use
  `SequenceLengthUTF8()`, `CopyTruncateString()`, `CropIncompleteUTF8()` from
  `util/UTF8.hpp`. Details: `.cursor/rules/utf8-safety.mdc`.
- User-visible strings: `_()`, `N_()`, `C_()`, `NC_()`. Reuse an existing
  msgid from `po/xcsoar.pot` when the English already fits. Then
  `make update-po` — the `.po` diff must only add new fields, not rewrap
  or rewrite existing translations. Details:
  `docs/content/5.dev/10.translations.md`. Log messages stay in English.
- User-visible behaviour: add a `NEWS.txt` bullet **and** update the English
  user manual in `docs/content/1.manual/`. Style: `.cursor/rules/news.txt.mdc`
  and `.cursor/rules/user-manual.mdc`.
- Style: 79 columns, 2-space indent, SPDX `GPL-2.0-or-later` headers.
- UI must work on OpenGL and memory canvas (Kobo). Scale with
  `Layout::`. Use `IsDithered()` / `HasColors()` for e-paper. Colour and
  in-flight HF: `docs/content/5.dev/06.architecture.md` User interface
  guidelines (NASA, FAA EFB).
- Layers: Foundation (`util/`, `Math/`, `Geo/`) → Engine → Backend
  (`Computer/`, `Device/`, `Blackboard/`) → UI. Device drivers and
  calculation must not include UI headers or call `CommonInterface`.
- Reviews: keep code human-readable, reject exponential/unbounded cost,
  and follow `docs/content/5.dev/06.architecture.md`. See
  `.cursor/rules/review-human-readable.mdc`,
  `.cursor/rules/review-exponential.mdc`,
  `.cursor/rules/review-architecture.mdc`.
- Do not create git commits unless the user explicitly asks.
- Before submitting a PR, compile with `make everything` (not only the
  main binary). Details: `.cursor/rules/pr-compilation.mdc`.

## Tests

Add TAP tests under `test/src/` for UI-free logic: parsers, formatters,
protocol encode/decode, math/geo helpers, string/path helpers, and
state-machine or policy functions. Extend an existing `Test*.cpp` when one
already covers the area. Register new binaries in `build/test.mk`. Do not
add unit tests for dialog layout, canvas drawing, JNI, or windowing.

```bash
make -j$(nproc) TARGET=UNIX USE_CCACHE=y check
```

Details: `.cursor/rules/xcsoar-testing.mdc`.

## Path-specific rules

| When working on | Read |
|------------------|------|
| C++ (`noexcept`, nullptr, enums) | `.cursor/rules/cpp-safety-patterns.mdc` |
| NMEA, devices, Validity / time | `.cursor/rules/nmea-validity-patterns.mdc` |
| Search / stacked dialogs | `.cursor/rules/search-dialog-ux.mdc` |
| Touch / lift-off / hold | `.cursor/rules/ui-touch.mdc`, `docs/content/5.dev/06.architecture.md` (Touch interaction) |
| Waypoint types / CUP round-trip | `.cursor/rules/waypoint-types.mdc` |
| SVG icons (`Data/icons/`) | `.cursor/rules/svg-icons.mdc` |
| `NEWS.txt` | `.cursor/rules/news.txt.mdc` |
| AUTHORS / third-party notices | `.cursor/rules/credits-attribution.mdc` |
| Test harness CLI (`--help`) | `.cursor/rules/cli-test-utilities.mdc` |
| TAP tests | `.cursor/rules/xcsoar-testing.mdc` |
| Architecture, i18n, build, platforms | `.cursor/rules/xcsoar-project-rules.mdc` |
| Pull requests / compile-before-PR | `.cursor/rules/pr-compilation.mdc` |
| Code review (readable, cost, layers) | `.cursor/rules/review-human-readable.mdc`, `.cursor/rules/review-exponential.mdc`, `.cursor/rules/review-architecture.mdc` |
| Layers, threads, blackboards, HTTP | `docs/content/5.dev/06.architecture.md` |
| UI colour, EFB / NASA HF | `docs/content/5.dev/06.architecture.md` (User interface guidelines) |
| Other developer docs | `.cursor/rules/developer-docs.mdc` |
| User manual (pilots) | `.cursor/rules/user-manual.mdc` |
| gettext / `po/` | `.cursor/rules/translations.mdc` |
