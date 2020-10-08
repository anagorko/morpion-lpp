"""
A Morpion 5D position solver.
"""

import argparse
from typing import List, Tuple

import pyscipopt
import networkx

from lazy_constraints.grid import Game, BoundingBox, Dot, Move, HalfSegment


class LoopChecker(pyscipopt.Conshdlr):
    def __init__(self, graph: networkx.DiGraph, move_var, dot_var, *args, **kwargs):
        super().__init__(*args, **kwargs)

        self.graph = graph
        self.move_var = move_var
        self.dot_var = dot_var

    def check_cycle(self, solution=None):
        selected_moves = list()
        for move in self.graph:
            assert self.model.isFeasIntegral(self.model.getSolVal(solution, self.move_var[move]))
            if self.model.isFeasEQ(self.model.getSolVal(solution, self.move_var[move]), 1.0):
                selected_moves.append(move)

        # print(f'Check cycles. Proposed solution has {len(selected_moves)} moves.')

        try:
            cycle = networkx.find_cycle(self.graph.subgraph(selected_moves))
            # print('FOUND CYCLE', cycle)
            return cycle
        except networkx.exception.NetworkXNoCycle:
            print('no cycle found', selected_moves)
            return None

    def conscheck(self, constraints, solution, checkintegrality, checklprows, printreason, completely):
        # print('---> conscheck')

        cycle = self.check_cycle(solution)

        if cycle is not None:
            return {'result': pyscipopt.SCIP_RESULT.INFEASIBLE}
        else:
            return {'result': pyscipopt.SCIP_RESULT.FEASIBLE}

    def conslock(self, constraint, locktype, nlockspos, nlocksneg):
        # print('---> conslock')
        for var in self.move_var:
            self.model.addVarLocks(self.move_var[var], nlocksneg, nlockspos)
        for var in self.dot_var:
            self.model.addVarLocks(self.dot_var[var], nlocksneg, nlockspos)

    def consenfolp(self, constraints, nusefulconss, solinfeasible):
        # print('---> consENFOLP')

        cycle = self.check_cycle(None)

        if cycle is not None:
            cycle_vars = [self.move_var[edge[0]] for edge in cycle]
            self.model.addCons(pyscipopt.quicksum(cycle_vars) <= len(cycle) - 1)

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


def get_grid(shape) -> Tuple[List[Dot], List[Move]]:
    game = Game(variant='5d', board_size=Game.BBox(*shape))
    grid = game.get_grid()

    box = get_bounding_box(game.dot_from_pos(game.reference), shape)
    moves = grid.get_moves(box)

    grid.get_PILImage(box=box).save('solver.png')

    return grid.dots.keys(), moves


def create_model_potential_method(dots: List[Dot], moves: List[Move], label: str) -> pyscipopt.Model:
    model = pyscipopt.Model(label)

    move_var = dict()
    for move in moves:
        move_var[move] = model.addVar(f'mv_{move}', vtype='B', lb=0, ub=1)

    # Lists of moves that remove a half-segment from potential
    removes = dict()
    for move in moves:
        for half_segment in move.removed_potential():
            if half_segment not in removes:
                removes[half_segment] = list()
            removes[half_segment].append(move_var[move])

    for s in [str(hs) + ' ' + str(removes[hs]) for hs in removes if hs.dot.x == 0]:
        print(s)

    # Lists of moves that add a half-segment to potential
    adds = dict()
    for move in moves:
        for half_segment in move.added_potential():
            if half_segment not in adds:
                adds[half_segment] = list()
            adds[half_segment].append(move_var[move])

    # 1) For half-segment that is in the starting potential
    #   a) sum of weights of moves that add this half-segment is <= 0
    #   b) sum of weights of moves that remove this half-segment is <= 1
    #
    # We don't generate moves that satisfy a) so only constraints from b) are added.

    print(dots)
    for dot in dots:
        for direction in range(8):
            half_segment = HalfSegment(dot, direction)
            print(half_segment)
            model.addCons(pyscipopt.quicksum(removes[half_segment]) <= 1)

    # 2) For half-segment that is not in the starting potential
    #   a) sum of weights of moves that add this half-segment is <= 1
    #   b) sum of weights of moves that remove this half_segment is <=
    #            sum of weights of moves that add this half_segment

    space = set()
    for move in moves:
        for dot in move.required_dots():
            space.add(dot)
        space.add(move.dot)

    for dot in space:
        if dot in dots:
            continue

        for direction in range(8):
            half_segment = HalfSegment(dot, direction)

            model.addCons(pyscipopt.quicksum(adds[half_segment]) <= 1)
            if half_segment in removes:
                model.addCons(pyscipopt.quicksum(removes[half_segment]) <=
                              pyscipopt.quicksum(adds[half_segment]))

    # Objective
    model.setObjective(pyscipopt.quicksum(move_var.values()), sense='maximize')

    return model


def create_model(dots: List[Dot], moves: List[Move], label: str) -> pyscipopt.Model:
    model = pyscipopt.Model(label)

    space = set()
    for move in moves:
        for dot in move.required_dots():
            space.add(dot)
        space.add(move.dot)

    lx = min([dot.x for dot in space])
    ux = max([dot.x for dot in space])
    ly = min([dot.y for dot in space])
    uy = max([dot.y for dot in space])

    dot_var = dict()

    for dot in space:
        if dot in dots:
            dot_var[dot] = model.addVar(f'dot_{dot}', vtype='B', lb=1, ub=1)
        else:
            dot_var[dot] = model.addVar(f'dot_{dot}', vtype='B', lb=0, ub=1)

    move_var = dict()

    for move in moves:
        move_var[move] = model.addVar(f'mv_{move}', vtype='B', lb=0, ub=1)

    print(f'We have {len(dot_var)} dots and {len(move_var)} moves on the board.')

    # We don't place a dot twice
    # No move -> no dot
    for dot in space:
        if dot in dots:
            continue

        moves_with_dot = list()
        for move in moves:
            if move.dot == dot:
                moves_with_dot.append(move_var[move])

        model.addCons(dot_var[dot] == pyscipopt.quicksum(moves_with_dot))

    # We don't place a half-segment twice
    moves_with_half_segment = dict()
    for move in moves:
        for half_segment in move.placed_half_segments():
            if half_segment not in moves_with_half_segment:
                moves_with_half_segment[half_segment] = list()
            moves_with_half_segment[half_segment].append(move_var[move])

    for half_segment in moves_with_half_segment:
        model.addCons(pyscipopt.quicksum(moves_with_half_segment[half_segment]) <= dot_var[half_segment.dot])

    # No dot -> no move
    for dot in space:
        if dot in dots:
            continue

        moves_requiring_dot = list()
        for move in moves:
            if dot in move.required_dots():
                moves_requiring_dot.append(move_var[move])
        model.addCons(pyscipopt.quicksum(moves_requiring_dot) <= 4 * dot_var[dot])

    # boundary constraints
    model.addCons(pyscipopt.quicksum([dot_var[dot] for dot in space if dot.x == lx]) >= 1)
    model.addCons(pyscipopt.quicksum([dot_var[dot] for dot in space if dot.x == ux]) >= 1)
    model.addCons(pyscipopt.quicksum([dot_var[dot] for dot in space if dot.y == ly]) >= 1)
    model.addCons(pyscipopt.quicksum([dot_var[dot] for dot in space if dot.y == uy]) >= 1)

    # objective cutoff

    model.addCons(pyscipopt.quicksum(dot_var.values()) - 36 >= 83.0)

    # branch priority

    for dot in space:
        if dot.x == lx or dot.x == ux:
            model.chgVarBranchPriority(dot_var[dot], 1)
        if dot.y == ly or dot.y == uy:
            model.chgVarBranchPriority(dot_var[dot], 1)

    # objective
    model.setObjective(pyscipopt.quicksum(dot_var.values()) - 36, sense='maximize')

    # constraint handler

    graph = create_dependency_graph(moves)
    conshdlr = LoopChecker(graph=graph, move_var=move_var, dot_var=dot_var)

    model.includeConshdlr(conshdlr, "Loop Checker", "Loop Checker",
                          sepapriority=0, enfopriority=-1, chckpriority=-1, sepafreq=-1,
                          propfreq=-1, eagerfreq=-1, maxprerounds=0, delaysepa=False,
                          delayprop=False, needscons=False, presoltiming=pyscipopt.SCIP_PRESOLTIMING.FAST,
                          proptiming=pyscipopt.SCIP_PROPTIMING.AFTERLPNODE)

    return model


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

    return graph


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--shape', nargs=4, default=[0, 0, 0, 0], type=int)
    args = parser.parse_args()

    dots, moves = get_grid(args.shape)

    model = create_model(dots, moves, f'Morpion 5D on {args.shape} board.')
    # model = create_model_potential_method(dots, moves, f'Morpion 5D on {args.shape} board (potential method).')
    model.writeProblem(f'morpion_{"_".join(map(str, args.shape))}.cip')

    model.optimize()
    model.printSol()


if __name__ == '__main__':
    main()
