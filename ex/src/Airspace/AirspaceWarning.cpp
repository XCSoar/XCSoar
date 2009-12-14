#include "AirspaceWarning.hpp"

void AirspaceWarning::save_state()
{
  m_state_last = m_state;
  m_state = WARNING_CLEAR;
}

void 
AirspaceWarning::update_solution(AirspaceWarningState state,
                                 AirspaceInterceptSolution& solution)
{
  if (state_upgraded(state)) {
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
AirspaceWarning::state_upgraded(AirspaceWarningState state)
{
  return (state>= m_state);
}
