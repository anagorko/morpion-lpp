#!/usr/bin/env python

import requests
import json
import os
import tempfile
import math
from gurobipy import *

import argparse
parser = argparse.ArgumentParser()

parser.add_argument("--host")
parser.add_argument("--tree")

args = parser.parse_args()

host = "http://localhost:10080/"
tree = "5DR"
generator = "../../build/generator/generator"
runs = -1

if args.host is not None:
    host = args.host

if args.tree is not None:
    tree = args.tree
    
def cutoff_callback(model, where):
  global callback_interrupt
  
  if where == GRB.callback.MIP:
    sols = model.cbGet(GRB.callback.MIP_SOLCNT)
    bound = model.cbGet(GRB.callback.MIP_OBJBND)
    
    if sols > 0 and math.floor(bound) <= best_known_bound:
        callback_interrupt = True
        print "Stopping computation (feasible model and objective smaller than known bound)"
        model.terminate()

def getProblem():
    url = host + "ProblemDB.getProblem"
    headers = {'content-type': 'application/json'}
    payload = {
        "tree_id": tree
    }
    return requests.post(url, json.dumps(payload), headers=headers).json()

while runs != 0:
    problem = getProblem()

    if 'generator_command' not in problem:
        break

    print problem['generator_command']
    print problem['problem_id']
    print problem['lower_bound']
    
    best_known_bound = problem['lower_bound']
    
    (fd, filename) = tempfile.mkstemp()

    filename = filename + ".lp"

    cmd = generator + " -p -w 30 -h 30 " + problem['generator_command'] + " -o " + filename
    print "Running " + cmd
    os.system(cmd)

    model = read(filename)
    os.remove(filename)
    
    model.params.Threads = 1
    callback_interrupt = False
    model.optimize(cutoff_callback)
    
    if model.Status != GRB.OPTIMAL and model.Status != GRB.INFEASIBLE and \
            model.Status != GRB.INF_OR_UNBD and not callback_interrupt:
        print "Computation interrupted, terminating."
        break

    url = host + "ProblemDB.postSolution"
    headers = {'content-type': 'application/json'}
    
    if model.SolCount > 0:
        payload = {
            "problem_id": problem['problem_id'],
            "solved": True,
            "computation_time": 1,
            "solution_type": "FEASIBLE",
            "upper_bound": model.ObjBound,
            "lower_bound": model.ObjVal
        }
    else:
        payload = {
            "problem_id": problem['problem_id'],
            "solved": True,
            "computation_time": 1,
            "solution_type": "INFEASIBLE"
        }
        
    print requests.post(url, json.dumps(payload), headers=headers)
    
    runs = runs - 1
