# verifier.py 
# a test program which runs the optimizer on a single instance

import os
import sys
from gurobipy import *
import time
import math

callback_interrupt = False
best_known_bound = 82

def gurobi_callback(model, where):
  global callback_interrupt
  
  if where == GRB.callback.MIP:
    sols = model.cbGet(GRB.callback.MIP_SOLCNT)
    bound = model.cbGet(GRB.callback.MIP_OBJBND)
        
    if sols > 0 and bound <= best_known_bound:
        callback_interrupt = True
        print "Stopping computation (feasible model and objective smaller than known bound)"
        model.terminate()

# a typical larger instance 
# os.system("./generator -o 5_4_2_4.lp  -p --exact -w 40 -h 40 --halfplanes 38 0 34 0 26 0 34 0 -v 5D --rhull --potential")
# a fast instance
os.system("./generator -o 4_2_0_2.lp  -p --exact -w 40 -h 40 --halfplanes 26 0 34 0 26 0 18 0 -v 5D --rhull --potential")


callback_interrupt = False
# a typical larger instance 
# model = read("5_4_2_4.lp")
# a fast instance
model = read("4_2_0_2.lp")

model.params.threads = 1
model.params.MIPfocus = 3
model.optimize(gurobi_callback)
