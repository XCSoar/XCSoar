#ifndef XCSOAR_MATH_GEOMETRY_HPP
#define XCSOAR_MATH_GEOMETRY_HPP

void irotate(int &xin, int &yin, const double &angle);
void irotatescale(int &xin, int &yin, const double &angle, const double &scale,
                  double &x, double &y);
void rotate(double &xin, double &yin, const double &angle);
void frotate(float &xin, float &yin, const float &angle);
void rotatescale(double &xin, double &yin,
                 const double &angle, const double &scale);
void frotatescale(float &xin, float &yin,
                  const float &angle, const float &scale);

bool AngleInRange(double Angle0, double Angle1, double x, bool is_signed=false);
double AngleLimit180(double theta);
double AngleLimit360(double theta);

double Reciprocal(double InBound);
double BiSector(double InBound, double OutBound);
double HalfAngle(double Angle0, double Angle1);

#endif
