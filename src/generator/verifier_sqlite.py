#!/usr/bin/python
import os
import sys
from gurobipy import *
import time
import math
import sqlite3

""" 
 First create a database

   sqlite3 data?.db

 Then inside of the database perform the following 

   sqlite>  create table cases(problem_id, problem, command, r_time, r_solved, r_locked, r_feasible, r_bound);
   sqlite> .separator "\t"
   sqlite> .import data?.csv cases  

 It may be useful to install a browser extension to browse the current state of the database
""" 

echo = True
best_known_bound = 82

conn = sqlite3.connect('data1.db')
conn.isolation_level = None
cur = conn.cursor()

def query():
   cur.execute("select * from cases where (not r_solved or r_solved is null) and (not r_locked or r_locked is null)")

callback_interrupt = False

def gurobi_callback(model, where):
  global callback_interrupt
  
  if where == GRB.callback.MIP:
    sols = model.cbGet(GRB.callback.MIP_SOLCNT)
    bound = model.cbGet(GRB.callback.MIP_OBJBND)
        
    if sols > 0 and bound <= best_known_bound:
        callback_interrupt = True
        print "Stopping computation (feasible model and objective smaller than known bound)"
        model.terminate()

while True:
  query()
  rows = cur.fetchall()
  if not rows:
    break
  row = rows[0]
  row_name = str(row[1])
  row_name = row_name.replace("[","")
  row_name = row_name.replace("]","")
  row_name = row_name.replace(", ","_")

  def update(colname, value, echo = False):
    query = "UPDATE cases SET " + colname + "=" + str(value) + " WHERE problem_id = '"+str(row[0] + "'")
    if echo:
      print query
    cur.execute(query)

  update("r_locked", 1)

  start_time = time.time() * 1000.0
  
  print "./generator -o " + row_name + ".lp " + str(row[2])
  os.system("./generator -o " + row_name + ".lp " + str(row[2]))

  callback_interrupt = False
  model = read(row_name + ".lp")
  model.params.threads = 1
  model.params.MIPfocus = 3
  model.optimize(gurobi_callback)

  elapsed_time = time.time() * 1000.0 - start_time
  
  if model.Status != GRB.OPTIMAL and model.Status != GRB.INFEASIBLE and \
          model.Status != GRB.INF_OR_UNBD and not callback_interrupt and\
          not (model.Status == GRB.CUTOFF and problem['cutoff']):
      update("r_locked", 0)
      print "Computation interrupted, terminating."
      break

  if model.SolCount > 0:
    update("r_feasible", 1)
    update("r_bound", model.ObjBound)
    update("r_time", elapsed_time)  
    update("r_solved", 1)
    
    model.write(row_name + ".sol")
  else:
    update("r_feasible", 0)
    update("r_bound", -1.0)
    update("r_time", elapsed_time)
    update("r_solved", 1)

  update("r_locked", 0)
