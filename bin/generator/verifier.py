#!/usr/bin/python
import MySQLdb
import os
import sys
from gurobipy import *
import time
import math

echo = True
best_known_bound = 82

#db = MySQLdb.connect(host="127.0.0.1", # your host, usually localhost
#                     user="morpion", # your username
#                     passwd="morpion", # your password
#                     db="morpion_gemmate",
#                     port=8889) # name of the data base

db = MySQLdb.connect(host="127.0.0.1", # your host, usually localhost
                     user="henrykm", # your username
                     passwd="MumigMyppEb4", # your password
                     db="henrykm",
                     port=9870) # name of the data base

# you must create a Cursor object. It will let
# you execute all the queries you need
cur = db.cursor() 

def query():
   cur.execute("select * from tree where (not solved or solved is null) and (not locked or locked is null) order by e_bound desc")

def lock():
  cur.execute("lock tables tree write")

def unlock():
  cur.execute("unlock tables")

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
  lock()
  query()
  rows = cur.fetchall()
  if not rows:
    unlock()
    break
  row = rows[0]

  def update(colname, value, echo = False):
    query = "UPDATE tree SET " + colname + "=" + str(value) + " WHERE problem_id = '"+str(row[1] + "'")
    if echo:
      print query
    cur.execute(query)

  update("locked", 1)
  unlock()

  start_time = time.time() * 1000.0
  
  os.system("./generator -o " + str(row[1]) + ".lp " + str(row[2]))

  callback_interrupt = False
  model = read(str(row[1]) + ".lp")
  model.params.threads = 1
#  model.params.MIPfocus = row[5]
  model.params.MIPfocus = 3
  model.optimize(gurobi_callback)

  elapsed_time = time.time() * 1000.0 - start_time
  
  if model.Status != GRB.OPTIMAL and model.Status != GRB.INFEASIBLE and \
          model.Status != GRB.INF_OR_UNBD and not callback_interrupt and\
          not (model.Status == GRB.CUTOFF and problem['cutoff']):
      update("locked", 0)
      print "Computation interrupted, terminating."
      break

  lock()

  if model.SolCount > 0:
    update("r_feasible", 1)
    update("r_bound", model.ObjBound)
    update("r_time", elapsed_time)  
    update("solved", True)
    
    model.write(str(row[1]) + ".sol")
  else:
    update("r_feasible", 0)
    update("r_bound", -1.0)
    update("r_time", elapsed_time)
    update("solved", True)

  update("locked", 0)
  unlock()

db.commit()
