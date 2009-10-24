#ifndef MACCREADY_HPP
#define MACCREADY_HPP

#include <iostream>

struct GLIDE_STATE;
struct GLIDE_RESULT;

class MacCready 
{
public:
  MacCready():mc(0.0),
              cruise_efficiency(1.0) {

  }
  
  GLIDE_RESULT solve(const GLIDE_STATE &task) const;

  GLIDE_RESULT solve_glide(const GLIDE_STATE &task,
			   const double V) const;

  GLIDE_RESULT solve_glide_zerowind(const GLIDE_STATE &task,
                                    const double V) const;

  GLIDE_RESULT solve_glide(const GLIDE_STATE &task,
			   const double V,
                           const double S) const;

  GLIDE_RESULT solve_sink(const GLIDE_STATE &task,
                          const double S) const;

  void set_mc(double _mc);

  double get_mc() const {
    return mc;
  }

  void set_cruise_efficiency(double _ef) {
    cruise_efficiency = _ef;
  }

  double get_cruise_efficiency() const {
    return cruise_efficiency;
  }

private:
  GLIDE_RESULT optimise_glide(const GLIDE_STATE &task) const;

  GLIDE_RESULT solve_vertical(const GLIDE_STATE &task) const;

  GLIDE_RESULT solve_cruise(const GLIDE_STATE &task) const;

  double SinkRate(const double V) const;

  double cruise_bearing(const double V, const double Wn, 
                        const double theta) const;

  void solve_vopt();
  double mc;
  double VOpt;
  double SOpt;
  double cruise_efficiency;
};


#endif
