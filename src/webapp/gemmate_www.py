# -*- coding: utf-8 -*-

import webapp2
from google.appengine.ext import ndb
from google.appengine.ext.webapp import template
import os
from hull import Rectangle, Octagon
from gemmate_model import Tree, Problem
from hull import Rectangle, Octagon, Hull
from sets import Set
import copy

# Get full path of a template file
def get_template_path(template_file):
    return os.path.join(os.path.dirname(__file__), "templates/", template_file)


class Init(webapp2.RequestHandler):
    def create_tree(self, id, name, variant, symmetric, shape, bound):
        tree = Tree(id=id, name=name, variant=variant, symmetric=symmetric, shape=shape, known_lower_bound=bound)

        if shape == 'R':
            board = Rectangle()
        else:
            board = Octagon()
        
        board_id = board.symmetryClass()
        
        self.response.write(board_id)
        
        problem = Problem(hull=board_id, type = Problem.FEASIBLE_HULL, id = board_id + "_" + str(Problem.FEASIBLE_HULL), solved = False,  parent=tree.key)
        problem.put()

        tree.root = problem.key
        tree.put()
        
        self.response.write(' Created tree "%s"\n' % (tree.name))

    def get(self):
        self.response.headers['Content-Type'] = 'text/plain'
        self.response.write('Cleaning the database\n')
        
        to_delete = []
        for cl in [ Tree, Problem ]:
            for r in cl.query().fetch(keys_only=True):
                self.response.write(' Deleting %s key %s\n' % (cl.__name__, str(r.id())))
                to_delete.append(r)
        ndb.delete_multi(to_delete)
        
        self.response.write('\nCreating default trees\n')

        self.create_tree('5DR', 'Morpion 5D with rectangular hulls', '5D', False, 'R', 82)
        self.create_tree('5DO', 'Morpion 5D with octagonal hulls', '5D', False, 'O', 82)
        self.create_tree('5DRs', 'Symmetric Morpion 5D with rectangular hulls', '5D', True, 'R', 68)
        self.create_tree('5DOs', 'Symmetric Morpion 5D with octagonal hulls', '5D', True, 'O', 68)

        self.create_tree('5TRs', 'Symmetric Morpion 5T with rectangular hulls', '5T', True, 'R', 136)
        
class Browser(webapp2.RequestHandler):
    def get(self):        
        self.response.headers['Content-Type'] = 'text/html'
        
        if 'problem' not in self.request.arguments():
            trees = Tree.query().fetch()
            tt = []
            for t in trees:
                t.tree_id = t.key.id()
                t.root_id = t.root.urlsafe()
                
                tmp = Problem.query(ancestor=t.key).order(-Problem.time_assigned).fetch(limit=1)
                if len(tmp) > 0:
                    t.last_activity = tmp[0].time_assigned
                    
                tt.append(t)

            self.response.out.write(template.render(get_template_path('browse_trees.html'), {"trees": tt} ))
        else:
            # Get problem information
            
            problem_key = ndb.Key(urlsafe=self.request.get("problem"))
            problem = problem_key.get()
            if problem is None:
                self.error(404)
                self.response.out.write('Problem not found')
                return

            # Retrieve tree
            tree = problem.key.parent().get()

            # Get problem information
            problems = Problem.query(Problem.hull==problem.hull, ancestor=tree.key).fetch()
            
            # Generate child problems
            
            hull = Hull.createFromId(problem.hull)    
            hull_list = Set()
            
            children = []
            
            for d in hull.boundary():
                hull_copy = copy.deepcopy(hull)
                hull_copy.put(d)
            
                if tree.symmetric:
                    hull_copy.put(-d)

                board_id = hull_copy.symmetryClass()

                if board_id in hull_list:
                    continue
                hull_list.add(board_id)
            
                child_key = ndb.Key(Problem, board_id + "_" + str(problem.type), parent=tree.key)                
                child = child_key.get()
                
                if child is not None:
                    children.append({ "name" : board_id, "key" : child_key.urlsafe(), "board": hull_copy.pr()})
                
            self.response.out.write(template.render(get_template_path('browse_problem.html'), {"problems": problems, "children": children} ))

class ProblemList(webapp2.RequestHandler):
    def get(self):        
        self.response.headers['Content-Type'] = 'text/html'
        
app = webapp2.WSGIApplication([
    ('/', Browser),
    ('/list', ProblemList),
    ('/init', Init)
], debug=True)
