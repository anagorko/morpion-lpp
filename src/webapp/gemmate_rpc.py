from protorpc import messages, message_types
from protorpc import remote
from protorpc.wsgi import service

from google.appengine.ext import ndb
import gemmate_model

from hull import Rectangle, Octagon, Hull

from sets import Set
import copy
import datetime

import logging

class ProblemRequest(messages.Message):
    tree_id = messages.StringField(1, required=False)

class ProblemResponse(messages.Message):
    problem_id = messages.StringField(1,required=True)
    generator_command = messages.StringField(2, required=False)
    lower_bound = messages.IntegerField(3, required=False)
    cutoff = messages.BooleanField(4, required=False)
    
class SolutionRequest(messages.Message):
    problem_id = messages.StringField(1,required=True)
    solved = messages.BooleanField(2)
    computation_time = messages.IntegerField(3)
    solution_type = messages.StringField(4) # FEASIBLE / INFEASIBLE / CUTOFF
    upper_bound = messages.FloatField(5)
    lower_bound = messages.FloatField(6)
    
class ProblemDB(remote.Service):
    @remote.method(ProblemRequest, ProblemResponse)
    def getProblem(self, request):
        # Retrieve tree key
        tree_key = ndb.Key(gemmate_model.Tree, request.tree_id)
        
        # Get tree entity
        tree = tree_key.get()
        
        # Retrieve unsolved problem from a database
        problems = gemmate_model.Problem.query(gemmate_model.Problem.solved==False, ancestor=tree_key).order(gemmate_model.Problem.type, gemmate_model.Problem.time_assigned, -gemmate_model.Problem.upper_bound).fetch(limit = 1)
        
        if len(problems) == 0:
            return ProblemResponse(problem_id="None")
        problem = problems[0]
        
        # Mark problem as assigned now
        problem.time_assigned = datetime.datetime.now()
        problem.put()
                
        # Create generator options
        hull = Hull.createFromId(problem.hull)
        
        command = '--exact --halfplanes ' + hull.halfplanes() + ' -v ' + tree.variant
        
        if tree.symmetric:
            command = command + ' --symmetric'
            
        if tree.shape == 'R':
            command = command + ' --rhull'
        else:
            command = command + ' --hull'

        if problem.type == gemmate_model.Problem.FEASIBLE_HULL:
            command = command + ' --potential'
            # no extra parameters needed for CYCLIC problems
        elif problem.type == gemmate_model.Problem.REGULAR:
            command = command + ' --dot-acyclic'

        # Cutoff allowed?
        if problem.type == gemmate_model.Problem.FEASIBLE_HULL:
            cutoff = False
        else:
            cutoff = True

        return ProblemResponse(problem_id = problem.key.urlsafe(), generator_command = command, lower_bound=tree.known_lower_bound,cutoff=cutoff)

    @remote.method(SolutionRequest, message_types.VoidMessage)
    def postSolution(self, request):        
        # Retrieve problem
        problem = ndb.Key(urlsafe=request.problem_id).get()
        
        updated_problems_list = []
        
        # Store solution
        problem.solved = True
        problem.computationTime = request.computation_time;
                
        if request.solution_type == "FEASIBLE":
            problem.upper_bound = request.upper_bound
            problem.lower_bound = request.lower_bound
            problem.solution_type = "FEASIBLE"                        

            # gemmate problems
            if problem.type == gemmate_model.Problem.FEASIBLE_HULL:
                updated_problems_list = gemmate_model.gemmateProblem(problem)

            tree_bound = problem.key.parent().get().known_lower_bound
            
            if problem.type == gemmate_model.Problem.FEASIBLE_HULL and problem.upper_bound > tree_bound:
                # create cyclic problem
                cyclic_problem_id = problem.hull + "_" + str(gemmate_model.Problem.CYCLIC)
                cyclic_problem = gemmate_model.Problem(hull=problem.hull, type=gemmate_model.Problem.CYCLIC,id=cyclic_problem_id,solved=False,parent=problem.key.parent())
    
                logging.info("Generating new CYCLIC problem with Id " + cyclic_problem_id)
                updated_problems_list.append(cyclic_problem)
            
            if problem.type == gemmate_model.Problem.CYCLIC and problem.upper_bound > tree_bound:
                # create regular problem
                regular_problem_id = problem.hull + "_" + str(gemmate_model.Problem.REGULAR)
                regular_problem = gemmate_model.Problem(hull=problem.hull, type=gemmate_model.Problem.REGULAR,id=regular_problem_id,solved=False,parent=problem.key.parent())
    
                logging.info("Generating new REGULAR problem with Id " + regular_problem_id)
                updated_problems_list.append(regular_problem)
        elif request.solution_type == "CUTOFF":
            problem.upper_bound = request.upper_bound
            problem.solution_type = "CUTOFF"
        else:
            problem.solution_type = "INFEASIBLE"

        updated_problems_list.append(problem)
            
        ndb.put_multi(updated_problems_list)
        
        return message_types.VoidMessage()

app = service.service_mappings([
    ('/ProblemDB.*', ProblemDB)
])
