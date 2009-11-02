#include "TaskMacCready.hpp"


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
    GlideResult gr = tp_solution(i, aircraft_predict, minHs[i]);
    if (minHs[i]<minHs[i-1]) {
      double dh = aircraft_predict.Altitude-gr.HeightGlide;
      if (minHs[i]+0.01<dh) {
        minHs[i] = dh;
        i--; 
        continue; // recalculate again for remainder
      }
    }
  }
}


GlideResult 
TaskMacCready::glide_solution(const AIRCRAFT_STATE &aircraft) 
{
  GlideResult acc_gr, gr;
  AIRCRAFT_STATE aircraft_predict = aircraft;
  AIRCRAFT_STATE aircraft_start = get_aircraft_start(aircraft);

  clearance_heights(aircraft);

  double excess_height = 0.0;
  excess_height = aircraft_start.Altitude-minHs[end];

  for (int i=end; i>=start; i--) {
    if (i>start) {
      aircraft_predict.Altitude = minHs[i-1]+std::max(excess_height,0.0);
    } else {
      aircraft_predict.Altitude = std::min(aircraft_start.Altitude,
                                           minHs[i]+std::max(excess_height,0.0));
    }

    // perform estimate, ensuring that alt is above previous taskpoint  
    gr = tp_solution(i,aircraft_predict, minHs[i]);
    gs[i] = gr;
    excess_height -= gr.HeightGlide;

    // update state
    if (i==end) {
      acc_gr = gr;
    } else if (i>start) {
      acc_gr.add(gr);
    }

    if (gr.Solution != GlideResult::RESULT_OK) {      
      return gr;
    }

  }
  if (end>start) {
    gr.add(acc_gr);
  }

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
  gr.calc_cruise_bearing();
  return gr;
}


GlideResult 
TaskMacCready::glide_sink(const AIRCRAFT_STATE &aircraft,
                          const double S) 
{
  AIRCRAFT_STATE aircraft_predict = aircraft;
  GlideResult acc_gr;
  for (int i=start; i<=end; i++) {
    GlideResult gr = tp_sink(i, aircraft_predict, S);

    aircraft_predict.Altitude -= gr.HeightGlide;
    if (i==start) {
      acc_gr = gr;
    } else {
      acc_gr.AltitudeDifference = std::min(acc_gr.AltitudeDifference,
                                           gr.AltitudeDifference);
    }
  }
  return acc_gr;
}

GlideResult 
TaskMacCready::tp_sink(const unsigned i,
                       const AIRCRAFT_STATE &aircraft, 
                       const double S) const
{
  return tps[i]->glide_solution_sink(aircraft, glide_polar, S);
}
