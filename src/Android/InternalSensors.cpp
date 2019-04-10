/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "InternalSensors.hpp"
#include "Context.hpp"
#include "Atmosphere/Pressure.hpp"
#include "org_xcsoar_InternalGPS.h"
#include "org_xcsoar_NonGPSSensors.h"
#include "Blackboard/DeviceBlackboard.hpp"
#include "Components.hpp"
#include "Math/SelfTimingKalmanFilter1d.hpp"
#include "OS/Clock.hpp"
#include "Geo/Geoid.hpp"
#include "Compiler.h"

Java::TrivialClass InternalSensors::gps_cls, InternalSensors::sensors_cls;
jmethodID InternalSensors::gps_ctor_id, InternalSensors::close_method;
jmethodID InternalSensors::sensors_ctor_id;
jmethodID InternalSensors::mid_sensors_getSubscribableSensors;
jmethodID InternalSensors::mid_sensors_subscribeToSensor_;
jmethodID InternalSensors::mid_sensors_cancelSensorSubscription_;
jmethodID InternalSensors::mid_sensors_subscribedToSensor_;
jmethodID InternalSensors::mid_sensors_cancelAllSensorSubscriptions_;

bool
InternalSensors::Initialise(JNIEnv *env)
{
  assert(!gps_cls.IsDefined());
  assert(!sensors_cls.IsDefined());
  assert(env != nullptr);

  gps_cls.Find(env, "org/xcsoar/InternalGPS");

  gps_ctor_id = env->GetMethodID(gps_cls, "<init>",
                                 "(Landroid/content/Context;I)V");
  close_method = env->GetMethodID(gps_cls, "close", "()V");

  sensors_cls.Find(env, "org/xcsoar/NonGPSSensors");

  sensors_ctor_id = env->GetMethodID(sensors_cls, "<init>",
                                     "(Landroid/content/Context;I)V");

  mid_sensors_getSubscribableSensors =
    env->GetMethodID(sensors_cls, "getSubscribableSensors", "()[I");
  assert(mid_sensors_getSubscribableSensors != nullptr);

  mid_sensors_subscribeToSensor_ =
      env->GetMethodID(sensors_cls, "subscribeToSensor", "(I)Z");
  mid_sensors_cancelSensorSubscription_ =
      env->GetMethodID(sensors_cls, "cancelSensorSubscription", "(I)Z");
  mid_sensors_subscribedToSensor_ =
      env->GetMethodID(sensors_cls, "subscribedToSensor", "(I)Z");
  mid_sensors_cancelAllSensorSubscriptions_ =
      env->GetMethodID(sensors_cls, "cancelAllSensorSubscriptions", "()V");
  assert(mid_sensors_subscribeToSensor_ != nullptr);
  assert(mid_sensors_cancelSensorSubscription_ != nullptr);
  assert(mid_sensors_subscribedToSensor_ != nullptr);
  assert(mid_sensors_cancelAllSensorSubscriptions_ != nullptr);

  return true;
}

void
InternalSensors::Deinitialise(JNIEnv *env)
{
  gps_cls.Clear(env);
  sensors_cls.Clear(env);
}

InternalSensors::InternalSensors(JNIEnv *env, jobject gps_obj,
                                 jobject sensors_obj)
    :obj_InternalGPS_(env, gps_obj),
     obj_NonGPSSensors_(env, sensors_obj)
{
  // Import the list of subscribable sensors from the NonGPSSensors object.
  getSubscribableSensors(env, sensors_obj);
}

InternalSensors::~InternalSensors()
{
  // Unsubscribe from sensors and the GPS.
  cancelAllSensorSubscriptions();
  JNIEnv *env = Java::GetEnv();
  env->CallVoidMethod(obj_InternalGPS_.Get(), close_method);
}

bool
InternalSensors::subscribeToSensor(int id)
{
  JNIEnv *env = Java::GetEnv();
  return env->CallBooleanMethod(obj_NonGPSSensors_.Get(),
                                mid_sensors_subscribeToSensor_, (jint) id);
}

bool
InternalSensors::cancelSensorSubscription(int id)
{
  JNIEnv *env = Java::GetEnv();
  return env->CallBooleanMethod(obj_NonGPSSensors_.Get(),
                                mid_sensors_cancelSensorSubscription_,
                                (jint)id);
}

bool
InternalSensors::subscribedToSensor(int id) const
{
  JNIEnv *env = Java::GetEnv();
  return env->CallBooleanMethod(obj_NonGPSSensors_.Get(),
                                mid_sensors_subscribedToSensor_, (jint)id);
}

void
InternalSensors::cancelAllSensorSubscriptions()
{
  JNIEnv *env = Java::GetEnv();
  env->CallVoidMethod(obj_NonGPSSensors_.Get(),
                      mid_sensors_cancelAllSensorSubscriptions_);
}

InternalSensors *
InternalSensors::create(JNIEnv *env, Context *context, unsigned int index)
{
  assert(sensors_cls != nullptr);
  assert(gps_cls != nullptr);

  // Construct InternalGPS object.
  jobject gps_obj =
    env->NewObject(gps_cls, gps_ctor_id, context->Get(), index);
  Java::RethrowException(env);
  assert(gps_obj != nullptr);

  // Construct NonGPSSensors object.
  jobject sensors_obj =
      env->NewObject(sensors_cls, sensors_ctor_id, context->Get(), index);
  assert(sensors_obj != nullptr);

  InternalSensors *internal_sensors =
      new InternalSensors(env, gps_obj, sensors_obj);
  env->DeleteLocalRef(gps_obj);
  env->DeleteLocalRef(sensors_obj);

  return internal_sensors;
}

// Helper for retrieving the set of sensors to which we can subscribe.
void
InternalSensors::getSubscribableSensors(JNIEnv *env, jobject sensors_obj)
{
  jintArray ss_arr = (jintArray)
    env->CallObjectMethod(obj_NonGPSSensors_.Get(),
                          mid_sensors_getSubscribableSensors);
  jsize ss_arr_size = env->GetArrayLength(ss_arr);
  jint *ss_arr_elems = env->GetIntArrayElements(ss_arr, nullptr);
  subscribable_sensors_.assign(ss_arr_elems, ss_arr_elems + ss_arr_size);
  env->ReleaseIntArrayElements(ss_arr, ss_arr_elems, 0);
}

/*
 * From here to end: C++ functions called by Java to export GPS and sensor
 * information into XCSoar C++ code.
 */

// Helper for the C++ functions called by Java (below).
static inline unsigned int
getDeviceIndex(JNIEnv *env, jobject obj)
{
  jfieldID fid_index = env->GetFieldID(env->GetObjectClass(obj), "index", "I");
  return env->GetIntField(obj, fid_index);
}

// Implementations of the various C++ functions called by InternalGPS.java.

gcc_visibility_default
JNIEXPORT void JNICALL
Java_org_xcsoar_InternalGPS_setConnected(JNIEnv *env, jobject obj,
                                         jint connected)
{
  unsigned index = getDeviceIndex(env, obj);

  ScopeLock protect(device_blackboard->mutex);
  NMEAInfo &basic = device_blackboard->SetRealState(index);

  switch (connected) {
  case 0: /* not connected */
    basic.alive.Clear();
    basic.location_available.Clear();
    break;

  case 1: /* waiting for fix */
    basic.alive.Update(MonotonicClockFloat());
    basic.gps.nonexpiring_internal_gps = true;
    basic.location_available.Clear();
    break;

  case 2: /* connected */
    basic.alive.Update(MonotonicClockFloat());
    basic.gps.nonexpiring_internal_gps = true;
    break;
  }

  device_blackboard->ScheduleMerge();
}

gcc_visibility_default
JNIEXPORT void JNICALL
Java_org_xcsoar_InternalGPS_setLocation(JNIEnv *env, jobject obj,
                                        jlong time, jint n_satellites,
                                        jdouble longitude, jdouble latitude,
                                        jboolean hasAltitude, jdouble altitude,
                                        jboolean hasBearing, jdouble bearing,
                                        jboolean hasSpeed, jdouble ground_speed,
                                        jboolean hasAccuracy, jdouble accuracy,
                                        jboolean hasAcceleration, jdouble acceleration)
{
  unsigned index = getDeviceIndex(env, obj);

  ScopeLock protect(device_blackboard->mutex);
  NMEAInfo &basic = device_blackboard->SetRealState(index);
  basic.UpdateClock();
  basic.alive.Update(basic.clock);

  BrokenDateTime date_time = BrokenDateTime::FromUnixTimeUTC(time / 1000);
  double second_of_day = date_time.GetSecondOfDay() +
    /* add the millisecond fraction of the original timestamp for
       better accuracy */
    unsigned(time % 1000) / 1000.;

  if (second_of_day < basic.time &&
      basic.date_time_utc.IsDatePlausible() &&
      (BrokenDate)date_time > (BrokenDate)basic.date_time_utc)
    /* don't wrap around when going past midnight in UTC */
    second_of_day += 24u * 3600u;

  basic.time = second_of_day;
  basic.time_available.Update(basic.clock);
  basic.date_time_utc = date_time;

  basic.gps.satellites_used = n_satellites;
  basic.gps.satellites_used_available.Update(basic.clock);
  basic.gps.real = true;
  basic.gps.nonexpiring_internal_gps = true;
  basic.location = GeoPoint(Angle::Degrees(longitude),
                            Angle::Degrees(latitude));
  basic.location_available.Update(basic.clock);

  if (hasAltitude) {
    auto GeoidSeparation = EGM96::LookupSeparation(basic.location);
    basic.gps_altitude = altitude - GeoidSeparation;
    basic.gps_altitude_available.Update(basic.clock);
  } else
    basic.gps_altitude_available.Clear();

  if (hasBearing) {
    basic.track = Angle::Degrees(bearing);
    basic.track_available.Update(basic.clock);
  } else
    basic.track_available.Clear();

  if (hasSpeed) {
    basic.ground_speed = ground_speed;
    basic.ground_speed_available.Update(basic.clock);
  }

  if (hasAccuracy)
    basic.gps.hdop = accuracy;

  if (hasAcceleration)
    basic.acceleration.ProvideGLoad(acceleration, true);

  device_blackboard->ScheduleMerge();
}

// Implementations of the various C++ functions called by NonGPSSensors.java.

gcc_visibility_default
JNIEXPORT void JNICALL
Java_org_xcsoar_NonGPSSensors_setAcceleration(JNIEnv *env, jobject obj,
                                              jfloat ddx, jfloat ddy,
                                              jfloat ddz)
{
  // TODO
  /*
  const unsigned int index = getDeviceIndex(env, obj);
  ScopeLock protect(device_blackboard->mutex);
  NMEAInfo &basic = device_blackboard->SetRealState(index);
  */
}

gcc_visibility_default
JNIEXPORT void JNICALL
Java_org_xcsoar_NonGPSSensors_setRotation(JNIEnv *env, jobject obj,
                                          jfloat dtheta_x, jfloat dtheta_y,
                                          jfloat dtheta_z)
{
  // TODO
  /*
  const unsigned int index = getDeviceIndex(env, obj);
  ScopeLock protect(device_blackboard->mutex);
  NMEAInfo &basic = device_blackboard->SetRealState(index);
  */
}

gcc_visibility_default
JNIEXPORT void JNICALL
Java_org_xcsoar_NonGPSSensors_setMagneticField(JNIEnv *env, jobject obj,
                                               jfloat h_x, jfloat h_y,
                                               jfloat h_z)
{
  // TODO
  /*
  const unsigned int index = getDeviceIndex(env, obj);
  ScopeLock protect(device_blackboard->mutex);
  NMEAInfo &basic = device_blackboard->SetRealState(index);
  */
}

/**
 * Helper function for
 * Java_org_xcsoar_NonGPSSensors_setBarometricPressure: Given a
 * current measurement of the atmospheric pressure and the rate of
 * change of atmospheric pressure (in millibars and millibars per
 * second), compute the uncompensated vertical speed of the glider in
 * meters per second, assuming standard atmospheric conditions
 * (deviations from these conditions should not matter very
 * much). This calculation can be derived by taking the formula for
 * converting barometric pressure to pressure altitude (see e.g.
 * http://psas.pdx.edu/RocketScience/PressureAltitude_Derived.pdf),
 * expressing it as a function P(t), the atmospheric pressure at time
 * t, then taking the derivative with respect to t. The dP(t)/dt term
 * is the pressure change rate.
 */
gcc_pure
static inline double
ComputeNoncompVario(const double pressure, const double d_pressure)
{
  static constexpr double FACTOR(-2260.389548275485);
  static constexpr double EXPONENT(-0.8097374740609689);
  return FACTOR * pow(pressure, EXPONENT) * d_pressure;
}

gcc_visibility_default
JNIEXPORT void JNICALL
Java_org_xcsoar_NonGPSSensors_setBarometricPressure(JNIEnv *env, jobject obj,
                                                    jfloat pressure,
                                                    jfloat sensor_noise_variance)
{
  /* We use a Kalman filter to smooth Android device pressure sensor
     noise.  The filter requires two parameters: the first is the
     variance of the distribution of second derivatives of pressure
     values that we expect to see in flight, and the second is the
     maximum time between pressure sensor updates in seconds before
     the filter gives up on smoothing and uses the raw value.
     The pressure acceleration variance used here is actually wider
     than the maximum likelihood variance observed in the data: it
     turns out that the distribution is more heavy-tailed than a
     normal distribution, probably because glider pilots usually
     experience fairly constant pressure change most of the time. */
  static constexpr double KF_VAR_ACCEL(0.0075);
  static constexpr double KF_MAX_DT(60);

  // XXX this shouldn't be a global variable
  static SelfTimingKalmanFilter1d kalman_filter(KF_MAX_DT, KF_VAR_ACCEL);

  const unsigned int index = getDeviceIndex(env, obj);
  ScopeLock protect(device_blackboard->mutex);

  /* Kalman filter updates are also protected by the blackboard
     mutex. These should not take long; we won't hog the mutex
     unduly. */
  kalman_filter.Update(pressure, sensor_noise_variance);

  NMEAInfo &basic = device_blackboard->SetRealState(index);
  basic.ProvideNoncompVario(ComputeNoncompVario(kalman_filter.GetXAbs(),
                                                kalman_filter.GetXVel()));
  basic.ProvideStaticPressure(
      AtmosphericPressure::HectoPascal(kalman_filter.GetXAbs()));
  device_blackboard->ScheduleMerge();
}
