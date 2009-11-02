#ifndef GLIDEPOLAR_HPP
#define GLIDEPOLAR_HPP

struct GlideState;
struct GlideResult;

class GlidePolar
{
public:
  GlidePolar(const double _mc,
             const double _bugs,
             const double _ballast);

  double get_VbestLD() const {
    return VbestLD;
  }
  double get_SbestLD() const {
    return SbestLD;
  }
  double get_bestLD() const {
    return VbestLD/SbestLD;
  }
  void set_cruise_efficiency(const double _ce) {
    cruise_efficiency = _ce;
  }
  void set_mc(const double _mc);
  double get_mc() const {
    return mc;
  }
  double SinkRate(const double V) const;
  double MSinkRate(const double V) const;

  GlideResult solve(const GlideState &task) const;

  GlideResult solve_sink(const GlideState &task,
                          const double S) const;

private:
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

