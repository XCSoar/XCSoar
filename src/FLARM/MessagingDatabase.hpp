// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Id.hpp"
#include "MessagingRecord.hpp"
#include "thread/SharedMutex.hpp"

#include <map>
#include <optional>

class BufferedOutputStream;

class FlarmMessagingDatabase {
  friend void SaveFlarmMessagingFile(BufferedOutputStream &, FlarmMessagingDatabase &);

private:
  using RecordMap = std::map<FlarmId, MessagingRecord>;
  mutable SharedMutex mutex;
  RecordMap map;
  size_t last_saved_hash = 0;

public:
  /**
   * Finds a MessagingRecord snapshot for the given FLARM id (thread-safe)
   * @param id FLARM id
   * @return Optional MessagingRecord snapshot
   */
  std::optional<MessagingRecord> FindRecordById(FlarmId id) const noexcept;

  void Insert(const MessagingRecord &record) noexcept;

  /**
   * Update a PFLAM messaging record with cycle boundary detection.
   * Automatically handles field updates and clears stale fields when
   * a new cycle is detected.
   */
  void Update(const MessagingRecord &record) noexcept;

  /**
   * Compute hash of database content for change detection
   */
  size_t ComputeHash() const noexcept;

  /**
   * Check if database has been updated since last save
   */
  bool HasUpdates() const noexcept;

  /**
   * Mark database as saved (update the saved hash)
   */
  void MarkSaved() noexcept;

private:
  size_t ComputeHashLocked() const noexcept;
};
