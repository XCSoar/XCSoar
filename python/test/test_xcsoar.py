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

  print "Takeoff: {}, location {} {}".format(takeoff['time'], takeoff['longitude'], takeoff['latitude'])
  print "Release: {}, location {} {}".format(release['time'], release['longitude'], release['latitude'])
  print "Landing: {}, location {} {}".format(landing['time'], landing['longitude'], landing['latitude'])

  print "Flight path from takeoff to release:"
  fixes = flight.path(takeoff['time'], release['time'])
  for fix in fixes:
    print fix

del flight


print
print "Init xcsoar.Flight, store flight on init in memory"
flight = xcsoar.Flight(args.file_name, True)

times = flight.times()

for dtime in times:
  takeoff = dtime['takeoff']
  release = dtime['release']
  landing = dtime['landing']

  print "Takeoff: {}, location {} {}".format(takeoff['time'], takeoff['longitude'], takeoff['latitude'])
  print "Release: {}, location {} {}".format(release['time'], release['longitude'], release['latitude'])
  print "Landing: {}, location {} {}".format(landing['time'], landing['longitude'], landing['latitude'])

  print "Flight path from takeoff to release:"
  fixes = flight.path(takeoff['time'], release['time'])
  for fix in fixes:
    print fix

  flight.reduce(takeoff['time'], landing['time'], max_points=10)

  print "Flight path from takeoff to landing, reduced:"
  fixes = flight.path(takeoff['time'], landing['time'])
  for fix in fixes:
    print fix

  analysis = flight.analyse(takeoff['time'], release['time'], landing['time'])
  pprint(analysis)

del flight


