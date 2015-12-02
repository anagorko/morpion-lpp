#!/usr/bin/python
import MySQLdb
import os
import sys
from gurobipy import *
import time
import math

db = MySQLdb.connect(host="127.0.0.1", # your host, usually localhost
                     user="henrykm", # your username
                     passwd="MumigMyppEb4", # your password
                     db="henrykm",
                     port=9870) # name of the data base

cur = db.cursor() 

def symmetryClass(b):
    if b[0] == b[1] and b[1] == b[2] and b[2] == b[3]:
        return b

    while b[0] < b[1] or b[0] < b[2] or b[0] < b[3]:
        b = b[1:] + b[:1]

    if (b[1] < b[3]):
        b[1], b[3] = b[3], b[1]

    if (b[1] < b[0] and b[2] < b[0] and b[3] < b[0]):
        return b

    if b[0] == b[3] or b[0] == b[1]:
        while not b[1] == b[0]:
            b = b[1:] + b[:1]
        if (b[2] < b[3]):
            b[2], b[3] = b[3], b[2]
    return b

def query():
   cur.execute("select * from tree order by r_bound desc")

def box(r):
    s = map(int, r.split(' '))
    s = [ s[0], s[2], s[4], s[6] ]
    return symmetryClass(s)
    
def bbox(r):
    s = box(r)
    return "(" + str((s[0] - 18) / 4) + ", " + str((s[1]-18)/4) + ", " + str((s[2]-18)/4)\
            + ", " + str((s[3]-18)/4) + ")"

    
def start_table():
    print """%
    \\begin{tabular}{|l|l|l|l|}
    \\hline
    No & BBox & Size  \\\\
    \\hline%
    """

def end_table():
    print """%
    \\hline
    \\end{tabular}%"""

query()
rows = cur.fetchall()

no = 1
start_table()

done = []

for row in rows:
    if row[7] == None:
        continue
    if row[7] < 83:
        continue

    if box(row[4]) in done:
        continue
    
    done.append(box(row[4]))

    print str(no) + "&" + bbox(row[4]) + "& " + str(row[7]) + "\\\\"
    no = no + 1
    if no == 11 or no == 21:
        end_table()
        print "\\hspace*{5mm}"
        start_table()
            
end_table()
