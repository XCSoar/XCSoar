// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "MessagingDatabase.hpp"
#include "Id.hpp"

#include <string_view>
#include <functional>

void
FlarmMessagingDatabase::Insert(const MessagingRecord &record) noexcept
{
  if (!record.id.IsDefined())
    return;

  const std::lock_guard<SharedMutex> lock(mutex);
  map.insert(std::make_pair(record.id, record));
}

void
FlarmMessagingDatabase::Update(const MessagingRecord &record) noexcept
{
  // Get or create the existing record
  if (!record.id.IsDefined())
    return;

  const std::lock_guard<SharedMutex> lock(mutex);

  MessagingRecord &existing = map.try_emplace(record.id, MessagingRecord{}).first->second;
  existing.id = record.id;

  // Frequency updates bypass cycle detection: frequency fields are
  // independent of the message cycle and don't trigger epoch transitions
  if (record.frequency.IsDefined()) {
    existing.frequency = record.frequency;
    return;  
  }

  // Identify the single incoming field and copy its value
  MessagingRecord::Field field = MessagingRecord::Field::Count;
  for (uint8_t i = 0; i < MessagingRecord::GetFieldCount(); ++i) {
    auto f = static_cast<MessagingRecord::Field>(i);
    if (!record.GetFieldValueConst(f).empty()) {
      field = f;
      existing.GetFieldValue(f) = record.GetFieldValueConst(f);
      break;
    }
  }

  if (field == MessagingRecord::Field::Count)
    return;  // No field to process

  /*
    Epoch-based cycle detection: repeating the same field within the
    current epoch marks the boundary to a new cycle.
  */ 
  const uint8_t prev_epoch = existing.epoch;
  uint8_t &seen_epoch = existing.GetFieldEpoch(field);
  // "Repeat in current epoch" marks a cycle boundary: we've seen this field
  // before in the current epoch, indicating the transmitter has begun a new
  // message cycle (essential for detecting cycle resets)
  const bool repeat_in_current_epoch = (seen_epoch == prev_epoch && prev_epoch != 0);

  if (repeat_in_current_epoch) {
    // Clear any field that wasn't seen in the previous epoch. This removes
    // stale data from fields not transmitted in the current cycle, ensuring
    // only fields from the active cycle are retained
    for (uint8_t i = 0; i < MessagingRecord::GetFieldCount(); ++i) {
      auto f = static_cast<MessagingRecord::Field>(i);
      if (existing.GetFieldEpoch(f) != prev_epoch)
        existing.GetFieldValue(f).clear();
    }

    // Advance epoch; keep 0 reserved as "never seen" sentinel. The value 0
    // indicates a field has not been transmitted in any cycle, distinguishing
    // "never transmitted" from "seen in a previous cycle"
    uint8_t next = static_cast<uint8_t>(prev_epoch + 1);
    if (next == 0) next = 1; // wrap-around protection: skip sentinel value
    existing.epoch = next;
  }

  // Mark this field as seen in the (possibly advanced) epoch
  seen_epoch = existing.epoch;
}

std::optional<MessagingRecord>
FlarmMessagingDatabase::FindRecordById(FlarmId id) const noexcept
{
  const std::shared_lock<SharedMutex> lock(mutex);

  auto i = map.find(id);
  if (i == map.end())
    return std::nullopt;

  return i->second;
}

size_t
FlarmMessagingDatabase::ComputeHashLocked() const noexcept
{
  const std::hash<std::string_view> hasher;
  size_t hash = 0;

  // Use boost::hash_combine algorithm for better distribution and collision resistance
  auto hash_combine = [](size_t seed, size_t value) noexcept {
    return seed ^ (value + 0x9e3779b9 + (seed << 6) + (seed >> 2));
  };

  for (const auto &pair : map) {
    const auto &record = pair.second;
    char id_buf[16];
    hash = hash_combine(hash, hasher(std::string_view(record.id.Format(id_buf))));
    hash = hash_combine(hash, hasher(std::string_view(record.pilot)));
    hash = hash_combine(hash, hasher(std::string_view(record.registration)));
    hash = hash_combine(hash, hasher(std::string_view(record.plane_type)));
    hash = hash_combine(hash, hasher(std::string_view(record.callsign)));
  }

  return hash;
}

size_t
FlarmMessagingDatabase::ComputeHash() const noexcept
{
  const std::shared_lock<SharedMutex> lock(mutex);
  return ComputeHashLocked();
}

bool
FlarmMessagingDatabase::HasUpdates() const noexcept
{
  const std::shared_lock<SharedMutex> lock(mutex);
  return ComputeHashLocked() != last_saved_hash;
}

void
FlarmMessagingDatabase::MarkSaved() noexcept
{
  const std::lock_guard<SharedMutex> lock(mutex);
  last_saved_hash = ComputeHashLocked();
}
