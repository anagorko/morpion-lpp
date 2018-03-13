#!/usr/bin/env python
#
# Creates lists for tikz foreach loop out of pentasol Morpion position file
#

import re
import sys

import argparse
parser = argparse.ArgumentParser()
parser.add_argument("filename")
parser.add_argument("--limit")
parser.add_argument("--suffix")

args = parser.parse_args()

limit = 1000
if args.limit is not None:
    limit = int(args.limit)

# Load the pentasol file

psol = open(args.filename)

def openext(ext):
    basename = args.filename.rsplit(".", 1)[0]
    if args.suffix is not None:
        basename += args.suffix
    return open(basename + "." + ext, 'w')

fmoves = openext("moves")
fdots = openext("dots")

# get the reference point

refstring = psol.readline()
refpattern = re.compile("([0-9]+),([0-9]+)")
refparsed = re.search(refpattern, refstring)

rx = int(refparsed.group(1))
ry = int(refparsed.group(2))


# read move list and create dot and segment list

movepattern = re.compile("\(([0-9]+),([0-9]+)\) (.) ([-+]?\d+)")

cnt = 1
for mvstring in psol:
    mvparsed = re.search(movepattern, mvstring)
    
    if mvparsed.group(3) == "|":
        dx, dy, d = 0, 1, 2
    elif mvparsed.group(3) == "-":
        dx, dy, d = 1, 0, 0
    elif mvparsed.group(3) == "/":
        dx, dy, d = 1, -1, 3
    elif mvparsed.group(3) == "\\":
        dx, dy, d = 1, 1, 1
    else:
        print("Error: wrong direction symbol " + mvparsed.group(3))
        break

    c = int(mvparsed.group(4)) - 2
    x = int(mvparsed.group(1)) - rx
    y = int(mvparsed.group(2)) - ry
    mx = x + c * dx
    my = y + c * dy

    if cnt > 1:
        fmoves.write(',')
        fdots.write(',')
    fmoves.write(str(mx) + '/' + str(my) + '/' + str(dx) + '/' + str(dy))
    fdots.write(str(cnt) + '/' + str(d) + '/' + str(x) + '/' + str(y))

    cnt += 1
    if cnt > limit:
        break
