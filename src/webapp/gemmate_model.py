from google.appengine.ext import ndb

from hull import Rectangle, Octagon, Hull
from sets import Set
import copy
import logging

class Tree(ndb.Model):
    name = ndb.StringProperty()
    
    variant = ndb.StringProperty()      # 5D / 5T
    symmetric = ndb.BooleanProperty()
    shape = ndb.StringProperty()       # O - octagon / R - rectangle
    
    known_lower_bound = ndb.IntegerProperty()
    
    root = ndb.KeyProperty()
        
class Problem(ndb.Model):
    FEASIBLE_HULL = 1
    CYCLIC = 2
    REGULAR = 3
    
    hull = ndb.StringProperty()
    type = ndb.IntegerProperty()
    
    solved = ndb.BooleanProperty()
    solution_type = ndb.StringProperty()

    time_assigned = ndb.DateTimeProperty()
    
    # Solution statistics
    computation_time = ndb.IntegerProperty()
    upper_bound = ndb.FloatProperty()
    lower_bound = ndb.FloatProperty()

class TreeSummary(ndb.Model):
    tree = ndb.KeyProperty()
    
    gemmate_solved_nodes = ndb.IntegerProperty(indexed=False)
    gemmate_unsolved_nodes = ndb.IntegerProperty(indexed=False)
    
    gemmate_upper_bound = ndb.IntegerProperty(indexed=False)
    gemmate_lower_bound = ndb.IntegerProperty(indexed=False)
    
    gemmate_computation_time = ndb.IntegerProperty(indexed=False)

# Returns a sequence of descendant hull id's
def getDescendantHulls(hull_id, symmetric):
    hull = Hull.createFromId(hull_id)    
    
    hull_id_set = Set()
                        
    for d in hull.boundary():
        hull_copy = copy.deepcopy(hull)
        hull_copy.put(d)
            
        if symmetric:
            hull_copy.put(-d)

        descendant_hull_id = hull_copy.symmetryClass()

        if descendant_hull_id in hull_id_set:
                continue
        
        hull_id_set.add(descendant_hull_id)

    return hull_id_set

# Creates descendant problems
def gemmateProblem(problem):
    # Retrieve tree
    tree = problem.key.parent().get()
                
    descendant_hulls = getDescendantHulls(problem.hull, tree.symmetric)
    
    to_put = []

    for hull_id in descendant_hulls:
        problem_id = hull_id + "_" + str(Problem.FEASIBLE_HULL)
        if ndb.Key(Problem,problem_id, parent=tree.key).get() is None:
            logging.info("Generating new problem with Id " + problem_id)
    
            p = Problem(hull=hull_id, type = Problem.FEASIBLE_HULL, id = problem_id, solved = False,  parent=tree.key)
            to_put.append(p)
        else:
            logging.info("Skipping duplicate problem with Id " + problem_id)

    return to_put
