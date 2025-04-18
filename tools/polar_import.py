#!/usr/bin/python
#
# c hjr 2021
#
# Prozess xyscan digitized polar data and compare with other sources
#
# Prerequisites:
# Linux box, xyscan, German locale
#
# The recommended Process in short:
# 1. Digitize a polar plot
#    - Have a polar scan as eg. jpg or pdf.
#    - Load it into xyscan and go through the xyscan procedure and digitize ca. 20 points on the polar in even space, most dense towards lower speeds.
#    - You might add a comment with xyscan to identify your work later on.
#    - Saving the result creates a xyscan.txt, Put the file in a new subdirectory. You might name it differently.
# 2. Create and complete a rawpolar file
#    - Start ./polar_import.py --xys my_subdir/xyscan.txt > my_subdir/MyGliderType.rawpolar.
#    - Edit the .rawpolar file and complete with the proper reference weight and wing area.
# 3. Find a good second degree polynolial approximathion that fits to your digi points in the desired area best.
#    - Start ./polar_import.py --raw my_subdir/MyGliderType.rawpolar --mod cutstall
#    - PLay with the --mod options:
#       * cutstall: chops off the lower speed kink at the min sink speed point, You almost always want this.
#       * emphld: emphasize the best glide ratio portion of the polar to have a best fit there.
#       * emphlow: emphasize the lower speed portion of the polar to fit best.
# 4. Put the result into the XCSoar source code.
#    - Pick the console output of your favorite run and drop the code line template into XCSoar's PolarStore.cpp AT THE VERY END.
#    - Complete with max. water ballast [l], max. speed for normal operation [m/s], and the handicap index
#
# done ;)
#
# You may use the handy --cmp feature to compare related polars at the same wing load to double check your results.
#

import numpy as np
import sys
import argparse
import matplotlib.pyplot as plt
import locale
from enum import Enum

# Plot enumeration
pnr =  0

class polar:
    _name = 'Some glider name'
    _w = 0. # ref weight
    _S = 0.   # wing area
    _x = []   # digitized point pairs
    _y = []

# Read scan digitization result
def read_xyscan_file(fnam):
    f = open(fnam, 'r')
    if not f:
        return None
    locale.setlocale(locale.LC_NUMERIC, 'de_DE.UTF-8')
    ret = polar()
    for line in f:
        if line[0] == '#':
            if "Quelle:" in line:
                ret._name = line.rstrip(".jpg\n").split('/')[-1]
            if "Kommentar:" in line:
                ret._name = line.split(':')[-1].strip()
            continue
        line_l = line.strip().split('\t')
        a = list(map(locale.atof, line_l))
        ret._x.append(a[0])
        ret._y.append(a[1])
    f.close()
    return ret

# Read a .rawpolar file
def read_raw_polar(fnam, weight=0.0):
    f = open(fnam, 'r')
    if not f:
        return None
    ret = polar()
    count = 0
    for line in f:
        if line[0] == '#' : continue
        if count == 0 :
            ret._name = line.strip()
        elif count == 1 :
            ret._w = float(line.strip())
        elif count == 2 :
            ret._S = float(line.strip())
        elif count == 3 :
            ret._x = list(map(float, line.strip('[] \n').split(',')))
        elif count == 4 :
            ret._y = list(map(float, line.strip('[] \n').split(',')))
        count += 1
    f.close()
    if weight != 0.0:
        s = np.sqrt(weight / ret._w)
        ret._w = weight
        ret._x = list(map(lambda x: x * s, ret._x))
        ret._y = list(map(lambda x: x * s, ret._y))
    return ret

# Scale polar to new ref weight, prefered wingload
def scale_polar(p, weight, wingload=0.0):
    if wingload != 0.0:
        weight = p._S * wingload
    else:
        if weight == 0.0:
            weight = p._w
    s = np.sqrt(weight / p._w)
    p._w = weight
    p._x = list(map(lambda x: x * s, p._x))
    p._y = list(map(lambda x: x * s, p._y))
    return p

def get_current_xc_polar(glider, weight=0.0):
    f = open("../src/Polar/PolarStore.cpp", 'r')
    if not f:
        return None
    ret = None
    for line in f:
        if line[0:7] != "  { _T(" : continue
        if glider in line:
            ##357, 165, 108.8, -0.64, 156.4, -1.18, 211.13, -2.5, 9.0, 0.0, 114
            ret = polar()
            ret._name = glider + "(xcs)"
            line = line.split('{')[1];
            line = line.split('}')[0];
            line_l = line.strip(" \n").split(',')
            del line_l[0]
            a = list(map(float, line_l))
            s = 1.0
            ret._w = a[0]
            if weight != 0:
                s = np.sqrt(weight/a[0])
                ret._w = weight
            ret._S = a[8]
            ret._x = [a[2]*s, a[4]*s, a[6]*s]
            ret._y = [a[3]*s, a[5]*s, a[7]*s]
            break
    f.close()
    return ret


def min_sink_idx(p):
    return np.argmin(p._y)

def cutoff_stall(p):
    minsinkspeed = p._x[np.argmax(p._y)]
    x = []
    y = []
    count = 0
    for i in p._x:
        if i > minsinkspeed:
            x.append(p._x[count])
            y.append(p._y[count])
        count += 1
    p._x = x
    p._y = y
    return

def best_ld(p):
    ld = []
    for i in range(len(p._x)):
        ld.append((-p._x[i])/p._y[i])
    return p._x[np.argmax(ld)]

def emphasize_bestld(p):
    ldmax_speed = best_ld(p)
    x = []
    y = []
    count = 0
    for i in p._x:
        emph = 8-int(abs((i-ldmax_speed)*(i-ldmax_speed))/8)
        if emph > 0:
            for _ in range(emph):
                x.append(p._x[count])
                y.append(p._y[count])
        x.append(p._x[count])
        y.append(p._y[count])
        count += 1
    p._x = x
    p._y = y
    return

def emphasize_lowerspeeds(p):
    min_speed = p._x[np.argmin(p._x)]
    x = []
    y = []
    count = 0
    for i in p._x:
        emph = 8-int(abs(i-min_speed)/10)
        if i < 140.:
            if emph > 0:
                for _ in range(emph):
                    x.append(p._x[count])
                    y.append(p._y[count])
            x.append(p._x[count])
            y.append(p._y[count])
        count += 1
    p._x = x
    p._y = y
    return

def polar_store_line(p, wingload=0.0):
    if wingload == 0.0:
        wieght = int(p._w)
    else:
        wieght = int(wingload * p._S)
    p = scale_polar(p, wieght)
    pf = np.polyfit(p._x, p._y, 2)
    p1d = np.poly1d(pf)
    minsinkspeed = p._x[np.argmax(p._y)]
    threedots = [minsinkspeed, 130.0, 170.0]
    print("XCSoar PolarStore template:")
    print('  { _T("' + p._name + '"),', wieght, ", 0.0,", threedots[0], ',', p1d(threedots[0]), ',', threedots[1], ',', p1d(threedots[1]), ',', threedots[2],',', p1d(threedots[2]), ',', p._S, ", 0.0, 0 },")
    print("The raw file for this:")
    print(p._name)
    print(p._w)
    print(p._S)
    print(threedots)
    print(list(map(p1d, threedots)))
    print("Second order polynomial coefficians:")
    print(list(p1d.coefficients))
    return


def plot_polar(ax, p, g, l=None):
    global pnr
    pnr = pnr + 1

    # It is convenient to use `poly1d` objects for dealing with polynomials:
    pf = np.polyfit(p._x, p._y, g)
    p1d = np.poly1d(pf)

    # Print the needed line for the XCS polar store
    polar_store_line(p, wingload)

    ch = '.' # ida polar with many points
    leg = "digitized, grade=" + str(grade)
    if len(p._x) == 3:
        leg = "3 points"
    if pnr > 1:
        ch = 'o'
        l.append(p._name + " " + str(p._w) + "kg")

    # Draw dots
    dots, = ax.plot(p._x, p._y, ch)

    # Draw the polar(s)
    xrange = np.linspace(np.amin(p._x + [80]), np.ma.max(p._x + [220]), 100)
    plot, = ax.plot(xrange, p1d(xrange))

    if pnr == 1:
        # Add a legend
        l1 = ax.legend([dots, plot], [leg, p._name + " " + str(p._w) + "kg"], bbox_to_anchor=(1.05, 1))
        plt.gca().add_artist(l1)

    return plot



## Script start

class PolarMod(Enum):
    CUTSTALL = 1
    EMPHLD = 2
    EMPHLOW = 3

parser = argparse.ArgumentParser(description='Process raw digitized polar data, and compare. Create XCS source code lines.')
parser.add_argument('--xys', dest='xyscan', const="xyscan.txt", nargs='?',
                    help='xyscan output of digitized points to generate a raw template.')
parser.add_argument('--raw', dest='rawfile',
                    help='input file with digitized polar points.')
parser.add_argument('--mod', dest='polar_mod', type=str, default=None, nargs='+',
                    choices=[i.name.lower() for i in PolarMod])
parser.add_argument('--ref', dest='refw', default=0.0, type=float,
                    help='desired reference mass in kg for the ouput polar')
parser.add_argument('--load', dest='wingload', default=0.0, type=float,
                    help='desired wingload in kg/m2 for the ouput polars, overriding --ref option')
parser.add_argument('--grd', dest='grade', default=2, type=int,
                    help='polynomial fit grade')
parser.add_argument('--cmp', action='append', nargs='+',
                    help='compare input with the following polars read from XCS PolarStore, or rawfile')
args = parser.parse_args()
#print(args.rawfile, args.refw, args.cmp)

if len(sys.argv)==1:
    parser.print_help()
    exit(1)

# Get the desired ref weight
wingload = args.wingload
refw = 0.0
if wingload == 0.0:
    refw = args.refw

# Fit digi points with this grade
grade = args.grade

# Parse xyscan output
if args.xyscan != None:
    tmp = read_xyscan_file(args.xyscan)
    if tmp:
        if refw != 0.0:
            tmp._w = refw
        print(tmp._name)
        print("# The reference weight for this polar plot:")
        print(tmp._w)
        print("# The wing area of this glider:")
        print(tmp._S)
        print("# The digitized x,y points of the polar curve:")
        print(tmp._x)
        print(tmp._y)
    exit(0)

# Read in digitized points
ida = False
if args.rawfile != None:
    idap = read_raw_polar(args.rawfile, refw)
    if idap:
        ida = True
        idap = scale_polar(idap, refw, wingload)
        if wingload == 0.0:
            wingload = idap._w / idap._S
            refw = 0.0

# Apply mods to the polar to make it reasonable for the glider
if ida and args.polar_mod != None:
    args.polar_mod = list(map(lambda x: PolarMod[x.upper()],args.polar_mod))
    if PolarMod.CUTSTALL in args.polar_mod:
        cutoff_stall(idap)
    if PolarMod.EMPHLD in args.polar_mod:
        emphasize_bestld(idap)
    if PolarMod.EMPHLOW in args.polar_mod:
        emphasize_lowerspeeds(idap)

# Optionally read in more polars to compare
cmp = []
if args.cmp:
    for cmplist in args.cmp:
        for f in cmplist:
            xcp = None
            if "." in f: # from XCS sources
                xcp = read_raw_polar(f)
            else:
                xcp = get_current_xc_polar(f)
            if xcp:
                xcp = scale_polar(xcp, refw, wingload)
                cmp.append(xcp)
                if wingload == 0.0:
                    wingload = xcp._w / xcp._S
                    refw = 0.0


fig, ax = plt.subplots()
title = "XCS polar "
plt.xlabel("km/h")
plt.ylabel("m/s")
plt.ylim(-4,0)

if ida:
    plot_polar(ax, idap, grade)
    title += idap._name

plt.title(title, size=18)

if len(cmp) > 0:
    legend = []
    plot = []
    for x in cmp:
        subplot = plot_polar(ax, x, 2, legend)
        if pnr == 1:
            legend = []
        else:
            plot.append(subplot)
    plt.legend(plot, legend, loc=3)

plt.show()
