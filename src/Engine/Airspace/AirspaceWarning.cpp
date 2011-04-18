/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "AirspaceWarning.hpp"

#include <assert.h>
#include <limits.h>

const unsigned AirspaceWarning::null_acktime = UINT_MAX;

AirspaceWarning::AirspaceWarning(const AbstractAirspace& the_airspace):
  m_airspace(the_airspace),
  m_state(WARNING_CLEAR),
  m_state_last(WARNING_CLEAR),
  m_acktime_warning(0),
  m_acktime_inside(0),
  m_debouncetime(60),
  m_ack_day(false),
  m_expired(true),
  m_expired_last(true)
{
}

void AirspaceWarning::save_state()
{
  m_state_last = m_state;
  m_state = WARNING_CLEAR;
  m_expired_last = m_expired;
}

void 
AirspaceWarning::update_solution(const AirspaceWarningState state,
                                 AirspaceInterceptSolution& solution)
{
  if (state_accepted(state)) {
    m_state = state;
    m_solution = solution;
  }
}


bool
AirspaceWarning::warning_live(const unsigned ack_time, const unsigned dt)
{
  // propagate settings from manager
  if (m_acktime_warning == null_acktime)
    m_acktime_warning = ack_time;

  if (m_acktime_inside == null_acktime)
    m_acktime_inside = ack_time;

  if ((m_state != WARNING_CLEAR) 
      && (m_state < m_state_last) 
      && (m_state_last == WARNING_INSIDE))
    // if inside was acknowledged, consider warning to be acknowledged
    m_acktime_warning = max(m_acktime_warning, m_acktime_inside);

  if (m_acktime_warning > dt)
    m_acktime_warning-= dt;
  else
    m_acktime_warning = 0;

  if (m_acktime_inside > dt)
    m_acktime_inside-= dt;
  else
    m_acktime_inside = 0;

  if (m_debouncetime > dt)
    m_debouncetime-= dt;
  else
    m_debouncetime = 0;

  m_expired = get_ack_expired();

  if (m_state == WARNING_CLEAR)
    return !m_expired;

  return true;
}

bool
AirspaceWarning::changed_state() const
{
  if (m_expired > m_expired_last) 
    return true;

  if ((m_state_last == WARNING_CLEAR) && (m_state > WARNING_CLEAR)) 
    return get_ack_expired();

  if ((m_state_last < WARNING_INSIDE) && (m_state == WARNING_INSIDE))
    return get_ack_expired();

  return false;
}

bool 
AirspaceWarning::state_accepted(const AirspaceWarningState state) const
{
  return (state >= m_state);
}

bool
AirspaceWarning::get_ack_expired() const
{
  if (m_ack_day)
    // these ones persist
    return false;

  switch (m_state) {
  case WARNING_CLEAR:
  case WARNING_TASK:
  case WARNING_FILTER:
  case WARNING_GLIDE:
    return !m_acktime_warning;
  case WARNING_INSIDE:
    return !m_acktime_inside;
  };
  // unknown, should never get here
  assert(1);
  return true;
}

void 
AirspaceWarning::acknowledge_inside(const bool set)
{
  if (set)
    m_acktime_inside = null_acktime;
  else
    m_acktime_inside = 0;
}

void 
AirspaceWarning::acknowledge_warning(const bool set)
{
  if (set)
    m_acktime_warning = null_acktime;
  else
    m_acktime_warning = 0;
}

void 
AirspaceWarning::acknowledge_day(const bool set)
{
  m_ack_day = set;
}

bool 
AirspaceWarning::get_ack_day() const
{
  return m_ack_day;
}

bool 
AirspaceWarning::trivial() const 
{
  return (m_state == WARNING_CLEAR)
    && (m_state_last == WARNING_CLEAR)
    && get_ack_expired()
    && (!m_debouncetime);
}

bool 
AirspaceWarning::operator<(const AirspaceWarning &other) const
{
  // compare bother.ack
  if (get_ack_expired() != other.get_ack_expired())
    // least expired top
    return get_ack_expired() > other.get_ack_expired();

  // compare bother.state
  if (get_warning_state() != other.get_warning_state())
    // most severe top
    return get_warning_state() > other.get_warning_state();

  // state and ack equal, compare bother.time to intersect
  return get_solution().elapsed_time < other.get_solution().elapsed_time;
}
