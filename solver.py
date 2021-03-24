"""
A Morpion 5D position solver.

TODO list:

- compute all three-cycles up front
- compute and periodically display stats
- write a logger
"""

import argparse
from typing import List, Tuple, Dict, Optional
import time
from termcolor import colored
import os
from dataclasses import dataclass, field

import pyscipopt
import networkx

from lazy_constraints.grid import Game, BoundingBox, Dot, Move, HalfSegment


def printNodeBranch(node: Optional[pyscipopt.scip.Node]):
    if node is None:
        return
    print(node.getDepth(), node.getType(), node.getLowerbound(), node.getNAddedConss(), node.getParentBranchings())

    printNodeBranch(node.getParent())


class EventPrinter(pyscipopt.Eventhdlr):
    def eventexec(self, event):
        eventName = {
            pyscipopt.SCIP_EVENTTYPE.VARFIXED: 'VARFIXED',
            pyscipopt.SCIP_EVENTTYPE.NODEBRANCHED: 'NODEBRANCHED',
            pyscipopt.SCIP_EVENTTYPE.NODEFOCUSED: 'NODEFOCUSED',
            pyscipopt.SCIP_EVENTTYPE.NODEFEASIBLE: 'NODEFEASIBLE',
            pyscipopt.SCIP_EVENTTYPE.NODEINFEASIBLE: 'NODEINFEASIBLE',
            pyscipopt.SCIP_EVENTTYPE.FIRSTLPSOLVED: 'FIRSTLPSOLVED',
            pyscipopt.SCIP_EVENTTYPE.NODESOLVED: 'NODESOLVED'
        }
        print('\n\n\n\n-------------------')
        print('EVENT', eventName[event.getType()])
        printNodeBranch(self.model.getCurrentNode())
        print('-------------\n\n\n\n')

    def eventinit(self):
        self.model.catchEvent(pyscipopt.SCIP_EVENTTYPE.VARFIXED, self)
        self.model.catchEvent(pyscipopt.SCIP_EVENTTYPE.NODEBRANCHED, self)
        self.model.catchEvent(pyscipopt.SCIP_EVENTTYPE.NODEFOCUSED, self)
        self.model.catchEvent(pyscipopt.SCIP_EVENTTYPE.FIRSTLPSOLVED, self)
        self.model.catchEvent(pyscipopt.SCIP_EVENTTYPE.NODESOLVED, self)

    def eventexit(self):
        self.model.dropEvent(pyscipopt.SCIP_EVENTTYPE.VARFIXED, self)


@dataclass
class Statistics:
    """Statistics of the computation."""

    non_integral: int = 0
    """Number of times a solution was rejected because variables were not integral."""
    cycling: Dict[int, int] = field(default_factory=dict)
    """Number of times a solution was rejected because there was a cycle of a given length.
    The key is 0 if no effort was made to find a min-length cycle."""
    constraint_violation: int = 0
    """Number of times a solution was rejected because a linear constraint of the problem was violated."""
    cycle_detection_time: float = 0.0
    shortest_cycle_detection_time: float = 0.0
    shortest_cycle_aborts: int = 0
    solutions_found: int = 0

    cutoffs: int = 0
    reduced_domain: int = 0
    reduced_domain_triangle: int = 0
    constraints_added: int = 0

    last_report: float = 0.0
    interval: float = 1.0


def print_stats(stats: Statistics):
    print('Cycle detection times', stats.cycle_detection_time, '/', stats.shortest_cycle_detection_time, ',',
          stats.shortest_cycle_aborts, 'aborts.')
    print('Domain reductions', stats.reduced_domain, 'triangle', stats.reduced_domain_triangle,
          'cutoffs', stats.cutoffs)
    print('Constraints added', stats.constraints_added)
    print('Non integral', stats.non_integral)


@dataclass
class Problem:
    """Metadata of the problem and computation statistics."""

    model: pyscipopt.Model
    """Model with constraints."""
    dot: Dict[Dot, pyscipopt.scip.Variable]
    """Binary variables corresponding to dots."""
    move: Dict[Move, pyscipopt.scip.Variable]
    """Binary variables corresponding to moves."""
    order: Dict[Dot, pyscipopt.scip.Variable]
    """Variables corresponding to move ordering."""
    starting_dots: List[Dot]
    """The starting cross"""
    stats: Statistics
    """Computation statistics"""
    graph: Optional[networkx.DiGraph] = None
    """A directed graph with moves as vertices and edges connecting moves that place a dot
    with moves that require it."""
    shape: Optional[Tuple[int, int, int, int]] = None
    """Board shape."""


def get_solution(problem: Problem, solution=None) -> Optional[Game]:
    """Convert a solution to a Morpion 5D game."""

    # Check integrality of variables
    for move in problem.move:
        if not problem.model.isFeasIntegral(problem.model.getSolVal(solution, problem.move[move])):
            problem.stats.non_integral += 1
            return None

    for dot in problem.dot:
        if not problem.model.isFeasIntegral(problem.model.getSolVal(solution, problem.dot[dot])):
            problem.stats.non_integral += 1
            return None

    selected_moves = set()
    """A set of moves played in the solution."""
    for move in problem.move:
        if problem.model.isFeasEQ(problem.model.getSolVal(solution, problem.move[move]), 1.0):
            selected_moves.add(move)

    """Look for a cycle in the solution."""
    try:
        start = time.time()
        _ = networkx.find_cycle(problem.graph.subgraph(selected_moves))
        end = time.time()
        problem.stats.cycle_detection_time += end - start

        """There is a cycle in the solution."""
        problem.stats.cycling[0] = problem.stats.cycling.get(0, 0) + 1
        return None
    except networkx.exception.NetworkXNoCycle:
        """No cycle was found."""

        ordered_moves = list()
        placed_dots = set(problem.starting_dots)

        while selected_moves:
            found = False
            for move in selected_moves:
                if set(move.required_dots()).issubset(placed_dots):
                    ordered_moves.append(move)
                    placed_dots.add(move.dot)
                    selected_moves.remove(move)
                    found = True
                    break

            if not found:
                """An LP constraint or bound is violated by the solution."""
                problem.stats.constraint_violation += 1
                return None

        game = Game(variant='5d', board_size=Game.BBox(*problem.shape))
        for move in ordered_moves:
            pos = game.pos_from_coords(move.starting_dot.x, move.starting_dot.y)
            direction = move.direction
            game.make_move(Game.move(pos, direction))

        return game


class LoopChecker(pyscipopt.Conshdlr):
    """Lazy constraint generation for move loops."""

    def __init__(self, problem: Problem, *args, **kwargs):
        super().__init__(*args, **kwargs)

        self.problem = problem

        """Precompute a list of moves that require a dot to be placed."""
        self.moves_requiring_dot: Dict[Dot, List[Move]] = dict()
        for move in self.problem.move:
            for dot in [move.dot] + move.required_dots():
                if dot in self.problem.starting_dots:
                    continue
                if dot not in self.moves_requiring_dot:
                    self.moves_requiring_dot[dot] = list()
                self.moves_requiring_dot[dot].append(move)

        """Precompute a list of moves that place a half-segment on the board."""
        self.moves_placing_hs: Dict[HalfSegment, List[Move]] = dict()
        for move in self.problem.move:
            for hs in move.placed_half_segments():
                if hs not in self.moves_placing_hs:
                    self.moves_placing_hs[hs] = list()
                self.moves_placing_hs[hs].append(move)

        """Precomute a list of moves that place a dot."""
        self.moves_placing_dot: Dict[Dot, List[Move]] = dict()
        for dot in self.problem.dot:
            self.moves_placing_dot[dot] = list()
        for move in self.problem.move:
            self.moves_placing_dot[move.dot].append(move)

    def consprop(self, constraints, nusefulconss, nmarkedconss, proptiming):
        """Propagate variable bounds."""
        elapsed = time.time()
        if elapsed - self.problem.stats.last_report > self.problem.stats.interval:
            print_stats(self.problem.stats)
            self.problem.stats.last_report = elapsed
            self.problem.stats.interval *= 1.1
            if self.problem.stats.interval > 60.0:
                self.problem.stats.interval = 60.0

        reduced_domain = 0
        reduced_domain_triangle = 0
        reduced = False

        """
        Propagation rules.
        
        If dot is 0, then all moves placing dot are 0.
        If all moves placing dot are 0, then dot is 0.
        If move is 1, then placed dot is 1.
        If all moves placing dot are 0, then placed dot is 0.
        If move is 1, then all other moves placing dot are 0.
        """
        while True:
            for dot in self.problem.dot:
                if self.problem.dot[dot].getLbLocal() == 0.0:
                    """If no move that places a dot is played, then dot is not placed."""

                    no_move = True
                    for move in self.moves_placing_dot[dot]:
                        if self.problem.move[move].getUbLocal() == 1.0:
                            no_move = False
                            break
                    if no_move:
                        if self.problem.dot[dot].getUbLocal() == 1.0:
                            self.problem.model.chgVarUb(self.problem.dot[dot], 0.0)
                            reduced_domain += 1

                if self.problem.dot[dot].getUbLocal() == 0.0:
                    """If dot is not placed, then all moves that require the dot are not played."""

                    for move in self.moves_requiring_dot[dot]:
                        if self.problem.move[move].getUbLocal() > 0.0:
                            self.problem.model.chgVarUb(self.problem.move[move], 0.0)
                            reduced_domain += 1
                        if self.problem.move[move].getLbLocal() > 0.0:
                            self.problem.stats.cutoffs += 1
                            return {'result': pyscipopt.SCIP_RESULT.CUTOFF}

            for move in self.problem.move:
                assert self.problem.move[move].getLbLocal() == 0.0 or self.problem.move[move].getLbLocal() == 1.0

                if self.problem.move[move].getLbLocal() == 1.0:
                    """If move is played, then its dot is placed."""
                    if self.problem.dot[move.dot].getLbLocal() < 1.0:
                        self.problem.model.chgVarLb(self.problem.dot[move.dot], 1.0)
                        reduced_domain += 1
                    if self.problem.dot[move.dot].getUbLocal() < 1.0:
                        self.problem.stats.cutoffs += 1
                        return {'result': pyscipopt.SCIP_RESULT.CUTOFF}

                    """If move is played, all moves placing its half-segments are not played."""
                    for hs in move.placed_half_segments():
                        for hs_move in self.moves_placing_hs[hs]:
                            if hs_move == move:
                                continue
                            if self.problem.move[hs_move].getUbLocal() > 0.0:
                                self.problem.model.chgVarUb(self.problem.move[hs_move], 0.0)
                                reduced_domain += 1
                            if self.problem.move[hs_move].getLbLocal() > 0.0:
                                self.problem.stats.cutoffs += 1
                                return {'result': pyscipopt.SCIP_RESULT.CUTOFF}

                    """If move is played, all other moves placing same dot are not played."""
                    for mv in self.moves_placing_dot[move.dot]:
                        if mv == move:
                            continue

                        if self.problem.move[mv].getUbLocal() == 1.0:
                            self.problem.model.chgVarUb(self.problem.move[mv], 0.0)
                            reduced_domain += 1
                        if self.problem.move[mv].getLbLocal() == 1.0:
                            self.problem.stats.cutoffs += 1
                            return {'result': pyscipopt.SCIP_RESULT.CUTOFF}

                    """Triangles."""
                    for b in self.problem.graph.neighbors(move):
                        if b == move:
                            continue
                        if self.problem.move[b].getLbLocal() == 1.0:
                            for c in self.problem.graph.neighbors(b):
                                if self.problem.graph.has_edge(c, move):
                                    if self.problem.move[c].getUbLocal() > 0.0:
                                        self.problem.model.chgVarUb(self.problem.move[c], 0.0)
                                        reduced_domain_triangle += 1
                                    if self.problem.move[c].getLbLocal() > 0.0:
                                        self.problem.stats.cutoffs += 1
                                        return {'result': pyscipopt.SCIP_RESULT.CUTOFF}

            if reduced_domain > 0 or reduced_domain_triangle > 0:
                self.problem.stats.reduced_domain += reduced_domain
                self.problem.stats.reduced_domain_triangle += reduced_domain_triangle
                if reduced:
                    print('extra reductions ', reduced_domain + reduced_domain_triangle)

                reduced = True

                reduced_domain = 0
                reduced_domain_triangle = 0
            else:
                if reduced:
                    return {'result': pyscipopt.SCIP_RESULT.REDUCEDDOM}
                else:
                    return {'result': pyscipopt.SCIP_RESULT.DIDNOTFIND}

    def check_for_cycles(self, solution=None, check_only=False) -> Optional[List[Move]]:
        """Check if the solution contains cycles."""

        selected_moves = list()
        for move in self.problem.graph:
            if not self.model.isFeasIntegral(self.model.getSolVal(solution, self.problem.move[move])):
                self.problem.stats.non_integral += 1
                return []

            if self.model.isFeasEQ(self.model.getSolVal(solution, self.problem.move[move]), 1.0):
                selected_moves.append(move)

        try:
            start = time.time()
            cycle = networkx.find_cycle(self.problem.graph.subgraph(selected_moves))
            end = time.time()
            self.problem.stats.cycle_detection_time += end - start
            self.problem.stats.cycling[0] = self.problem.stats.cycling.get(0, 0) + 1

            cycle = [edge[0] for edge in cycle]

            """We found a cycle."""
            if check_only:
                return cycle

            min_length = len(selected_moves)

            start = time.time()
            min_cycle = find_one_3_cycle(self.problem.graph.subgraph(selected_moves))

            if min_cycle is None:
                for cycle in networkx.simple_cycles(self.problem.graph.subgraph(selected_moves)):
                    if len(cycle) < min_length:
                        min_length = len(cycle)
                        min_cycle = cycle
                    if min_length == 3:
                        break

            end = time.time()
            self.problem.stats.shortest_cycle_detection_time += end - start
            if end - start > 1.0:
                self.problem.stats.shortest_cycle_aborts += 1
            else:
                self.problem.stats.cycling[len(min_cycle)] = self.problem.stats.cycling.get(len(min_cycle), 0) + 1
            return min_cycle
        except networkx.exception.NetworkXNoCycle:
            """There is no cycle."""

            game = get_solution(self.problem)
            if game is not None:
                """We have a valid solution."""
                self.problem.stats.solutions_found += 1

                game.get_grid().get_PILImage().save(f'solution_{len(selected_moves)}.png')
                return None

            """The solution was still infeasible."""
            return []

    def conscheck(self, constraints, solution, checkintegrality, checklprows, printreason, completely):
        print('conscheck', constraints)

        cycle = self.check_for_cycles(solution, check_only=True)

        if cycle is not None:
            return {'result': pyscipopt.SCIP_RESULT.INFEASIBLE}
        else:
            return {'result': pyscipopt.SCIP_RESULT.FEASIBLE}

    def conslock(self, constraint, locktype, nlockspos, nlocksneg):
        print('\n -------------> CONSLOCK ', constraint, '\n')

        for var in self.problem.move:
            self.model.addVarLocks(self.problem.move[var], nlocksneg, nlockspos)
        for dot_order in self.problem.order:
            self.model.addVarLocks(self.problem.order[dot_order], nlocksneg + nlockspos, nlocksneg + nlockspos)

    def consenfolp(self, constraints, nusefulconss, solinfeasible):
        print('consenfolp')

        cycle = self.check_for_cycles(None)

        if cycle is not None:
            cycle_vars = [self.problem.move[vertex] for vertex in cycle]
            self.model.addCons(pyscipopt.quicksum(cycle_vars) <= len(cycle) - 1)

            assert len(cycle) > 0

            self.problem.stats.constraints_added += 1

            return {'result': pyscipopt.SCIP_RESULT.CONSADDED}
        else:
            return {'result': pyscipopt.SCIP_RESULT.FEASIBLE}


def get_bounding_box(reference: Dot, shape: List[int]) -> BoundingBox:
    """A bounding box for the board."""

    upper_left = Dot(reference.x - 3 - shape[3], reference.y - 3 - shape[0])
    lower_right = Dot(reference.x + 6 + shape[1], reference.y + 6 + shape[2])
    rows = lower_right.y - upper_left.y + 2
    cols = lower_right.x - upper_left.x + 2

    return BoundingBox(upper_left, lower_right, rows, cols)


def get_starting_position(shape) -> Tuple[List[Dot], List[Move]]:
    """List of starting dots and a list of all possible moves for a given shape."""
    game = Game(variant='5d', board_size=Game.BBox(*shape))
    grid = game.get_grid()

    box = get_bounding_box(game.dot_from_pos(game.reference), shape)
    moves = grid.get_moves(box)

    return grid.dots.keys(), moves


def create_5d_plus_constraints(starting_dots: List[Dot], moves: List[Move], model: pyscipopt.Model) -> Problem:
    """Creates a 5D+ model for a given move graph."""

    dot_space = set()
    """A set of all dots."""

    for move in moves:
        for dot in move.required_dots():
            dot_space.add(dot)
        dot_space.add(move.dot)

    assert set(starting_dots).issubset(dot_space)

    dot_var = dict()
    """Binary variables corresponding to dots."""
    for dot in dot_space:
        if dot in starting_dots:
            dot_var[dot] = model.addVar(f'dot_{dot}', vtype='B', lb=1, ub=1)
        else:
            dot_var[dot] = model.addVar(f'dot_{dot}', vtype='B', lb=0, ub=1)

    move_var = dict()
    """Binary variables corresponding to moves."""
    for move in moves:
        move_var[move] = model.addVar(f'mv_{move}', vtype='B', lb=0, ub=1)

    moves_placing_half_segment: Dict[HalfSegment, List[Move]] = dict()
    """Half-segment -> a list of moves placing the half-segment."""
    for move in moves:
        for half_segment in move.placed_half_segments():
            if half_segment not in moves_placing_half_segment:
                moves_placing_half_segment[half_segment] = list()
            moves_placing_half_segment[half_segment].append(move_var[move])

    """Constraints."""

    """ 
    I. Dot variable equals sum of move variables that place the dot.
    
    This implies that
      a) we do not place a dot twice;
      b) dot is either in the initial cross or is placed by a move.
    """
    for dot in dot_space:
        moves_placing_dot = list()
        for move in moves:
            if move.dot == dot:
                moves_placing_dot.append(move_var[move])

        if not moves_placing_dot:
            assert dot in starting_dots
        else:
            model.addCons(dot_var[dot] == pyscipopt.quicksum(moves_placing_dot))

    """
    II. We do not place same half-segment twice.
    """

    moves_with_half_segment = dict()
    for move in moves:
        for half_segment in move.placed_half_segments():
            if half_segment not in moves_with_half_segment:
                moves_with_half_segment[half_segment] = list()
            moves_with_half_segment[half_segment].append(move_var[move])

    for half_segment in moves_with_half_segment:
        model.addCons(pyscipopt.quicksum(moves_with_half_segment[half_segment]) <= dot_var[half_segment.dot])

    """
    III. No loops.
    """

    M = len(dot_space) ** 2
    dot_order = dict()
    for dot in dot_space:
        if dot not in starting_dots:
            dot_order[dot] = model.addVar(f'dot_{dot}', vtype='C', lb=0, ub=M)

    for move in moves:
        model.addCons(dot_order[move.dot] + (1 - move_var[move]) * M >= 1 +
                      pyscipopt.quicksum(dot_order[dot] for dot in move.required_dots() if dot not in starting_dots))

    # Objective
    model.setObjective(pyscipopt.quicksum(dot_var.values()) - 36, sense='maximize')

    # Boundary constraints
    lx = min([dot.x for dot in dot_space])
    ux = max([dot.x for dot in dot_space])
    ly = min([dot.y for dot in dot_space])
    uy = max([dot.y for dot in dot_space])

    model.addCons(pyscipopt.quicksum([dot_var[dot] for dot in dot_space if dot.x == lx]) >= 1)
    model.addCons(pyscipopt.quicksum([dot_var[dot] for dot in dot_space if dot.x == ux]) >= 1)
    model.addCons(pyscipopt.quicksum([dot_var[dot] for dot in dot_space if dot.y == ly]) >= 1)
    model.addCons(pyscipopt.quicksum([dot_var[dot] for dot in dot_space if dot.y == uy]) >= 1)

    return Problem(dot=dot_var, move=move_var, order=dot_order, model=model, starting_dots=starting_dots,
                   stats=Statistics())


def create_dependency_graph(moves: List[Move]) -> networkx.DiGraph:
    graph = networkx.DiGraph()

    places_dot = dict()
    for move in moves:
        if move.dot not in places_dot:
            places_dot[move.dot] = list()
        places_dot[move.dot].append(move)

    graph.add_nodes_from(moves)

    for move in moves:
        for dot in move.required_dots():
            if dot in places_dot:
                for pre_move in places_dot[dot]:
                    graph.add_edge(pre_move, move)

    print('Move graph with', len(graph.nodes), 'vertices.')

    return graph


def find_all_3_cycles(graph: networkx.DiGraph) -> List:
    cycles = list()
    for a in graph:
        for b in graph.neighbors(a):
            for c in graph.neighbors(b):
                if c == a:
                    continue
                if graph.has_edge(c, a):
                    cycles.append([a, b, c])

    return cycles


def find_one_3_cycle(graph: networkx.DiGraph) -> Optional[List]:
    for a in graph:
        for b in graph.neighbors(a):
            for c in graph.neighbors(b):
                if c == a:
                    continue
                if graph.has_edge(c, a):
                    return [a, b, c]
    return None


def main():
    def print_param(param, value):
        """Pretty print program parameter."""
        print('{0:>24}: {1}'.format(param, colored(value, attrs=['bold'])))

    print(colored('Morpion 5D Solver', 'yellow', attrs=['bold']))

    parser = argparse.ArgumentParser()
    parser.add_argument('--shape', nargs=4, default=[0, 0, 0, 0], type=int)
    parser.add_argument('--cutoff', type=int, default=83)
    args = parser.parse_args()

    solution_directory = 'solution_{}_{}_{}_{}/'.format(*args.shape)
    log_file = solution_directory + 'log.txt'

    print()
    print_param('Board shape', args.shape)
    print_param('Solution directory', solution_directory)
    print_param('Log file', log_file)
    print()

    try:
        os.makedirs(solution_directory)
    except OSError:
        print('{}: directory {} exists, existing files will be overwritten.\n'.
              format(colored('Info', 'yellow'), colored(solution_directory, attrs=['bold'])))

    model = pyscipopt.Model(f'Morpion 5D on {args.shape} board.')

    starting_dots, possible_moves = get_starting_position(args.shape)

    problem = create_5d_plus_constraints(starting_dots, possible_moves, model)
    problem.shape = args.shape

    # Objective cutoff
    model.addCons(pyscipopt.quicksum(problem.dot.values()) - 36 >= args.cutoff)

    """
    Constraint handler.
    """

    problem.graph = create_dependency_graph(possible_moves)
    constraint_handler = LoopChecker(problem=problem)

    model.includeConshdlr(constraint_handler, "Loop Checker", "Loop Checker",
                          sepapriority=0, enfopriority=-1, chckpriority=-1, sepafreq=-1,
                          propfreq=1, eagerfreq=-1, maxprerounds=-1, delaysepa=False,
                          delayprop=False, needscons=False, presoltiming=pyscipopt.SCIP_PRESOLTIMING.EXHAUSTIVE,
                          proptiming=pyscipopt.SCIP_PROPTIMING.BEFORELP)

    eventhdlr = EventPrinter()
    model.includeEventhdlr(eventhdlr, "EventPrinter", "prints events")

    model.setPresolve(pyscipopt.SCIP_PARAMSETTING.FAST)
    model.setHeuristics(pyscipopt.SCIP_PARAMSETTING.OFF)
    model.setSeparating(pyscipopt.SCIP_PARAMSETTING.AGGRESSIVE)

    model.optimize()


if __name__ == '__main__':
    main()
