#!/usr/bin/env python

# Octagonal Hull module

import unittest
import copy
from dot import Dot

# Hull class
#
#  Defines a hull that is an intersection of half-planes with specified set of normal vectors
#
#  The center of the cross is (0,0)
#  The grid is on integral points with odd coordinates

class Hull:
    # initialize with the Morpion cross
    def __init__(self):
        self.put(Dot(3,9))
        self.put(Dot(9,3))
        self.put(Dot(9,-3))
        self.put(Dot(3,-9))
        self.put(Dot(-3,-9))
        self.put(Dot(-9,-3))
        self.put(Dot(-9,3))
        self.put(Dot(-3,9))

    def init_from_id(self, id):
        self.sides = map(int,id.split('_')[1:])
        self.dirty=True
        # todo: find a reference point inside
        
    # return unique id of the polygon
    def id(self):
        return "hull_" + "_".join(map(str, self.sides))

    # add point and possibly enlarge the hull
    def put(self, dot):
        self.dirty = True
        self.ref = dot
        for i, s in enumerate(self.sides):
            self.sides[i] = max(s, dot * self.directions[i])

    # determine whether dot is inside of the hull
    def inside(self, dot):
        for i, s in enumerate(self.sides):
            if self.sides[i] < dot * self.directions[i]:
                return False
        return True

    # flood fill the hull and find points on the boundary
    def fill(self, dot):
        if dot in self.interior_set:
            return

        if self.inside(dot):
            self.interior_set.add(dot)
            if dot.x < self._lx:
                self._lx = dot.x
            if dot.x > self._hx:
                self._hx = dot.x
            if dot.y < self._ly:
                self._ly = dot.y
            if dot.y > self._hy:
                self._hy = dot.y
            for d in self.directions:
                self.fill(dot + d)
        else:
            self.boundary_set.add(dot)

    def compute(self):
        if not self.dirty:
            return
    
        self.boundary_set = set()
        self.interior_set = set()
        
        # todo: check properly if self.ref is set
        if not self.ref is None:
            self._lx = self.ref.x
            self._hx = self.ref.x
            self._ly = self.ref.y
            self._hy = self.ref.y
            self.fill(self.ref)
        
        self.dirty = False

    def boundary(self):
        self.compute()
        
        return self.boundary_set

    def interior(self):
        self.compute()

        return self.interior_set
    
    def lx(self):
        self.compute()
        return self._lx
        
    def ly(self):
        self.compute()
        return self._ly
        
    def hx(self):
        self.compute()
        return self._hx
        
    def hy(self):
        self.compute()
        return self._hy

    def pr(self):
        self.compute()
        
        for y in range(self.ly(), self.hy()+1,2):
            for x in range(self.lx(), self.hx()+1,2):
                if Dot(x,y) in self.interior():
                    print '*',
                else:
                    print '-',
            print

    def edge(self, dir):
        self.compute()
        
        norm = -1000000
        
        for d in self.interior_set:
            n = d.x * dir.x + d.y * dir.y
            if t > norm:
                norm = t
    
        len = 0
           
        for d in self.interior_set:
            if d.x * dir.x + d.y * dir.y == norm:
                len = len + 1
                 
        return len - 1

    def r(self):
        self.compute()
        return str(-self.lx()/2 - 1) + "," + str(-self.ly()/2 - 1)

    def width(self):
        self.compute()
        return 1 + (self.hx() - self.lx()) / 2
        
    def height(self):
        self.compute()
        return 1 + (self.hy() - self.ly()) / 2

    def params(self):
        return ' -w ' + str(self.width()) +\
               ' -h ' + str(self.height()) +\
               ' -r ' + self.r();

    def symmetryClass(self):
        return self.id()

    def solve_lpp(self):            
        cmd = './generator --exact --potential ' + self.params() + ' -p -v 5d';
        if symmetric:
            cmd = cmd + ' --symmetric'

        print cmd                  
        os.system(cmd)

        model = read("morpion_lpp.lp")
        model.params.threads = 4
        model.params.mIPfocus = 3
        model.optimize(only_feasible_callback)
        
        self.RunTime = model.RunTime

        if model.SolCount > 0:
            self.ObjVal = model.ObjVal
            self.ObjBound = model.ObjBound
            
        return model.SolCount

class Rectangle(Hull):
    def __init__(self):
        self.sides = [ 0, 0, 0, 0 ]
        self.directions =\
                  [ Dot(0, 2),   # N
                    Dot(2, 0),   # E
                    Dot(0, -2),  # S
                    Dot(-2, 0) ] # W
        self.dirty = True
        Hull.__init__(self)

    def params(self):
        return Hull.params(self) + ' --rhull';

    def symmetryClass(self):
        cl = copy.copy(self.sides)
        
        if self.width() < self.height():
            x = cl.pop(0)
            cl.append(x)
        if cl[0] < cl[2]:
            cl[0], cl[2] = cl[2], cl[0]
        if cl[1] < cl[3]:
            cl[1], cl[3] = cl[3], cl[1]
    
        return "rect_" + "_".join(map(str, cl))
        
class Octagon(Hull):
    def __init__(self):
        self.sides = [ 0, 0, 0, 0, 0, 0, 0, 0 ]
        self.directions =\
                  [ Dot(0, 2),   # N
                    Dot(2, 2),   # NE
                    Dot(2, 0),   # E
                    Dot(2, -2),  # SE
                    Dot(0, -2),  # S
                    Dot(-2, -2), # SW
                    Dot(-2, 0),  # W
                    Dot(-2, 2) ] # NW
        self.dirty = True
        Hull.__init__(self)

    def g(self):
        return str(self.edge(Dot(0,-1))) + " " +\
                str(self.edge(Dot(1,-1))) + " " +\
                str(self.edge(Dot(1, 0))) + " " +\
                str(self.edge(Dot(1, 1))) + " " +\
                str(self.edge(Dot(0, 1))) + " " +\
                str(self.edge(Dot(-1,1))) + " " +\
                str(self.edge(Dot(-1,0))) + " " +\
                str(self.edge(Dot(-1,-1)))

    def params(self):
        return Hull.params(self) + ' --hull'\
                + ' -g ' + self.g();

    def symmetryClass(self):
        cl = copy.copy(self.sides)
        
        if self.width() < self.height():
            x = cl.pop(0)
            cl.append(x)
            x = cl.pop(0)
            cl.append(x)
        
        if cl[0] < cl[4]:
            cl[0], cl[4] = cl[4], cl[0]
            cl[1], cl[3] = cl[3], cl[1]
            cl[7], cl[5] = cl[5], cl[7]
        if cl[2] < cl[6]: 
            cl[2], cl[6] = cl[6], cl[2]
            cl[1], cl[7] = cl[7], cl[1]
            cl[3], cl[5] = cl[5], cl[3]
            
        return "oct_" + "_".join(map(str, cl))

#
# Unit tests for the Hull class
#

class HullTests(unittest.TestCase):

    def testBlank(self):
        self.failUnless(True)
    
        
def main():
    unittest.main()

if __name__ == '__main__':
    main()
