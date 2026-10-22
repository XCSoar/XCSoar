// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Geo/GeoPoint.hpp"

#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>

class Path;

/** Pages kept in the session file. Matches the checklist dialog. */
static constexpr unsigned SESSION_MAX_CHECKLIST_PAGES = 32;

/**
 * One checklist page in #SessionState.  @c sum is FNV-1a of the page
 * text.  @c ticks is @c 0 and @c 1 in checkbox order.
 */
struct SessionChecklistPage {
  bool present = false;
  uint32_t sum = 0;
  std::string ticks;

  bool operator==(const SessionChecklistPage &) const noexcept = default;
};

/**
 * A page handed to Session::NoteChecklist().
 * Views stay valid for the call.
 */
struct SessionChecklistNote {
  uint32_t sum = 0;
  std::string_view ticks;
};

/**
 * Flight state that must not live in a shareable @c .prf.
 * Restored on startup only when @c flying is set.
 */
struct SessionState {
  bool modified = false;

  /** Set after this process has seen @c flying true.  Not stored. */
  bool seen_flying = false;

  bool flying = false;

  bool have_ballast = false;
  bool have_crew = false;
  bool have_bugs = false;
  double ballast_litres = 0;
  double crew_mass = 0;
  double bugs = 1;

  /** QNH in hPa. */
  bool have_qnh = false;
  double qnh_hpa = 0;

  /** Forecast maximum temperature in Kelvin. */
  bool have_max_temp = false;
  double max_temp_k = 0;

  bool have_atc_reference = false;
  GeoPoint atc_reference = GeoPoint::Invalid();
  bool have_atc_declination = false;
  double atc_declination_deg = 0;

  bool have_goto = false;
  unsigned goto_id = 0;
  bool have_goto_location = false;
  GeoPoint goto_location = GeoPoint::Invalid();
  bool have_goto_elevation = false;
  double goto_elevation = 0;
  std::string goto_name;

  std::string checklist_file;
  unsigned checklist_page = 0;
  std::array<SessionChecklistPage, SESSION_MAX_CHECKLIST_PAGES> checklist{};

  [[nodiscard]] bool ShouldRestoreGoto() const noexcept;

  /**
   * Copy ticks for @p page when the text hash matches.
   * @return false when the page is absent or the text changed
   */
  [[nodiscard]] bool ChecklistTicks(unsigned page, uint32_t sum,
                                    std::string &ticks) const noexcept;

  /**
   * @return true when the persisted flying flag or polar snapshot changed
   */
  bool NoteFlying(bool now_flying, double ballast_litres, double crew_mass,
                  double bugs) noexcept;
};

[[nodiscard]] uint32_t
SessionHash(std::string_view text) noexcept;

/**
 * Replace @p state with the file.  Throws on I/O error.
 * A missing key leaves that field at its default.
 */
void
LoadSession(SessionState &state, Path path);

/** Throws on I/O error.  Does not clear @c modified. */
void
SaveSession(const SessionState &state, Path path);

namespace Session {

SessionState &
State() noexcept;

/** Load @c xcsoar-session.txt.  A missing file keeps defaults. */
void
Load() noexcept;

/** Write the file when @c modified is set. */
void
Flush() noexcept;

/** ProcessTimer may call this after startup has applied the file. */
[[nodiscard]] bool
IsReady() noexcept;

void
SetReady() noexcept;

/** Forget the in-flight snapshot.  The next start will not ask. */
void
DeclineRestore() noexcept;

/** Record the live polar.  Does not write the file. */
void
NotePolar(double ballast_litres, double crew_mass, double bugs) noexcept;

/** Record QNH in hPa.  Writes the file when the value changes. */
void
NoteQNH(double hpa) noexcept;

/** Record the forecast maximum temperature in Kelvin. */
void
NoteMaxTemp(double kelvin) noexcept;

void
NoteAtcReference(const GeoPoint &reference) noexcept;

void
ClearAtcReference() noexcept;

void
NoteAtcDeclination(double degrees) noexcept;

void
NoteGoto(unsigned id, const GeoPoint &location, bool has_elevation,
         double elevation, std::string_view name) noexcept;

void
ClearGoto() noexcept;

void
NoteChecklist(std::string_view path, unsigned page,
              std::span<const SessionChecklistNote> pages) noexcept;

void
ClearChecklist() noexcept;

}
