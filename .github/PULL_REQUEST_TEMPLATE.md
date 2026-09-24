Thank you for contributing to XCSoar!

We truly appreciate your time and effort in making XCSoar better. We know
that contributing to an open-source project can be challenging, and we're
grateful that you've chosen to help.

This checklist is here to help guide you through the submission process.
Don't worry if you're not sure about something—feel free to ask questions
or submit your PR, and we'll work together to get it ready.

For coding, style, and architecture see the
[development guide](https://xcsoar.readthedocs.io/en/latest/index.html),
especially
[architecture](https://xcsoar.readthedocs.io/en/latest/architecture.html)
and [policy](https://xcsoar.readthedocs.io/en/latest/policy.html).

For Git tips and tricks, including interactive rebase, fixup commits, and
common workflows, see the
[Git Tips](https://xcsoar.readthedocs.io/en/latest/git_tips.html)
documentation.

For testing and debugging utilities, see the
[Test & Debug
Utilities](https://xcsoar.readthedocs.io/en/latest/test_debug_utilities.html)
documentation. 

## Pre-Submission Checklist

Please verify the following before submitting your PR:

### Git & Commit History

#### Branch & Rebase
- [ ] PR is rebased on current `master`
- [ ] Use `git rebase -i` to clean up commit history before PR
  submission

#### Commit Format & Messages
- [ ] All commits follow the format: `<Component>: <Summary>` (no
  `src/` prefix)
- [ ] Use present tense in commit messages ("Fix" not "Fixed", "Add"
  not "Added")
- [ ] Commit messages explain *why* the change was made, not just
  *what* changed
- [ ] Commit message body (if needed) provides detailed reasoning and
  context

#### Commit Structure
- [ ] Each commit is atomic and builds successfully (every commit
  must compile)
- [ ] One commit per logical change (don't mix refactoring with
  feature changes)
- [ ] Self-contained commits (each commit changes one thing)
- [ ] No fixup commits (squashed into parent commits using
  `git rebase -i`)
- [ ] No duplicate commits (check with `git log --oneline`)
- [ ] No "WIP" or "testing" commits (clean up before PR)

### Code, tests & docs (where it applies)

- [ ] Reuses existing helpers and UI strings (no parallel API, extra
  abstraction, or near-duplicate `_()` msgid)
- [ ] TAP tests for new UI-free logic (parsers, formatters, protocol,
  math/geo); registered in `build/test.mk`
- [ ] User-visible behaviour: `NEWS.txt` and English manual
  (`doc/manual/en/`)
- [ ] New gettext strings: `make update-po`; `.po` diff only adds new
  fields (see
  [i18n](https://xcsoar.readthedocs.io/en/latest/i18n.html))

---

- [ ] I'm ready to merge
