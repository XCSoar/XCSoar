#include "AirspaceWarning.hpp"

AirspaceWarning::AirspaceWarning(const AbstractAirspace& the_airspace):
  m_airspace(the_airspace),
  m_state(WARNING_CLEAR),
  m_state_last(WARNING_CLEAR),
  m_acktime_warning(0),
  m_acktime_inside(0),
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
AirspaceWarning::warning_live()
{
  if ((m_state != WARNING_CLEAR) 
      && (m_state < m_state_last) 
      && (m_state_last == WARNING_INSIDE)) 
  {
    // if inside was acknowledged, consider warning to be acknowledged
    m_acktime_warning = max(m_acktime_warning, m_acktime_inside);
  }

  if (m_acktime_warning) {
    m_acktime_warning--;
  }
  if (m_acktime_inside) {
    m_acktime_inside--;
  }

  m_expired = get_ack_expired();

  if (m_state == WARNING_CLEAR) {
    return !m_expired;
  } else { 
    return true;
  }
}

bool
AirspaceWarning::changed_state() const
{
  if (m_expired > m_expired_last) 
    return true;

  if ((m_state_last == WARNING_CLEAR) && (m_state > WARNING_CLEAR)) 
    return get_ack_expired();

  if ((m_state_last < WARNING_INSIDE) && (m_state == WARNING_INSIDE)) {
    return get_ack_expired();
  }
  if ((m_state_last < WARNING_GLIDE) && (m_state == WARNING_GLIDE)) {
    return true;
  }

  return false;
}

bool 
AirspaceWarning::state_accepted(const AirspaceWarningState state) const
{
  return (state>= m_state);
}

bool
AirspaceWarning::get_ack_expired() const
{
  if (m_ack_day) {
    return false; // these ones persist
  }
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
  return true;
}

void 
AirspaceWarning::acknowledge_inside(const bool set)
{
  if (set) {
    m_acktime_inside = 60;
  } else {
    m_acktime_inside = 0;
  }
}

void 
AirspaceWarning::acknowledge_warning(const bool set)
{
  if (set) {
    m_acktime_warning = 60;
  } else {
    m_acktime_warning = 0;
  }
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
  return (m_state==WARNING_CLEAR) && (m_state_last==WARNING_CLEAR) && get_ack_expired();
}
