#!/usr/bin/env python
#
# Creates tikz diagram out of pentasol Morpion position file
#

import re
import dot
import segment

import argparse
parser = argparse.ArgumentParser()
parser.add_argument("filename")
parser.add_argument("--scale")
parser.add_argument("--nonumbers",action='store_true')
parser.add_argument("--dotsize")
args = parser.parse_args()

if args.scale is not None:
    scale = args.scale
else:
    scale = "1.0"
    
# Load the pentasol file

psol = open(args.filename)

# get the reference point

refstring = psol.readline()
refpattern = re.compile("([0-9]+),([0-9]+)")
refparsed = re.search(refpattern, refstring)

ref = dot.Dot(int(refparsed.group(1)) * 2, int(refparsed.group(2)) * 2)

dots = []
segments = []

# create initial cross

dots.append([dot.Dot(3,9), 0])
dots.append([dot.Dot(3,7), 0])
dots.append([dot.Dot(3,5), 0])
dots.append([dot.Dot(3,3), 0])
dots.append([dot.Dot(5,3), 0])
dots.append([dot.Dot(7,3), 0])
dots.append([dot.Dot(9,3), 0])
dots.append([dot.Dot(9,1), 0])
dots.append([dot.Dot(9,-1), 0])
dots.append([dot.Dot(9,-3), 0])
dots.append([dot.Dot(7,-3), 0])
dots.append([dot.Dot(5,-3), 0])
dots.append([dot.Dot(3,-3), 0])
dots.append([dot.Dot(3,-5), 0])
dots.append([dot.Dot(3,-7), 0])
dots.append([dot.Dot(3,-9), 0])
dots.append([dot.Dot(1,-9), 0])
dots.append([dot.Dot(-1,-9), 0])
dots.append([dot.Dot(-3,-9), 0])
dots.append([dot.Dot(-3,-7), 0])
dots.append([dot.Dot(-3,-5), 0])
dots.append([dot.Dot(-3,-3), 0])
dots.append([dot.Dot(-5,-3), 0])
dots.append([dot.Dot(-7,-3), 0])
dots.append([dot.Dot(-9,-3), 0])
dots.append([dot.Dot(-9,-1), 0])
dots.append([dot.Dot(-9,1), 0])
dots.append([dot.Dot(-9,3), 0])
dots.append([dot.Dot(-7,3), 0])
dots.append([dot.Dot(-5,3), 0])
dots.append([dot.Dot(-3,3), 0])
dots.append([dot.Dot(-3,5), 0])
dots.append([dot.Dot(-3,7), 0])
dots.append([dot.Dot(-3,9), 0])
dots.append([dot.Dot(-1,9), 0])
dots.append([dot.Dot(1,9), 0])

# read move list and create dot and segment list

movepattern = re.compile("\(([0-9]+),([0-9]+)\) (.) ([-+]?\d+)")

max_x = max_y = 9
min_x = min_y = -9

cnt = 1
for mvstring in psol:
    mvparsed = re.search(movepattern, mvstring)
    
    if mvparsed.group(3) == "|":
        dir = dot.Dot(0,2)
    elif mvparsed.group(3) == "-":
        dir = dot.Dot(2, 0)
    elif mvparsed.group(3) == "/":
        dir = dot.Dot(2,-2)
    elif mvparsed.group(3) == "\\":
        dir = dot.Dot(2,2)
    else:
        print "Error: wrong direction symbol"

    x = int(mvparsed.group(1))*2 - ref.x - 3 
    y = int(mvparsed.group(2))*2 - ref.y - 3
    
    mx = x + int(mvparsed.group(4)) * dir.x
    my = y + int(mvparsed.group(4)) * dir.y
    
    if x > max_x:
        max_x = x
    if y > max_y:
        max_y = y
    if x < min_x:
        min_x = x
    if y < min_y:
        min_y = y

    dots.append([dot.Dot(x, y), cnt])
    cnt = cnt + 1

    m = dot.Dot(mx,my)
    
    segments.append(segment.Segment(m, m + dir))    
    segments.append(segment.Segment(m + dir, m + 2*dir))
    segments.append(segment.Segment(m, m - dir))
    segments.append(segment.Segment(m-dir,m-2*dir))
    
if args.dotsize is None:
    dotsize = "1"
else:
    dotsize = args.dotsize
    
print """
\\begin{tikzpicture}[scale=%s]
\\draw[step=2*\unitsize,gray!30,very thin,shift={(-\\unitsize, -\\unitsize)}] (%f*\\unitsize,%f*\\unitsize) grid (%f*\unitsize,%f*\unitsize);
\\tikzstyle{every node}=[draw,
			circle,
			fill=white,
			minimum size  = %s*\\unitsize,
			node distance = 2*\\unitsize,
			inner sep=0pt
			] 
""" % (scale, min_x - 0.9, min_y - 0.9, max_x + 0.9, max_y + 0.9, dotsize) 

for s in segments:
    print """
\\draw[thick] (%d*\unitsize,%d*\unitsize) -- (%d*\unitsize,%d*\unitsize);
    """ % (s.p.x, s.p.y, s.q.x, s.q.y)

for d in dots:
    if d[1] == 0:
        print """
\\node[fill=black!30, thin] at (%d*\unitsize,%d*\unitsize) {};
        """ % (d[0].x, d[0].y)
    else:
        if not args.nonumbers:
            print """
\\node[thin] at (%d*\unitsize,%d*\unitsize)  {\scalebox{0.35}{%s}};
            """ % (d[0].x, d[0].y, d[1])
        else:
            print """
\\node[thin] at (%d*\unitsize,%d*\unitsize)  {\scalebox{0.35}{}};
            """ % (d[0].x, d[0].y)
        
print """
\\end{tikzpicture}
"""
