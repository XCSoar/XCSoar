# Store metadata

This directory contains in-repo store metadata managed by Fastlane.

## Google Play (org.xcsoar.play)

Google Play listing metadata is stored in Fastlane's standard layout:

- `metadata/android/en-US/title.txt`
- `metadata/android/en-US/short_description.txt`
- `metadata/android/en-US/full_description.txt`

CI uploads this metadata with:

- `.github/workflows/update-play-store-metadata.yml` (metadata-only updates)
- `.github/workflows/build-native.yml` (AAB + metadata upload to internal track)

Both workflows require the `GOOGLE_PLAY_JSON_KEY` GitHub secret.

## Other stores

Apple and F-Droid metadata are not managed here yet.
