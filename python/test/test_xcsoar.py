#!/usr/bin/env python

import xcsoar
import argparse

# Parse command line parameters
parser = argparse.ArgumentParser(
    description='Please give me a IGC file name...')

parser.add_argument('file_name', type=str)

args = parser.parse_args()

print "Init xcsoar.Flight, don't store flight in memory"
flight = xcsoar.Flight(args.file_name, False)

fixes = flight.path()
for fix in fixes:
  print fix

del flight


print "Init xcsoar.Flight, store flight on init in memory"
flight = xcsoar.Flight(args.file_name, True)

fixes = flight.path()
for fix in fixes:
  print fix

del flight


