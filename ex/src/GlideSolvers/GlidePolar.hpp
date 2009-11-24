#ifndef GLIDEPOLAR_HPP
#define GLIDEPOLAR_HPP

struct GlideState;
struct GlideResult;

/**
 * Class implementing basic glide polar performance model
 * 
 * Implements aircraft-specific glide performance, including
 * bugs/ballast, MacCready setting and cruise efficiency.
 *
 * Cruise efficiency is the ratio of actual cruise speed to 
 * target to the classical MacCready speed.
 * Cruise efficiency is stored in this class for convenience,
 * it is used in MacCready class.
 * 
 * The MacCready class uses this GlidePolar data to calculate
 * specific GlideSolutions. 
 *
 * \todo
 * - currently the polar itself is hard-coded
 * - currently bugs/ballast are ignored
 */

class GlidePolar
{
public:
/** 
 * Constructor.  Performs search for best LD at instantiation.
 * 
 * @param _mc MacCready value at construction
 * @param _bugs Bugs value at construction (currently unimplemented)
 * @param _ballast Ballast value at construction (currently unimplemented)
 */
  GlidePolar(const double _mc,
             const double _bugs,
             const double _ballast);

/** 
 * Accesses best L/D speed
 * 
 * @return Speed of best LD (m/s)
 */
  double get_VbestLD() const {
    return VbestLD;
  }

/** 
 * Accesses best L/D sink rate (positive down)
 * 
 * @return Sink rate at best L/D (m/s)
 */
  double get_SbestLD() const {
    return SbestLD;
  }

/** 
 * Accesses best L/D ratio
 * 
 * @return Best L/D ratio
 */
  double get_bestLD() const {
    return VbestLD/SbestLD;
  }

/** 
 * Set cruise efficiency value.  1.0 = perfect MacCready speed
 * 
 * @param _ce The new cruise efficiency value
 */
  void set_cruise_efficiency(const double _ce) {
    cruise_efficiency = _ce;
  }

/** 
 * Set MacCready value.  Internally this performs search
 * for best LD values corresponding to this setting.
 * 
 * @param _mc The new MacCready ring setting (m/s)
 */
  void set_mc(const double _mc);

/** 
 * Accessor for MC setting
 * 
 * @return The current MacCready ring setting (m/s)
 */
  double get_mc() const {
    return mc;
  }

/** 
 * Sink rate model (actual glide polar) function.
 * 
 * @param V Speed at which sink rate is to be evaluated
 * 
 * @return Sink rate (m/s, positive down)
 */
  double SinkRate(const double V) const;

/** 
 * Sink rate model adjusted by MC setting.  This is used
 * to accomodate speed ring (MC) settings in optimal glide
 * calculations. 
 * 
 * @param V Speed at which sink rate is to be evaluated
 * 
 * @return Sink rate plus MC setting (m/s, positive down)
 */
  double MSinkRate(const double V) const;

/** 
 * Use classical MC theory to compute the optimal glide solution
 * for a given task.
 * 
 * @param task The glide task for which to compute a solution
 * 
 * @return Glide solution
 */
  GlideResult solve(const GlideState &task) const;

/** 
 * Neglecting the actual glide sink rate, calculate the
 * glide solution with an externally supplied sink rate,
 * assuming the glider flies at speeds according to classical
 * MacCready theory.  This is used to calculate the sink rate
 * required for glide-only solutions.
 * 
 * @param task The glide task for which to compute a solution
 * @param S Imposed sink rate
 * 
 * @return Glide solution for the virtual task
 */
  GlideResult solve_sink(const GlideState &task,
                          const double S) const;

/** 
 * Quickly determine whether a task is achievable without 
 * climb, assuming favorable wind.  This can be used to quickly
 * pre-filter waypoints for arrival altitude before performing
 * expensive optimal glide solution searches.
 * 
 * @param task The glide task for which to estimate a solution
 * 
 * @return True if a glide solution is feasible (optimistically)
 */
  bool possible_glide(const GlideState &task) const;

private:
/** 
 * Solve for best LD at current MC/bugs/ballast setting.
 */
  void solve();

  double mc;                  
  double bugs;
  double ballast;
  double cruise_efficiency;
  double VbestLD;
  double SbestLD;

  /** @link dependency */
  /*#  MacCready lnkMacCready; */
};

#endif

