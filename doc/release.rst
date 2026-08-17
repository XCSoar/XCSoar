###############
Release process
###############

XCSoar uses two separate version changes for each minor release: marking the
current version as released and starting development of the next version.
Keeping these operations separate ensures that development builds, release
artifacts, and release notes all use the same version.

Version files
=============

The release manager must keep these files in sync:

``VERSION.txt``
  Contains the version used by the build system.  A build whose commit is not
  tagged with the matching ``v<version>`` tag gets a Git commit suffix.

``NEWS.txt``
  Contains the cumulative release notes.  The first block describes the
  version currently under development.  See :doc:`policy` for release-note
  guidelines.

``debian/changelog``
  Contains a top stanza for the current version.

``android/AndroidManifest.xml.template``
  Contains the baseline Android ``versionName`` and ``versionCode``.  CI
  replaces these values for master and tag builds, but the checked-in values
  must still match ``VERSION.txt``.

The Android version code is calculated as::

  major * 10000000 + minor * 100000 + patch * 10000 + build

``patch`` and ``build`` are zero for a normal minor release.  For example,
version 7.46 uses version code ``74600000``.  CI uses its run number for the
``build`` field on master and zero for a release tag.

Preparing a minor release
=========================

#. Finalise the release notes in the first ``NEWS.txt`` block.  Follow the
   section, wording, and issue-reference conventions in :doc:`policy`.
#. Verify that ``VERSION.txt``, the first ``NEWS.txt`` version, the first
   ``debian/changelog`` stanza, and the Android ``versionName`` all contain
   the version being released.
#. Verify that the Android ``versionCode`` matches the formula above.
#. Replace ``not yet released`` in the ``NEWS.txt`` version heading with the
   release date in ``YYYY-MM-DD`` format.
#. Refresh the maintainer timestamp in the existing ``debian/changelog``
   stanza.  Do not add another stanza for the same version.
#. Check that the release-note extractor finds the intended block::

     ./tools/changelog.sh v7.46

#. Build and test the release as appropriate for the supported targets.
#. Commit the release metadata.  The current commit-message pattern is::

     NEWS.txt, debian/changelog: Mark v7.46 released

At this point ``VERSION.txt`` and the Android manifest template do not change;
they have contained the release version throughout its development cycle.

Tagging and publication
=======================

Create an annotated ``v<version>`` tag on the release commit and push it::

  git tag -a v7.46 -m "v7.46"
  git push upstream v7.46

The tag version must match ``VERSION.txt``.  The native-build workflow uses
the tag to extract the corresponding ``NEWS.txt`` block, create the GitHub
release, build release artifacts, and run the configured publication jobs.
Release branches are maintained according to the branch policy in
:doc:`policy`.

Starting the next development cycle
===================================

After publishing a minor release, bump master to the next minor version in
one commit:

#. Increment ``VERSION.txt``.
#. Add ``Version <next> - not yet released`` and a blank line at the top of
   ``NEWS.txt``.
#. Add a new top stanza to ``debian/changelog`` for the next version.  Refresh
   its timestamp when that version is eventually released.
#. Update ``versionName`` and the baseline ``versionCode`` in
   ``android/AndroidManifest.xml.template``.
#. Verify the changes with ``git diff --check`` and an appropriate build.

The current commit-message pattern is::

  VERSION.txt, NEWS, debian, android: Bump to v7.47 development

Do not update ``src/Version.hpp`` for a release bump.  Its declarations use
the value supplied by the build system from ``VERSION.txt``.

Patch releases
==============

Patch releases use all three version components, for example ``7.46.1``.
Apply the same consistency checks and include the patch component in the
Android version code.  Make patch-release changes on the current minor branch
as described in :doc:`policy`.
