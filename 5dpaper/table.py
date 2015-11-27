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

def query():
   cur.execute("select * from tree order by r_bound desc")

def bbox(r):
    s = map(int,r.split(' '))
    return "(" + str((s[0] - 18) / 4) + ", " + str((s[2]-18)/4) + ", " + str((s[4]-18)/4)\
            + ", " + str((s[6]-18)/4) + ")"

def start_table():
    print """%
    \\begin{tabular}{|l|l|l|l|}
    \\hline
    No &  Bounding box &  Max size  \\\\
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

for row in rows:
    if row[7] == None:
        continue
    if row[7] < 83:
        continue
    
    print str(no) + "&" + bbox(row[4]) + "& " + str(row[7]) + "\\\\"
    no = no + 1
    if no == 19:
        end_table()
        print "\\hspace*{5mm}"
        start_table()
            
end_table()
