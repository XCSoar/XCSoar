#include "AirspaceWarning.hpp"

void AirspaceWarning::save_state()
{
  m_state_last = m_state;
  m_state = WARNING_CLEAR;
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
AirspaceWarning::action_updates()
{
  if (m_state != WARNING_CLEAR) {
    return true;
  }

  return false;
}

bool
AirspaceWarning::changed_state() const
{
  return (m_state != m_state_last);
}

bool 
AirspaceWarning::state_accepted(const AirspaceWarningState state) const
{
  return (state>= m_state);
}
