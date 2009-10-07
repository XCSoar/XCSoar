#include "Navigation/Aircraft.hpp"
#include "GlideSolvers/MacCready.hpp"
#include <algorithm>

void AIRCRAFT_STATE::back_predict(const GLIDE_RESULT &res)
{
  Altitude -= res.HeightGlide;
}

// 
// start height is aircraft height
// min h for this stage is max of min heights remaining
// this will ensure final glide gets pushed to end and only used
// when it will allow terrain clearance
//
// accumulate HeightClimb along the way, result is the height below fg
void AIRCRAFT_STATE::forward_predict(const GLIDE_RESULT &res)
{
  Altitude -= res.HeightGlide;
}
