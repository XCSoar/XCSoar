Git Tips and Tricks
===================

This document provides practical Git tips and tricks for XCSoar development,
focusing on common workflows and useful commands. For detailed Git documentation,
see the official Git manual or `git help <command>`.

Interactive Rebase
------------------

Interactive rebase (``git rebase -i``) is one of the most powerful tools for
cleaning up commit history before submitting a pull request.

**Starting an interactive rebase**:

.. code-block:: bash

   # Rebase last 5 commits
   git rebase -i HEAD~5
   
   # Rebase from a specific commit
   git rebase -i <commit-hash>
   
   # Rebase from upstream/master
   git rebase -i upstream/master

**Common rebase operations**:

- ``pick`` (or ``p``): Use the commit as-is
- ``reword`` (or ``r``): Change the commit message
- ``edit`` (or ``e``): Stop to amend the commit
- ``squash`` (or ``s``): Combine with previous commit, keep both messages
- ``fixup`` (or ``f``): Combine with previous commit, discard this message
- ``drop`` (or ``d``): Remove the commit entirely
- ``exec`` (or ``x``): Run a shell command

**Example: Squashing fixup commits**:

.. code-block:: bash

   # Interactive rebase to squash fixups
   git rebase -i HEAD~10
   # Change "pick" to "fixup" for commits you want to squash
   # Save and close editor

**Example: Reordering commits**:

.. code-block:: bash

   git rebase -i HEAD~5
   # Reorder lines in the editor to change commit order
   # Git will replay commits in the new order

**Example: Dropping unwanted commits**:

.. code-block:: bash

   git rebase -i HEAD~10
   # Change "pick" to "drop" (or delete the line) for commits to remove

Fixup Commits
-------------

Fixup commits are useful for making small corrections without creating a new
standalone commit.

**Creating a fixup commit**:

.. code-block:: bash

   # Make your changes
   git add <files>
   git commit --fixup <commit-hash>
   
   # Or use autosquash to automatically fixup the referenced commit
   git commit --fixup HEAD~2

**Auto-squashing fixups**:

.. code-block:: bash

   # Automatically squash all fixup commits during rebase
   git rebase -i --autosquash HEAD~10
   
   # Or set autosquash as default
   git config --global rebase.autoSquash true

**Example workflow**:

.. code-block:: bash

   # Make a commit
   git commit -m "Device/Flarm: Add new feature"
   
   # Later, find a typo
   git commit --fixup HEAD
   
   # Before pushing, auto-squash
   git rebase -i --autosquash HEAD~5

Useful One-Liners
-----------------

**View commit history**:

.. code-block:: bash

   # Compact one-line log
   git log --oneline
   
   # Last 10 commits with graph
   git log --oneline --graph -10
   
   # Commits in a specific date range
   git log --oneline --since="2 weeks ago" --until="1 week ago"

**Finding commits**:

.. code-block:: bash

   # Find commits touching a file
   git log --oneline -- <file>
   
   # Find commits with specific text in message
   git log --oneline --grep="Fix"
   
   # Find commits by author
   git log --oneline --author="John"

**Cleaning up**:

.. code-block:: bash

   # Remove untracked files and directories
   git clean -fd
   
   # Dry run first
   git clean -fdn
   
   # Reset to match remote (discard local changes)
   git reset --hard origin/branch-name

**Branch management**:

.. code-block:: bash

   # List merged branches
   git branch --merged
   
   # Delete merged branches
   git branch --merged | grep -v "\*\|master\|main" | xargs git branch -d
   
   # Rename current branch
   git branch -m new-branch-name

**Stashing**:

.. code-block:: bash

   # Stash with message
   git stash push -m "WIP: working on feature"
   
   # Stash including untracked files
   git stash push -u
   
   # List stashes
   git stash list
   
   # Apply and keep stash
   git stash apply
   
   # Apply and drop stash
   git stash pop

**Viewing changes**:

.. code-block:: bash

   # Show what changed in last commit
   git show HEAD
   
   # Show diff between branches
   git diff branch1..branch2
   
   # Show only file names that changed
   git diff --name-only branch1..branch2

**Amending commits**:

.. code-block:: bash

   # Amend last commit message
   git commit --amend
   
   # Amend last commit with new changes
   git add <files>
   git commit --amend --no-edit
   
   # Amend commit date
   git commit --amend --date="now"

Visual Tools
------------

**tig** - Text-mode interface for Git:

.. code-block:: bash

   # Install (Debian/Ubuntu)
   sudo apt install tig
   
   # Run tig
   tig
   
   # View specific branch
   tig branch-name
   
   # View file history
   tig <file>

**lazygit** - Simple terminal UI for Git:

.. code-block:: bash

   # Install (see https://github.com/jesseduffield/lazygit)
   # Run lazygit
   lazygit

Both tools provide interactive interfaces for:
- Viewing commit history
- Staging/unstaging files
- Creating commits
- Managing branches
- Interactive rebase operations

Common Workflows
----------------

**Cleaning up before PR**:

.. code-block:: bash

   # 1. Fetch latest from upstream
   git fetch upstream
   
   # 2. Rebase on upstream/master
   git rebase upstream/master
   
   # 3. Squash fixup commits
   git rebase -i --autosquash upstream/master
   
   # 4. Force push to your fork
   git push mine branch-name --force-with-lease

**Finding and fixing a commit**:

.. code-block:: bash

   # Find the commit
   git log --oneline --grep="typo"
   
   # Create fixup
   git commit --fixup <commit-hash>
   
   # Auto-squash during rebase
   git rebase -i --autosquash HEAD~10

**Splitting a large commit**:

.. code-block:: bash

   # Start interactive rebase
   git rebase -i HEAD~2
   
   # Mark commit as "edit"
   # Git will stop at that commit
   
   # Reset to previous commit, keep changes
   git reset HEAD~1
   
   # Stage and commit files in smaller groups
   git add <some-files>
   git commit -m "First part"
   git add <other-files>
   git commit -m "Second part"
   
   # Continue rebase
   git rebase --continue

**Undoing the last commit (keep changes)**:

.. code-block:: bash

   git reset --soft HEAD~1

**Undoing the last commit (discard changes)**:

.. code-block:: bash

   git reset --hard HEAD~1

Tips
----

- **Always rebase on upstream/master** before submitting PRs to keep history clean
- **Use fixup commits** for small corrections, then auto-squash before pushing
- **Test after rebase** - rebasing rewrites history and can introduce issues
- **Use ``--force-with-lease``** instead of ``--force`` when pushing rebased branches
- **Keep commits focused** - one logical change per commit
- **Write good commit messages** - follow the project's format (see :doc:`gitworkflow`)
- **Use visual tools** (tig, lazygit) for complex operations - easier than command line
- **Don't rebase shared branches** - only rebase your local feature branches

Additional Resources
--------------------

- :doc:`gitworkflow` - XCSoar Git workflow guidelines
- ``git help <command>`` - Built-in Git documentation
- `Pro Git book <https://git-scm.com/book>`_ - Comprehensive Git reference

