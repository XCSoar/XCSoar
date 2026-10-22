// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SessionState.hpp"

#include "LocalPath.hpp"
#include "LogFile.hpp"
#include "Profile/File.hpp"
#include "Profile/Map.hpp"
#include "system/FileUtil.hpp"
#include "system/Path.hpp"

#include <stdio.h>
#include <utility>

static constexpr char SESSION_FILE[] = "xcsoar-session.txt";
static constexpr std::string_view TAKEOFF_GOTO{"(takeoff)"};

static constexpr double QNH_MIN_HPA = 100;
static constexpr double QNH_MAX_HPA = 1200;
static constexpr double TEMP_MIN_K = 200;
static constexpr double TEMP_MAX_K = 350;

static bool
InOpenRange(double value, double min, double max) noexcept
{
  return value > min && value < max;
}

static bool
SetPolar(SessionState &state, double ballast, double crew,
         double bugs) noexcept
{
  if (state.have_ballast && state.have_crew && state.have_bugs &&
      state.ballast_litres == ballast && state.crew_mass == crew &&
      state.bugs == bugs)
    return false;

  state.ballast_litres = ballast;
  state.crew_mass = crew;
  state.bugs = bugs;
  state.have_ballast = true;
  state.have_crew = true;
  state.have_bugs = true;
  state.modified = true;
  return true;
}

static SessionState session_state;
static bool session_ready = false;

static bool
ValidTicks(std::string_view ticks) noexcept
{
  for (const char c : ticks)
    if (c != '0' && c != '1')
      return false;

  return true;
}

static void
ChecklistKey(char *buffer, std::size_t size,
             unsigned page, const char *field) noexcept
{
  snprintf(buffer, size, "Checklist.%u.%s", page, field);
}

static void
ReadChecklist(const ProfileMap &map, SessionState &state) noexcept
{
  for (unsigned i = 0; i < SESSION_MAX_CHECKLIST_PAGES; ++i) {
    char sum_key[32];
    char ticks_key[32];
    ChecklistKey(sum_key, sizeof(sum_key), i, "Sum");
    ChecklistKey(ticks_key, sizeof(ticks_key), i, "Ticks");

    unsigned sum = 0;
    std::string ticks;
    if (!map.Get(sum_key, sum) || !map.Get(ticks_key, ticks) ||
        !ValidTicks(ticks))
      continue;

    auto &page = state.checklist[i];
    page.present = true;
    page.sum = sum;
    page.ticks = std::move(ticks);
  }
}

static void
WriteChecklist(ProfileMap &map, const SessionState &state) noexcept
{
  if (state.checklist_file.empty())
    return;

  map.Set("ChecklistFile", state.checklist_file);
  map.Set("ChecklistPage", state.checklist_page);

  for (unsigned i = 0; i < SESSION_MAX_CHECKLIST_PAGES; ++i) {
    const auto &page = state.checklist[i];
    if (!page.present)
      continue;

    char sum_key[32];
    char ticks_key[32];
    ChecklistKey(sum_key, sizeof(sum_key), i, "Sum");
    ChecklistKey(ticks_key, sizeof(ticks_key), i, "Ticks");
    map.Set(sum_key, page.sum);
    map.Set(ticks_key, page.ticks);
  }
}

uint32_t
SessionHash(std::string_view text) noexcept
{
  uint32_t hash = 0x811c9dc5u;
  for (unsigned char c : text) {
    hash ^= c;
    hash *= 0x01000193u;
  }
  return hash;
}

bool
SessionState::ShouldRestoreGoto() const noexcept
{
  if (!have_goto || goto_name == TAKEOFF_GOTO)
    return false;

  return goto_id != 0 || have_goto_location;
}

bool
SessionState::ChecklistTicks(unsigned page, uint32_t sum,
                             std::string &out) const noexcept
{
  if (page >= SESSION_MAX_CHECKLIST_PAGES)
    return false;

  const auto &slot = checklist[page];
  if (!slot.present || slot.sum != sum)
    return false;

  out = slot.ticks;
  return true;
}

bool
SessionState::NoteFlying(bool now_flying, double ballast, double crew,
                         double _bugs) noexcept
{
  if (now_flying) {
    const bool edge = !seen_flying || !flying;
    seen_flying = true;
    flying = true;
    const bool polar = SetPolar(*this, ballast, crew, _bugs);
    if (edge)
      modified = true;
    return edge || polar;
  }

  if (!seen_flying || !flying)
    return false;

  flying = false;
  modified = true;
  return true;
}

void
LoadSession(SessionState &state, Path path)
{
  ProfileMap map;
  /* shortcut: LoadFile also migrates AirspaceFile and WPFile keys,
     use a plain read if this file ever carries those names */
  Profile::LoadFile(map, path);

  SessionState loaded;

  unsigned flying = 0;
  if (map.Get("Flying", flying) && flying <= 1)
    loaded.flying = flying == 1;

  if (double litres; map.Get("BallastLitres", litres) && litres >= 0) {
    loaded.ballast_litres = litres;
    loaded.have_ballast = true;
  }

  if (double mass; map.Get("CrewMass", mass) && mass >= 0) {
    loaded.crew_mass = mass;
    loaded.have_crew = true;
  }

  if (double bugs; map.Get("Bugs", bugs) && bugs > 0 && bugs <= 1) {
    loaded.bugs = bugs;
    loaded.have_bugs = true;
  }

  if (double qnh; map.Get("QNH", qnh) &&
      InOpenRange(qnh, QNH_MIN_HPA, QNH_MAX_HPA)) {
    loaded.qnh_hpa = qnh;
    loaded.have_qnh = true;
  }

  if (double kelvin; map.Get("MaxTemp", kelvin) &&
      InOpenRange(kelvin, TEMP_MIN_K, TEMP_MAX_K)) {
    loaded.max_temp_k = kelvin;
    loaded.have_max_temp = true;
  }

  GeoPoint location = GeoPoint::Invalid();
  if (map.GetGeoPoint("AtcReference", location)) {
    loaded.atc_reference = location;
    loaded.have_atc_reference = true;
  }

  if (double degrees; map.Get("AtcDeclination", degrees)) {
    loaded.atc_declination_deg = degrees;
    loaded.have_atc_declination = true;
  }

  if (unsigned id; map.Get("GotoId", id)) {
    loaded.goto_id = id;
    loaded.have_goto = true;
  }

  location = GeoPoint::Invalid();
  if (map.GetGeoPoint("GotoLocation", location)) {
    loaded.goto_location = location;
    loaded.have_goto_location = true;
    loaded.have_goto = true;
  }

  if (double elevation; map.Get("GotoElevation", elevation)) {
    loaded.goto_elevation = elevation;
    loaded.have_goto_elevation = true;
    loaded.have_goto = true;
  }

  if (std::string name; map.Get("GotoName", name)) {
    loaded.goto_name = std::move(name);
    loaded.have_goto = true;
  }

  map.Get("ChecklistFile", loaded.checklist_file);
  if (unsigned page; map.Get("ChecklistPage", page))
    loaded.checklist_page = page;

  ReadChecklist(map, loaded);
  state = std::move(loaded);
}

void
SaveSession(const SessionState &state, Path path)
{
  ProfileMap map;
  map.Set("Flying", state.flying);

  if (state.have_ballast)
    map.Set("BallastLitres", state.ballast_litres);
  if (state.have_crew)
    map.Set("CrewMass", state.crew_mass);
  if (state.have_bugs)
    map.Set("Bugs", state.bugs);
  if (state.have_qnh)
    map.Set("QNH", state.qnh_hpa);
  if (state.have_max_temp)
    map.Set("MaxTemp", state.max_temp_k);

  if (state.have_atc_reference)
    map.SetGeoPoint("AtcReference", state.atc_reference);
  if (state.have_atc_declination)
    map.Set("AtcDeclination", state.atc_declination_deg);

  if (state.have_goto && state.goto_name != TAKEOFF_GOTO) {
    if (state.goto_id != 0)
      map.Set("GotoId", state.goto_id);
    if (state.have_goto_location)
      map.SetGeoPoint("GotoLocation", state.goto_location);
    if (state.have_goto_elevation)
      map.Set("GotoElevation", state.goto_elevation);
    map.Set("GotoName", state.goto_name);
  }

  WriteChecklist(map, state);
  /* SaveFile, not Profile::Save(): that returns when the .prf is clean. */
  Profile::SaveFile(map, path);
}

namespace Session {

SessionState &
State() noexcept
{
  return session_state;
}

void
Load() noexcept
{
  try {
    const auto path = LocalPath(SESSION_FILE);
    if (!File::Exists(path))
      return;

    LoadSession(session_state, path);
  } catch (...) {
    LogError(std::current_exception(), "Failed to load session");
    session_state = {};
  }
}

void
Flush() noexcept
{
  if (!session_state.modified)
    return;

  try {
    SaveSession(session_state, LocalPath(SESSION_FILE));
    session_state.modified = false;
  } catch (...) {
    LogError(std::current_exception(), "Failed to save session");
  }
}

bool
IsReady() noexcept
{
  return session_ready;
}

void
SetReady() noexcept
{
  session_ready = true;
}

void
DeclineRestore() noexcept
{
  if (!session_state.flying)
    return;

  session_state.flying = false;
  session_state.modified = true;
  Flush();
}

static void
NoteValue(bool &have, double &slot, double value) noexcept
{
  if (have && slot == value)
    return;

  slot = value;
  have = true;
  session_state.modified = true;
  Flush();
}

void
NotePolar(double ballast_litres, double crew_mass, double bugs) noexcept
{
  SetPolar(session_state, ballast_litres, crew_mass, bugs);
}

void
NoteQNH(double hpa) noexcept
{
  if (!InOpenRange(hpa, QNH_MIN_HPA, QNH_MAX_HPA))
    return;

  NoteValue(session_state.have_qnh, session_state.qnh_hpa, hpa);
}

void
NoteMaxTemp(double kelvin) noexcept
{
  if (!InOpenRange(kelvin, TEMP_MIN_K, TEMP_MAX_K))
    return;

  NoteValue(session_state.have_max_temp, session_state.max_temp_k, kelvin);
}

void
NoteAtcReference(const GeoPoint &reference) noexcept
{
  if (!reference.IsValid()) {
    ClearAtcReference();
    return;
  }

  auto &state = session_state;
  if (state.have_atc_reference && state.atc_reference == reference)
    return;

  state.atc_reference = reference;
  state.have_atc_reference = true;
  state.modified = true;
  Flush();
}

void
ClearAtcReference() noexcept
{
  auto &state = session_state;
  if (!state.have_atc_reference)
    return;

  state.atc_reference.SetInvalid();
  state.have_atc_reference = false;
  state.modified = true;
  Flush();
}

void
NoteAtcDeclination(double degrees) noexcept
{
  NoteValue(session_state.have_atc_declination,
            session_state.atc_declination_deg, degrees);
}

void
NoteGoto(unsigned id, const GeoPoint &location, bool has_elevation,
         double elevation, std::string_view name) noexcept
{
  if (name == TAKEOFF_GOTO)
    return;

  auto &state = session_state;
  state.have_goto = true;
  state.goto_id = id;
  state.have_goto_location = location.IsValid();
  if (state.have_goto_location)
    state.goto_location = location;
  state.have_goto_elevation = has_elevation;
  if (has_elevation)
    state.goto_elevation = elevation;
  state.goto_name = name;
  state.modified = true;
  Flush();
}

void
ClearGoto() noexcept
{
  auto &state = session_state;
  if (!state.have_goto && state.goto_id == 0 && !state.have_goto_location &&
      state.goto_name.empty())
    return;

  state.have_goto = false;
  state.goto_id = 0;
  state.have_goto_location = false;
  state.goto_location.SetInvalid();
  state.have_goto_elevation = false;
  state.goto_elevation = 0;
  state.goto_name.clear();
  state.modified = true;
  Flush();
}

void
NoteChecklist(std::string_view path, unsigned page,
              std::span<const SessionChecklistNote> pages) noexcept
{
  auto &state = session_state;
  std::array<SessionChecklistPage, SESSION_MAX_CHECKLIST_PAGES> next{};
  unsigned n = pages.size();
  if (n > SESSION_MAX_CHECKLIST_PAGES)
    n = SESSION_MAX_CHECKLIST_PAGES;

  for (unsigned i = 0; i < n; ++i) {
    next[i].present = true;
    next[i].sum = pages[i].sum;
    next[i].ticks = pages[i].ticks;
  }

  if (state.checklist_file == path && state.checklist_page == page &&
      state.checklist == next)
    return;

  state.checklist_file = path;
  state.checklist_page = page;
  state.checklist = std::move(next);
  state.modified = true;
  Flush();
}

void
ClearChecklist() noexcept
{
  auto &state = session_state;
  const std::array<SessionChecklistPage,
                   SESSION_MAX_CHECKLIST_PAGES> empty{};
  if (state.checklist_file.empty() && state.checklist_page == 0 &&
      state.checklist == empty)
    return;

  state.checklist_file.clear();
  state.checklist_page = 0;
  state.checklist = {};
  state.modified = true;
  Flush();
}

}
