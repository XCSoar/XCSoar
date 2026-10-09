# Store metadata

This directory contains in-repo store metadata managed by Fastlane.

## Google Play (org.xcsoar.play)

Google Play listing metadata is stored in Fastlane's standard layout:

- `metadata/android/en-US/title.txt`
- `metadata/android/en-US/short_description.txt`
- `metadata/android/en-US/full_description.txt`
- `metadata/android/en-US/images/phoneScreenshots/*.jpg`
- `metadata/android/<locale>/title.txt`
- `metadata/android/<locale>/short_description.txt`
- `metadata/android/<locale>/full_description.txt`

`phoneScreenshots` currently contains the selected screenshot set
for issue #2104, ordered with numeric prefixes to control upload
display order.

Localized Play Store metadata is currently provided for:
`de-DE`, `el-GR`, `es-ES`, `fr-FR`, `it-IT`, `ja-JP`, `ko-KR`,
`nl-NL`, `pl-PL`, `pt-BR`, `pt-PT`, `tr-TR`, `uk`, `zh-CN`,
and `zh-TW`.

CI uploads this metadata with:

- `.github/workflows/update-play-store-metadata.yml` (metadata-only updates)
- `.github/workflows/build-native.yml` (AAB + metadata upload to internal track)
- `.github/workflows/promote-play-open-testing.yml` (weekly promotion of the
  latest internal release to open testing; lane `promote_open_testing`)

These workflows require the `GOOGLE_PLAY_JSON_KEY` GitHub secret.

Ruby dependencies are pinned in the repo-root `Gemfile` / `Gemfile.lock`
and installed via Bundler in CI (`bundle exec fastlane supply`).

## F-Droid

F-Droid reads graphics from this tree after the build:

- `metadata/android/en-US/images/icon.png` (512×512 store icon)
- `metadata/android/en-US/images/phoneScreenshots/`

The store icon is not in git. The Android build renders it from
`Data/graphics/logo.svg` (`FASTLANE_ICON` in `build/android.mk`).
Listing text stays in fdroiddata.

The APK also ships `mipmap-<density>/ic_launcher.png` and
`ic_launcher_round.png` so the indexer can extract a PNG. The
on-device icon remains the adaptive XML in `mipmap-anydpi-v26`.

## Other stores

Apple metadata is not managed here yet.
