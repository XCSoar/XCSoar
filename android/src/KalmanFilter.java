// Pressure Altimeter - a pressure altimeter for Android devices
// written by Tom Stepleton <stepleton@gmail.com> in early 2012
// released into the public domain

// A Kalman filter that maintains estimates of a quantity and its first
// derivative given measurements of that quantity over time. The notation,
// the arithmetic, and the model itself borrow from the "truck" example on
// this version of the Wikipedia page for Kalman filters:
//   http://en.wikipedia.org/w/index.php?title=Kalman_filter&oldid=484054295<

package org.xcsoar;

public class KalmanFilter {
  // The state we are tracking, namely:
  private double x_abs_;  // The absolute value of x.
  private double x_vel_;  // The rate of change of x.

  // Covariance matrix for the state.
  private double p_abs_abs_;
  private double p_abs_vel_;
  private double p_vel_vel_;

  // The variance of the acceleration noise input in the system model.
  private double var_accel_;

  // Constructor. Assumes a variance of 1.0 for the system model's
  // acceleration noise input, in units per second squared.
  public KalmanFilter() {
    setAccelerationVariance(1.);
    reset();
  }

  // Constructor. Caller supplies the variance for the system model's
  // acceleration noise input, in units per second squared.
  public KalmanFilter(double var_accel) {
    setAccelerationVariance(var_accel);
    reset();
  }

  // The following three methods reset the filter. All of them assign a huge
  // variance to the tracked absolute quantity and a var_accel_ variance to
  // its derivative, so the very next measurement will essentially be
  // copied directly into the filter. Still, we provide methods that allow
  // you to specify initial settings for the filter's tracked state.

  public void reset() {
    reset(0., 0.);
  }

  public void reset(double abs_value) {
    reset(abs_value, 0.);
  }

  public void reset(double abs_value, double vel_value) {
    x_abs_ = abs_value;
    x_vel_ = vel_value;
    p_abs_abs_ = 1.e10;
    p_abs_vel_ = 0.;
    p_vel_vel_ = var_accel_;
  }

  // Sets the variance for the acceleration noise input in the system model,
  // in units per second squared.
  public void setAccelerationVariance(double var_accel) {
    var_accel_ = var_accel;
  }

  // Updates state given a sensor measurement of the absolute value of x,
  // the variance of that measurement, and the interval since the last
  // measurement in seconds. This interval must be greater than 0; for the
  // first measurement after a reset(), it's safe to use 1.0.
  public void update(double z_abs, double var_z_abs, double dt) {
    // Note: math is not optimized by hand. Let the compiler sort it out.
    // Predict step.
    // Update state estimate.
    x_abs_ += x_vel_ * dt;
    // Update state covariance. The last term mixes in acceleration noise.
    p_abs_abs_ += 2.*dt*p_abs_vel_ + dt*dt*p_vel_vel_ + var_accel_*dt*dt*dt*dt/4.;
    p_abs_vel_ +=                       dt*p_vel_vel_ + var_accel_*dt*dt*dt/2.;
    p_vel_vel_ +=                                     + var_accel_*dt*dt;

    // Update step.
    double y = z_abs - x_abs_;  // Innovation.
    double s_inv = 1. / (p_abs_abs_ + var_z_abs);  // Innovation precision.
    double k_abs = p_abs_abs_*s_inv;  // Kalman gain
    double k_vel = p_abs_vel_*s_inv;
    // Update state estimate.
    x_abs_ += k_abs * y;
    x_vel_ += k_vel * y;
    // Update state covariance.
    p_vel_vel_ -= p_abs_vel_*k_vel;
    p_abs_vel_ -= p_abs_vel_*k_abs;
    p_abs_abs_ -= p_abs_abs_*k_abs;
  }

  // Getters for the state and its covariance.
  public double getXAbs() { return x_abs_; }
  public double getXVel() { return x_vel_; }
  public double getCovAbsAbs() { return p_abs_abs_; }
  public double getCovAbsVel() { return p_abs_vel_; }
  public double getCovVelVel() { return p_vel_vel_; }
}
