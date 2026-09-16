// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

class Path;

/**
 * Make an existing IGC file fit to be appended to, or refuse it.
 *
 * A Logger Session cut by a power failure can end mid-record.  Appending to
 * such a file would leave the fragment in the middle, where it would later
 * be signed as though it were a record.  Truncating to the last newline
 * repairs that, and is sufficient: IGCWriter::CommitLine() writes each
 * record together with its newline, so a trailing newline proves the last
 * record is complete.
 *
 * Every line is then checked.  Passing G-record verification is *not*
 * evidence that a file is clean -- the hash is computed over whatever the
 * line reader returns, so a corrupt fragment would be hashed as a record and
 * the resulting signature would verify.  Validity has to be established
 * separately.
 *
 * Throws std::runtime_error if the file cannot be made fit to append to, in
 * which case the caller should start a new file rather than risk the
 * existing one.
 */
void
RepairIgcForAppend(Path path);
