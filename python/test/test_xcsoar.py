#!/usr/bin/env python

import xcsoar
import argparse
from pprint import pprint

# Parse command line parameters
parser = argparse.ArgumentParser(
    description='Please give me a IGC file name...')

parser.add_argument('file_name', type=str)

args = parser.parse_args()

print "Init xcsoar.Flight, don't store flight in memory"
flight = xcsoar.Flight(args.file_name, False)

times = flight.times()

for dtime in times:
  takeoff = dtime['takeoff']
  release = dtime['release']
  landing = dtime['landing']

  print "Takeoff: {}, location {}".format(takeoff['time'], takeoff['location'])
  print "Release: {}, location {}".format(release['time'], release['location'])
  print "Landing: {}, location {}".format(landing['time'], landing['location'])

  print "Flight path from takeoff to release:"
  fixes = flight.path(takeoff['time'], release['time'])
  for fix in fixes:
    print fix

del flight


print
print "Init xcsoar.Flight, store flight on init in memory"
flight = xcsoar.Flight(args.file_name, True)

times = flight.times()

flight_sequence = None

for dtime in times:
  takeoff = dtime['takeoff']
  release = dtime['release']
  landing = dtime['landing']

  print "Takeoff: {}, location {}".format(takeoff['time'], takeoff['location'])
  print "Release: {}, location {}".format(release['time'], release['location'])
  print "Landing: {}, location {}".format(landing['time'], landing['location'])

  print "Flight path from takeoff to release:"
  fixes = flight.path(takeoff['time'], release['time'])
  for fix in fixes:
    print fix

  flight.reduce(takeoff['time'], landing['time'], max_points=10)

  print "Flight path from takeoff to landing, reduced:"
  fixes = flight.path(takeoff['time'], landing['time'])
  for fix in fixes:
    print fix

  flight_sequence = fixes

  analysis = flight.analyse(takeoff=takeoff['time'],
                            scoring_start=release['time'],
                            scoring_end=landing['time'],
                            landing=landing['time'])
  pprint(analysis)

  fixes = flight.path(takeoff['time'], landing['time'])
  print xcsoar.encode([(row[2]['longitude'], row[2]['latitude']) for row in fixes], floor=10e5, method="double")

  pprint(flight.encode())

del flight


print
print "Init xcsoar.Flight with a python sequence"

flight = xcsoar.Flight([fix[0:5] for fix in flight_sequence])

for fix in flight.path():
  print fix

del flight
