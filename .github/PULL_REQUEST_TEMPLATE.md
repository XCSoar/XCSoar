Thank you for contributing to XCSoar!

We truly appreciate your time and effort in making XCSoar better. We know
that contributing to an open-source project can be challenging, and we're
grateful that you've chosen to help.

This checklist is here to help guide you through the submission process.
Don't worry if you're not sure about something—feel free to ask questions
or submit your PR, and we'll work together to get it ready.

For coding, style guide, architecture information please see our
[development guide](https://xcsoar.readthedocs.io/en/latest/index.html).

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

---

- [ ] I'm ready to merge
