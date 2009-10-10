#include "GlideSolvers/TaskMacCready.hpp"

TaskMacCreadyTravelled::TaskMacCreadyTravelled(const std::vector<OrderedTaskPoint*> &_tps,
                                               const unsigned _activeTaskPoint,
                                               const double _mc):
  TaskMacCready(_tps,_activeTaskPoint, _mc)
{
  start = 0;
  end = activeTaskPoint;
}

TaskMacCreadyRemaining::TaskMacCreadyRemaining(const std::vector<OrderedTaskPoint*> &_tps,
                                               const unsigned _activeTaskPoint,
                                               const double _mc):
  TaskMacCready(_tps,_activeTaskPoint, _mc)
{
  start = activeTaskPoint;
  end = tps.size()-1;
}

GLIDE_RESULT 
TaskMacCreadyRemaining::tp_solution(const unsigned i,
                                    const AIRCRAFT_STATE &aircraft, 
                                    double minH)
{
  return tps[i]->glide_solution_remaining(aircraft, msolv, minH);
}

GLIDE_RESULT 
TaskMacCreadyTravelled::tp_solution(const unsigned i,
                                    const AIRCRAFT_STATE &aircraft, 
                                    double minH)
{
  return tps[i]->glide_solution_travelled(aircraft, msolv, minH);
}

const AIRCRAFT_STATE 
TaskMacCreadyRemaining::get_aircraft_start(const AIRCRAFT_STATE &aircraft)
{
  return aircraft;

}

const AIRCRAFT_STATE 
TaskMacCreadyTravelled::get_aircraft_start(const AIRCRAFT_STATE &aircraft)
{
  if (tps[0]->has_entered()) {
    return tps[0]->get_state_entered();
  } else {
    return aircraft;
  }
}


///////////////

void
TaskMacCready::clearance_heights(const AIRCRAFT_STATE &aircraft)
{
  // set min heights (earliest climb possible)
  double minH = get_min_height(aircraft);
  for (int i=end; i>=start; i--) {
    minH = std::max(minH,tps[i]->getElevation());
    minHs[i] = minH;
  }
  // set min heights (ensure clearance possible for latest glide)
  for (int i=start+1; i<=end; i++) {
    AIRCRAFT_STATE aircraft_predict = aircraft;
    aircraft_predict.Altitude = minHs[i-1];
    GLIDE_RESULT gr = tp_solution(i, aircraft_predict, minHs[i]);
    if (minHs[i]<minHs[i-1]) {
      double dh = aircraft_predict.Altitude-gr.HeightGlide;
      if (minHs[i]<dh) {
        minHs[i] = dh;
        i--; 
        continue; // recalculate again for remainder
      }
    }
  }
}


GLIDE_RESULT 
TaskMacCready::glide_solution(const AIRCRAFT_STATE &aircraft)
{
  GLIDE_RESULT acc_gr, gr;
  AIRCRAFT_STATE aircraft_predict = aircraft;
  AIRCRAFT_STATE aircraft_start = get_aircraft_start(aircraft);

  clearance_heights(aircraft);

  double glide_height = 0.0;
  glide_height = aircraft_start.Altitude-minHs[end];

  for (int i=end; i>=start; i--) {
//    printf("glide_height %g\n",glide_height);
    if (i>start) {
      aircraft_predict.Altitude = minHs[i-1]+std::max(glide_height,0.0);
    } else {
      aircraft_predict.Altitude = std::min(aircraft_start.Altitude,
                                           minHs[i]+std::max(glide_height,0.0));
    }

    // perform estimate, ensuring that alt is above previous taskpoint  
    gr = tp_solution(i,aircraft_predict, minHs[i]);
    gs[i] = gr;
    glide_height -= gr.HeightGlide;

    // update state
    if (i==end) {
      acc_gr = gr;
    } else if (i>start) {
      acc_gr.add(gr);
    }

    if (gr.Solution != MacCready::RESULT_OK) {      
      return gr;
    }

  }
  gr.add(acc_gr);

  aircraft_predict.Altitude = aircraft_start.Altitude;
  double alt_difference = aircraft_start.Altitude-minHs[start];
  for (int i=start; i<=end; i++) {
    aircraft_predict.Altitude -= gs[i].HeightGlide;
    alt_difference = std::min(alt_difference, aircraft_predict.Altitude-minHs[i]);
  }
  if (gr.HeightClimb>0) {
    alt_difference = -gr.HeightClimb;
  }
  gr.AltitudeDifference = alt_difference;
  return gr;
}


void
TaskMacCready::report(const AIRCRAFT_STATE &aircraft)
{
  AIRCRAFT_STATE aircraft_start = get_aircraft_start(aircraft);
  AIRCRAFT_STATE aircraft_predict = aircraft;
  aircraft_predict.Altitude = aircraft_start.Altitude;
  printf("  i alt  min\n");
  printf("  %d %4.2f\n", start, aircraft_start.Altitude);
  for (int i=start; i<=end; i++) {
    aircraft_predict.Altitude -= gs[i].HeightGlide;
    printf("  %d %4.2f %4.2f\n", i, aircraft_predict.Altitude,minHs[i]);
  }
  printf("\n");
}
