# XCSoar software-update repository entries

XCSoar discovers directly distributed stable releases in the trusted main file
repository at `https://download.xcsoar.org/repository`.  It does not accept
software-update offers from user-configured repositories.

The repository is refreshed in the background at startup when its cached copy
is at least 24 hours old.  This refresh is shared with the existing data-file
repository; update discovery does not make a separate HTTP request.  **Check
now** forces a refresh of the same repository.

This initial implementation reports a newer release and opens its download
page.  It does not download or install an executable or package.

## Entry format

Publish one `software-update` entry for each directly distributed build target:

```text
name = xcsoar-WIN64OPENGL
uri = https://download.xcsoar.org/releases/7.46/WIN64OPENGL/
description =
type = software-update
target = WIN64OPENGL
version = 7.46
channel = stable
offer-id = 7.46
source = XCSoar
```

The fields have these meanings:

| Field | Requirement |
|---|---|
| `name` | Unique valid repository filename, conventionally `xcsoar-<TARGET>`. |
| `uri` | HTTPS handoff URL on a host allowed by the build. |
| `description` | Optional plain UTF-8 summary, at most 383 bytes. |
| `type` | Exactly `software-update`. |
| `target` | Exact build target, such as `UNIX`, `MACOS`, or `WIN64OPENGL`. |
| `version` | Two or three unsigned numeric components. |
| `channel` | Exactly `stable` in the initial implementation. |
| `offer-id` | Stable identifier used for **Skip this version**; defaults operationally to the version. |
| `source` | Optional publisher label, at most 127 UTF-8 bytes. |

Unknown, incompatible, malformed, non-stable, and non-allow-listed entries are
ignored.  If more than one valid entry exists for a target, XCSoar selects the
newest version numerically.

Software-update entries are metadata, not normal repository resources.  They
must not appear in the file picker or participate in **Update all**.

## Publication

Generate a validated entry with:

```sh
python3 tools/generate_update_repository_entry.py \
  --target WIN64OPENGL \
  --version 7.46 \
  --url https://download.xcsoar.org/releases/7.46/WIN64OPENGL/ \
  --output output/update-repository/WIN64OPENGL
```

Run the publisher tests with:

```sh
python3 test/test_update_repository_entry.py
```

The download server's repository publication process must merge the generated
target entries into the trusted main repository atomically after the matching
release artifacts are available.  CI does not update the shared repository
file directly because matrix jobs must not race while rewriting one index.

Google Play and App Store availability have separate publication boundaries
and should use store-specific update providers.  User repositories must never
be treated as trusted software-update sources.
