# Store listings

This folder holds the canonical copy for app store listings so it can be
version-controlled and reviewed in the repo.

## Google Play (org.xcsoar.play)

The live listing (title, short description, full description, screenshots,
feature graphic) is edited in the [Google Play Console](https://play.google.com/console).

**To update the listing:**

1. **Option A – Copy from repo:** Use the text in `play-store/` (see below)
   when editing the listing in Play Console. Update the files here first, then
   paste into Console.

2. **Option B – GitHub Action:** The workflow
   [.github/workflows/update-play-store-metadata.yml](../../.github/workflows/update-play-store-metadata.yml)
   uploads metadata from `play-store/metadata/android/` to Play Console using
   [Fastlane supply](https://docs.fastlane.tools/actions/supply/). It runs
   on push to `master` when files under `doc/store/play-store/` change, or
   manually via **Actions → Update Play Store metadata → Run workflow**.

   **Required secret:** In the repo **Settings → Secrets and variables →
   Actions**, add a secret named `GOOGLE_PLAY_JSON_KEY` with the **entire
   contents** of your Google Play service account JSON key file (from Play
   Console → Setup → API access → Create service account → JSON key). See
   [Fastlane supply setup](https://docs.fastlane.tools/actions/supply/#setup)
   for creating the service account and enabling the Google Play Developer API.

3. **Option C – Local Fastlane:** Run supply yourself with a local key file
   and `metadata_path` pointing at `play-store/metadata/android/`.

**Releases (build and push to Play):**

On push of a tag `v*`, the workflow
[.github/workflows/build-native.yml](../../.github/workflows/build-native.yml)
builds the Android app bundle (AAB) and then the **Upload to Google Play** job
downloads that AAB and runs [Fastlane supply](https://docs.fastlane.tools/actions/supply/)
to upload it to the **internal** track (promote to beta/production from Play
Console). Metadata from `play-store/metadata/android/` is uploaded with the
AAB.

**Required:** Same `GOOGLE_PLAY_JSON_KEY` secret as for metadata (Option B).
For a **signed** release AAB, the build uses the upload keystore when
`XCSOAR_UPLOAD_KEY_JKS_PASSWORD` and `XCSOAR_UPLOAD_KEY_JKS_KEY_ALIAS` (and the
keystore file) are configured in the build job; see the build workflow for
details.

**Play Console locations:**

- **App name:** Main store listing → Store settings → App name
- **Short description** (max 80 chars): Store listing → Main store listing
- **Full description** (max 4000 chars): Store listing → Main store listing
- **Screenshots / feature graphic:** Store listing → Graphics

**Limits:** Title 30 chars, short description 80 chars, full description 4000
chars. See [Play Console help](https://support.google.com/googleplay/android-developer/answer/9859152).

**Metadata layout:**

- `play-store/title.txt`, `short-description.txt`, `full-description.txt` –
  human-friendly copy (Option A).
- `play-store/metadata/android/en-US/` – [Fastlane supply](https://docs.fastlane.tools/actions/supply/) layout
  (`title.txt`, `short_description.txt`, `full_description.txt`) used by the
  GitHub Action and Option C. Keep in sync with the root files when editing.

## App Store (iOS)

iOS listing is managed in [App Store Connect](https://appstoreconnect.apple.com/).
There is no in-repo copy here yet; add `app-store/` with description text if
you want to version it the same way.
