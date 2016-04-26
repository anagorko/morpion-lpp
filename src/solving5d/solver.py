#!/usr/bin/env python

import argparse, sqlite3, os, subprocess, timeit
from gurobipy import *

generator = "../../build/generator/generator"

parser = argparse.ArgumentParser()
parser.add_argument('--db', help='directory where the problems are stored', action='store', default='hard_cases/')
parser.add_argument('--problem', help='number of the problem', type=int, default=-1)
parser.add_argument('--ord', help='use branch priorities', action='store_true')
#parser.add_argument('--status', action='store_true')
args = parser.parse_args()

print "\033[1;34mSolver for the hard cases of Morpion 5D\033[0m"
print ""
#print "Use --status to see the hard case list"
#print ""

###
### Create problem directory if it does not exist
###

directory = args.db
if not os.path.exists(directory):
    os.makedirs(directory)

###
### Open or create the database
###

conn = sqlite3.connect(directory + '/status.db')
conn.isolation_level = None
cur = conn.cursor()

cur.execute('SELECT name FROM sqlite_master WHERE type="table" AND name="problems";')

if cur.fetchone() is None:
    print "\033[1mFirst run...\033[0m"
    
    cur.execute('CREATE TABLE problems (\
        problem_id INTEGER, \
        filename VARCHAR(64), \
        box VARCHAR(64), \
        parameters VARCHAR(128), \
        assigned TIMESTAMP, \
        solved BOOLEAN, \
        solution_time INTEGER, \
        solution_type VARCHAR(32), \
        solution_objective FLOAT \
      )')

    # insert the problems into the database

    problem_list = [ [0, (0,0,0,0) ], [ 1, (4,3,1,1) ], [ 2, (4,3,1,2) ], [ 3, (4,3,1,3) ], [ 4, (4,2,1,2) ],
      [ 5, (4,2,2,2) ], [ 6, (5,2,2,1) ], [ 7, (5,2,1,2) ], [ 8, (5,2,2,2) ],
      [ 9, (3,3,2,1) ], [ 10, (3,3,2,2) ], [ 11, (4,3,2,1) ], [ 12, (4,3,3,1) ],
      [ 13, (4,3,2,2) ], [ 14, (4,3,2,3) ], [ 15, (4,3,0,2) ], [ 16, (3,2,1,2) ],
      [ 17, (3,2,2,2) ], [ 18, (5,2,1,1) ], [ 19, (3,3,3,1) ], [ 20, (4,3,3,2) ],
      [ 21, (5,3,1,1) ], [ 22, (5,3,1,2) ], [ 23, (4,3,0,3) ], [ 24, (4,4,1,0) ],
      [ 25, (4,4,2,0) ], [ 26, (4,4,1,1) ], [ 27, (4,4,2,1) ], [ 28, (4,4,3,1) ] ]

    for problem in problem_list:
        # Create database record
        
        problem_id = problem[0]
        filename = 'rect_%d_%d_%d_%d' % (problem[1][0], problem[1][1], problem[1][2], problem[1][3])
        box = str(problem[1])
        halfplanes = '%d 0 %d 0 %d 0 %d 0' % (problem[1][0] * 4 + 18, problem[1][1] * 4 + 18,
            problem[1][2] * 4 + 18, problem[1][3] * 4 + 18)
        parameters = ' -w 40 -h 40 --halfplanes ' + halfplanes + ' -v 5d --exact --dot-acyclic --potential -o ' + directory + '/' + filename + '.lp --ord'

        cur.execute("INSERT into problems VALUES (%d, '%s', '%s', '%s', NULL, 0, NULL, NULL, NULL)" % (problem_id, filename, box, parameters))        

        print "      ... created problem " + str(problem_id) + " for box " + box,

        # Create problem .lp files

        cmd = generator + parameters
        print " ... running generator."
        subprocess.check_output(cmd, shell=True)
    
    print ""
    print "\033[1m... preparation done.\033[0m"
    print ""
    
#if args.status:
#    print "--status not implemented."
    # FIXME
#    pass

cur.close()
conn.close()

while True:
    conn = sqlite3.connect(directory + '/status.db')
    conn.isolation_level = None
    cur = conn.cursor()
    
    # fetch unassigned problem description or report that there are none
    
    qry = 'SELECT * FROM problems WHERE assigned is NULL and not solved'
    if args.problem > -1:
        qry = qry + ' and problem_id = ' + str(args.problem)
        
    cur.execute(qry)
    
    problem = cur.fetchone()
    
    if problem is None:
        if args.problem == -1:
            print "There are no unassigned solved problems in the database. ",
        else:
            print "The specified problem does not exists, is assigned or is solved. ",
        print "Terminating."
        
        exit(0)
    
    print "\033[1;34mFetched problem " + str(problem[0]) + "\033[0m"
    problem_id = problem[0]
    
    cur.execute("UPDATE problems SET assigned = datetime('now') WHERE problem_id = " + str(problem[0]))
    cur.close()
    conn.close()
    
    # run the solver
    
    filename = problem[1]
    model = read(directory + '/' + filename + '.lp')
    model.params.LogFile = directory + '/' + filename + '.log'
    model.params.Threads = 1
    model.params.MIPFocus = 3
    model.params.Cutoff = 82.9
    
    start = timeit.default_timer()
    model.optimize()
    stop = timeit.default_timer()
    
    conn = sqlite3.connect(directory + '/status.db')
    conn.isolation_level = None
    cur = conn.cursor()
        
    if model.Status != GRB.OPTIMAL and model.Status != GRB.INFEASIBLE and \
        model.Status != GRB.INF_OR_UNBD and\
        not (model.Status == GRB.CUTOFF):
    
        print "\033[1;31mComputation interrupted, terminating.\033[0m"
        
        cur.execute("UPDATE problems SET assigned = NULL WHERE problem_id = " + str(problem[0]))
        cur.close()
        conn.close()
        exit(0)
    
    # save the search results
    
    if model.Status == GRB.CUTOFF:
        solution_type = 'CUTOFF'
    elif model.Status == GRB.OPTIMAL:
        solution_type = 'OPTIMAL'
    else:
        solution_type = 'ERROR'
        
    solution_objective = model.ObjBound
    solution_time = stop - start
    
    cur.execute("UPDATE problems SET solved = 1, solution_time = '%d', solution_type = '%s', solution_objective=%f WHERE problem_id=%d" % (solution_time, solution_type, solution_objective, problem_id))
    cur.close()
    conn.close()

    if args.problem > -1:
        break
