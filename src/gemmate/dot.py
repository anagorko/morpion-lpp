#!/usr/bin/env python

import unittest
import numbers

#
# Dot class
#

class Dot:
    max_range = 1000
    
    def __init__(self):
        self.x = 0
        self.y = 0

    def __init__(self, x, y):
        self.x = x
        self.y = y

        assert isinstance(self.x, numbers.Integral)
        assert isinstance(self.y, numbers.Integral)
        assert abs(self.x) < self.max_range
        assert abs(self.y) < self.max_range
        
    def __add__(self, d):
        return Dot(self.x+d.x, self.y + d.y)

    def __str__(self):
        return "(" + str(self.x) + "," + str(self.y) + ")"

    def __repr__(self):
        return self.__str__();
        
    def __eq__(self, d):
        return self.x == d.x and self.y == d.y

    def __hash__(self):
        return self.y * 2 * self.max_range + self.y

    def __mul__(self, x):
        if isinstance(x, numbers.Integral):
            return Dot(self.x * x, self.y * x)
        else:
            return self.x * x.x + self.y * x.y
        
    __rmul__ = __mul__

    def __neg__(self):
        return Dot(-self.x, -self.y)

#
# Unit tests for the Dot class
#

class DotTests(unittest.TestCase):

    def testAddition(self):
        self.failUnless(Dot(2,3) + Dot(-1,2) == Dot(1,5))
        self.failIf(Dot(2,3) + Dot(-1,2) == Dot(1,3))
        self.failIf(Dot(2,3) + Dot(-1,2) == Dot(1,2))
        self.failIf(Dot(2,3) + Dot(-1,2) == Dot(-1,5))
        self.failIf(Dot(2,3) + Dot(-1,2) == Dot(-1,2))
    
    def testNegation(self):
        self.failUnless(Dot(7,8) + -Dot(7,8) == Dot(0,0))
        
    def testInnerProduct(self):
        self.failUnless(Dot(2,1) * Dot(-1,2) == 0)
        self.failUnless(Dot(2,1) * Dot(2,1) == 5)

    def testScalarMultiplication(self):
        self.failUnless(Dot(3,7) * -3 == -3 * Dot(3,7))
        self.failUnless(-1 * Dot(-1,-3) == Dot(1,3))
        self.failUnless(-Dot(-1,-3) == Dot(1,3))
        
def main():
    unittest.main()

if __name__ == '__main__':
    main()
