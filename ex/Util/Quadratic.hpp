#ifndef QUADRATIC_HPP
#define QUADRATIC_HPP

class Quadratic {
public:
  Quadratic(const double _b, const double _c):
    da(2),
    b(_b),
    denom(b*b-4.0*_c)
    {};
  Quadratic(const double _a, const double _b, const double _c):
    da(2*_a),
    b(_b),
    denom(b*b-2*da*_c)
    {};
  bool check() const {
    if (denom<0) {
      return false;
    }
    if (da==0.0) {
      return false;
    }
    return true;
  }
  double solution_max() const {
    return (da>0? solution(true):solution(false));
  }
  double solution_min() const {
    return (da>0? solution(false):solution(true));
  }
private:
  double solution(const bool positive) const {
    return (-b+(positive?sqrt(denom):-sqrt(denom)))/da;
  }
  const double da;
  const double b;
  const double denom;
};


#endif
