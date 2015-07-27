#!/usr/bin/env python

import unittest
import numbers
import dot

#
# Segment class
#

class Segment:
    max_range = 1000
    
    def __init__(self):
        self.p = dot.Dot(0,0)
        self.q = dot.Dot(0,0)

    def __init__(self, p, q):
        self.p = p
        self.q = q

        assert isinstance(self.p, dot.Dot)
        assert isinstance(self.q, dot.Dot)
        
    def __str__(self):
        return str(self.p) + " -- " + str(self.q)

    def __repr__(self):
        return self.__str__();
        
    def __eq__(self, d):
        return self.p == d.p and self.q == d.q

#
# Unit tests for the Dot class
#

class DotTests(unittest.TestCase):
    def testNothing(self):
        return
        
def main():
    unittest.main()

if __name__ == '__main__':
    main()
