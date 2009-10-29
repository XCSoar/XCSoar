#ifndef MACCREADY_HPP
#define MACCREADY_HPP

#include <iostream>

struct GLIDE_STATE;
struct GLIDE_RESULT;
class GlidePolar;

class MacCready 
{
public:
  MacCready(const GlidePolar &_glide_polar,
            const double cruise_efficiency);

  GLIDE_RESULT solve_sink(const GLIDE_STATE &task,
                          const double S) const;

  GLIDE_RESULT solve(const GLIDE_STATE &task) const;

  GLIDE_RESULT solve_glide(const GLIDE_STATE &task,
			   const double V) const;

  double get_mc() const;

private:

  GLIDE_RESULT solve_glide_zerowind(const GLIDE_STATE &task,
                                    const double V) const;

  GLIDE_RESULT solve_glide(const GLIDE_STATE &task,
			   const double V,
                           const double S) const;

  GLIDE_RESULT optimise_glide(const GLIDE_STATE &task) const;

  GLIDE_RESULT solve_vertical(const GLIDE_STATE &task) const;

  GLIDE_RESULT solve_cruise(const GLIDE_STATE &task) const;

  const GlidePolar &glide_polar;
  const double cruise_efficiency;

  /** @link dependency */
  /*#  MacCreadyVopt lnkMacCreadyVopt; */

  /** @link dependency */
  /*#  GlideQuadratic lnkGlideQuadratic; */
};


#endif
