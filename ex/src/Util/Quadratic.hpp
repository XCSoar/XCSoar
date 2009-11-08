#ifndef QUADRATIC_HPP
#define QUADRATIC_HPP

class Quadratic {
public:

/** 
 * Constructor for quadratic function x^2+b*x+c=0
 * 
 * @param _b Value of b
 * @param _c Value of c
 */
  Quadratic(const double _b, const double _c):
    da(2),
    b(_b),
    denom(b*b-4.0*_c)
    {};

/** 
 * Constructor for quadratic function a*x^2+b*x+c=0
 * 
 * @param _a Value of a
 * @param _b Value of b
 * @param _c Value of c
 */
  Quadratic(const double _a, const double _b, const double _c):
    da(2*_a),
    b(_b),
    denom(b*b-2*da*_c)
    {};

/** 
 * Check if all solutions of quadratic are real
 * 
 * @return True if quadratic has at least one real solution
 */
  bool check() const {
    if (denom<0) {
      return false;
    }
    if (da==0.0) {
      return false;
    }
    return true;
  }

/** 
 * Returns largest real solution.  Valid only where check() has passed. 
 * 
 * @return greater x value of solutions 
 */
  double solution_max() const {
    return (da>0? solution(true):solution(false));
  }

/** 
 * Returns smallest real solution.  Valid only where check() has passed. 
 * 
 * @return smallest x value of solutions 
 */
  double solution_min() const {
    return (da>0? solution(false):solution(true));
  }

private:

/** 
 * Calculate solution of quadratic equation using relation:
 *   x = (-b +/- sqrt(b^2-4*a*c))/(2*a)
 * 
 * @param positive whether positive or negative sqrt is used
 * 
 * @return x value of solution
 */
  double solution(const bool positive) const {
    return (-b+(positive?sqrt(denom):-sqrt(denom)))/da;
  }
  const double da;
  const double b;
  const double denom;
};


#endif
