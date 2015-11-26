#!/usr/bin/env python

# skrypt generuje plik .tex z ilustracja do rozwiazania .sol
# wygenerowanego przez gurobiego

import re

class Dot:
    def __init__(self):
        self.x = 0
        self.y = 0

    def __init__(self, x, y):
        self.x = x
        self.y = y

    def __add__(self, d):
        return Dot(self.x+d.x, self.y + d.y)

    def __str__(self):
        return "(" + str(self.x) + "," + str(self.y) + ")"

    def __repr__(self):
        return self.__str__();
        
    def __eq__(self, d):
        return self.x == d.x and self.y == d.y

    def __hash__(self):
        return self.y * 10000 + self.y

class Edge:
    def __init__(self):
        self.x = 0
        self.y = 0
        self.d = 0
        
    def __init__(self, x, y, d, dx, dy):
        self.x = x
        self.y = y
        self.d = d
        self.dx = dx
        self.dy = dy

    def __str__(self):
        return "(" + str(self.x) + "," + str(self.y) + "_D" + str(self.d) + "@" + str(self.dx) + "_" + str(self.dy)

    def __repr__(self):
        return self.__str__();
        
    def __eq__(self, d):
        return self.x == d.x and self.y == d.y and self.d == d.d

    def __hash__(self):
        return self.y * 100 + 4 * self.x + self.d + self.dx * 10000 + self.dy * 1000000

    def begin(self):
        return Dot(self.x, self.y)

    def end(self):
        delta = [ Dot(1,0), Dot(1,1), Dot(0,1), Dot(-1,1) ]
        
        return self.begin() + delta[self.d]

    def dirsym(self):
        return [ '-', '\\', '|', '/'][self.d]

    def dir(self):
        delta = [ Dot(1,0), Dot(1,1), Dot(0,1), Dot(-1,1) ]

        return delta[self.d]
    
# read a solution file to a dictionary

import argparse
parser = argparse.ArgumentParser()
parser.add_argument("filename")
args = parser.parse_args()
solfile = open(args.filename, "r")

vars = dict()

for line in solfile:
    if line[0] == '#': 
        continue
    ls = line.split(" ")
    vars[ls[0]] = float(ls[1])
solfile.close()

# create vertex set

v = dict()

dot_pattern = re.compile(r'^dot_(\d+),(\d+)')

for var in vars:
    dot = dot_pattern.search(var)
    
    if dot is None:
        continue
        
    v[Dot(int(dot.groups()[0]), int(dot.groups()[1]))] = vars[var]

# create edge set

e = dict()

edge_pattern = re.compile(r'mv_(\d+)_(\d+)_D(\d)@(\d+)_(\d+)')

for var in vars:
    edge = edge_pattern.search(var)
    
    if edge is None:
        continue
    
    e[Edge(int(edge.groups()[0]), int(edge.groups()[1]), int(edge.groups()[2]), int(edge.groups()[3]), int(edge.groups()[4]))] = vars[var]

ref = Dot(18,18)

print ref

for mv in e.keys():
    if e[mv] == 0: 
        continue
            
    mid = mv.begin() + mv.dir() +  mv.dir()
    if (mid.x != mv.dx):
        shift = -mv.dx + mid.x
    else:
        shift = -mv.dy + mid.y

    print Dot(mv.dx, mv.dy), mv.dirsym(), shift
    
