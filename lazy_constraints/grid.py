"""
Python implementation of Morpion Solitaire.
"""

from __future__ import annotations
from dataclasses import dataclass
from typing import List, Tuple

from lark import Lark, Transformer
import numpy
import cairo
import PIL.Image as Image


@dataclass(frozen=True)
class Dot:
    """A dot on the grid."""
    x: int
    y: int

    def __add__(self, other: Dot):
        """Vector addition for dots."""
        return Dot(self.x + other.x, self.y + other.y)

    def __mul__(self, a):
        """Scalar multiplication."""
        return Dot(self.x * a, self.y * a)

    def __repr__(self):
        return "(" + str(self.x) + "," + str(self.y) + ")"


@dataclass(frozen=True)
class Segment:
    """A segment on the grid."""
    dot: Dot
    dir: int

    def __repr__(self) -> str:
        """ASCII-art representation of the segment."""

        direction_symbol = ['-', '\\', '|', '/']
        return repr(self.dot) + " " + direction_symbol[self.dir]

    def end(self) -> Dot:
        """A dot on the other end of the segment."""

        directions: List[Dot] = [Dot(1, 0), Dot(1, 1), Dot(0, 1), Dot(-1, 1)]
        return self.dot + directions[self.dir]


@dataclass(frozen=True)
class HalfSegment:
    """A half-segment on the grid."""
    dot: Dot
    dir: int


@dataclass(frozen=True)
class Move:
    """The dot and a list of segments placed by a move."""
    dot: Dot
    segs: Tuple[Segment, ...]

    def __repr__(self):
        return repr(self.dot) + '_' + repr(self.segs[0].dot) + '_' + str(self.segs[0].dir)

    @property
    def direction(self) -> int:
        """Direction of the move."""
        return self.segs[0].dir

    @property
    def starting_dot(self) -> Dot:
        """The starting dot."""
        return self.segs[0].dot

    def required_dots(self) -> List[Dot]:
        """A list of dots required to play the move."""

        dots = [segment.dot for segment in self.segs if segment.dot != self.dot]
        if self.segs[-1].end() != self.dot:
            dots.append(self.segs[-1].end())
        return dots

    def added_potential(self) -> List[HalfSegment]:
        """Half-segments added to potential by this move."""

        return [HalfSegment(self.dot, direction) for direction in range(8)]

    def removed_potential(self) -> List[HalfSegment]:
        """Half-segments removed from potential by this move."""

        return self.placed_half_segments()

    def placed_half_segments(self) -> List[HalfSegment]:
        """A list of half-segments placed by the move."""

        dots = [segment.dot for segment in self.segs]
        dots.append(self.segs[-1].end())

        direction = self.segs[0].dir

        half_segments = list()
        for dot in dots:
            half_segments.append(HalfSegment(dot, direction))
            half_segments.append(HalfSegment(dot, direction + 4))

        return half_segments


@dataclass
class BoundingBox:
    """A bounding box dimensions."""
    upper_left: Dot
    lower_right: Dot
    rows: int
    cols: int

    def dots(self) -> List[Dot]:
        dots_ = list()
        for col in range(self.upper_left.x, self.lower_right.x + 1):
            for row in range(self.upper_left.y, self.lower_right.y + 1):
                dots_.append(Dot(col, row))

        return dots_


class Grid:
    """Dots and segments placed on the grid along with a list of legal moves,
    i.e. position of a game.
    """

    def __init__(self):
        self.dots = dict()
        self.segs = []
        self.movs = []

    def get_moves(self, box: BoundingBox):
        """A list of all potential moves in a given bounding box."""

        directions: List[Dot] = [Dot(1, 0), Dot(1, 1), Dot(0, 1), Dot(-1, 1)]
        dots = box.dots()
        moves = list()

        for dot in dots:
            for direction in range(4):
                end = dot + directions[direction] * 4
                if end in dots:
                    segments = tuple(Segment(dot + directions[direction] * i, direction) for i in range(4))
                    for i in range(5):
                        placed = dot + directions[direction] * i
                        if placed not in self.dots:
                            moves.append(Move(placed, segments))

        return moves

    def bounding_box(self):
        """Calculate the bounding box for the grid."""

        upper_left = Dot(min(self.dots, key=lambda d: d.x).x, min(self.dots, key=lambda d: d.y).y)
        lower_right = Dot(max(self.dots, key=lambda d: d.x).x, max(self.dots, key=lambda d: d.y).y)
        rows = lower_right.y - upper_left.y + 2
        cols = lower_right.x - upper_left.x + 2

        return BoundingBox(upper_left, lower_right, rows, cols)

    def draw(self, cairo_context, box=None):
        """Render the grid on a cairo context."""

        def scale_to_context(dot_):
            """Scales integer grid coordinates to cairo context coordinates."""
            return (dot_.x - box.upper_left.x + 1) * cell_size, \
                   (dot_.y - box.upper_left.y + 1) * cell_size

        def draw_label(dot_, label):
            """Draws label on a dot."""
            cairo_context.set_source_rgb(1.0, 1.0, 1.0)
            cairo_context.select_font_face("monospace",
                                           cairo.FONT_SLANT_NORMAL,  # pylint: disable=no-member
                                           cairo.FONT_WEIGHT_BOLD)  # pylint: disable=no-member
            cairo_context.set_font_size(cell_size / 4.5)
            (_, _, width, height, _, _) = cairo_context.text_extents(label)
            (center_x, center_y) = scale_to_context(dot_)
            cairo_context.move_to(center_x - width / 2, center_y + height / 2)
            cairo_context.show_text(label)

        def draw_dot(dot_):
            """Draw a dot."""
            cairo_context.move_to(*scale_to_context(dot_))
            cairo_context.arc(*scale_to_context(dot_), cell_size / 4, 0, 2 * 3.142)
            cairo_context.set_line_width(line_width * 2.5)
            cairo_context.set_source_rgb(0, 0, 0)
            cairo_context.stroke_preserve()
            if self.dots[dot_] == '0':
                cairo_context.set_source_rgb(0.7, 0.7, 0.7)
            else:
                cairo_context.set_source_rgb(0, 0, 0)
            cairo_context.fill()

            # Dot label
            if self.dots[dot_] != '0':
                draw_label(dot_, self.dots[dot_])

        def draw_segment(seg_):
            """Draw a segment."""
            directions = [Dot(1, 0), Dot(1, 1), Dot(0, 1), Dot(-1, 1)]

            cairo_context.set_source_rgb(0, 0, 0)
            cairo_context.set_line_width(line_width)
            cairo_context.move_to(*scale_to_context(seg_.dot))
            cairo_context.line_to(*scale_to_context(seg_.dot + directions[seg_.dir]))
            cairo_context.stroke()

        if box is None:
            box = self.bounding_box()

        cell_size = min(1.0 / box.rows, 1.0 / box.cols)
        line_width, _ = cairo_context.device_to_user(1.0, 0.0)

        # Draw grid
        for grid_x in range(box.cols + 1):
            cairo_context.set_source_rgb(0.8, 0.8, 0.8)
            cairo_context.set_line_width(line_width)
            cairo_context.move_to(grid_x * cell_size, 0.0)
            cairo_context.line_to(grid_x * cell_size, box.rows * cell_size)
            cairo_context.stroke()

        for grid_y in range(box.rows + 1):
            cairo_context.set_source_rgb(0.8, 0.8, 0.8)
            cairo_context.set_line_width(line_width)
            cairo_context.move_to(0.0, grid_y * cell_size)
            cairo_context.line_to(box.cols * cell_size, grid_y * cell_size)
            cairo_context.stroke()

        # Draw segments
        for seg in self.segs:
            draw_segment(seg)

        # Draw dots
        for dot in self.dots:
            draw_dot(dot)

    def get_PILImage(self, width=400, height=400, box=None):
        """Render the grid as a PIL.Image."""

        image_surface = cairo.ImageSurface(cairo.Format.ARGB32, width, height)
        cairo_context = cairo.Context(image_surface)
        cairo_context.scale(image_surface.get_width(), image_surface.get_height())
        self.draw(cairo_context,  box)
        pil_image = Image.frombuffer("RGBA", (image_surface.get_width(),
                                              image_surface.get_height()),
                                     image_surface.get_data(), "raw", "RGBA", 0, 1)

        return pil_image


class PentasolParser:  # pylint: disable=too-few-public-methods
    """Parser for pentasol file format."""

    class PentasolTransformer(Transformer):
        """Transforms pentasol grammar tree into a pair (dot, [ move list ])."""

        @staticmethod
        def psol(matches):
            """Transform tree node for production `psol: dot sequence`
            into pair (dot, sequence)."""
            return matches

        @staticmethod
        def move(matches):
            """Transform tree node for production `move: dot direction pos`
            into Move tuple."""

            # Third direction in pentasol format has opposite direction that the one we use.
            if matches[1] == 3:
                offset = -matches[2] - 2
            else:
                offset = matches[2] - 2

            direction = [Dot(1, 0), Dot(1, 1), Dot(0, 1), Dot(-1, 1)][matches[1]]
            placed = matches[0]
            start = placed + direction * offset

            segs = [Segment(start + direction * i, matches[1]) for i in range(4)]

            return Move(placed, segs)

        @staticmethod
        def dot(matches):
            """Transform tree node for production `dot: "(" coord "," coord ")"`
            into
            """
            return Dot(matches[0], matches[1])

        @staticmethod
        def coord(matches):
            """Transform tree node for production `coord: SIGNED_NUMBER`
            into an int.
            """
            return int(matches[0])

        @staticmethod
        def pos(matches):
            """Transform tree node for production `pos: SIGNED_NUMBER`
            into an int.
            """
            return int(matches[0])

        @staticmethod
        def dir_s(_):
            """Transform tree node for constant `"|" -> s` into 2."""
            return 2

        @staticmethod
        def dir_sw(_):
            """Transform tree node for constant `"/" -> sw` into 3."""
            return 3

        @staticmethod
        def dir_se(_):
            """Transform tree node for constant `"\\\\" -> se` into 1."""
            return 1

        @staticmethod
        def dir_e(_):
            """Transform tree node for constant `"-" -> e` into 0."""
            return 0

        @staticmethod
        def sequence(matches):
            """Transform tree node for production `sequence: move+`
            into a list.
            """
            return matches

    def __init__(self):
        psol_grammar = """
                dot: "(" coord "," coord ")"
                coord: SIGNED_NUMBER
                direction: "|" -> dir_s | "/" -> dir_sw | "\\\\" -> dir_se | "-" -> dir_e
                pos: SIGNED_NUMBER
                move: dot direction pos
                sequence: move+
                psol: dot sequence

                %import common.SIGNED_NUMBER
                %import common.WS
                %ignore WS
        """

        self.parser = Lark(psol_grammar, start='psol')

    def parse(self, filename):
        """Parse a Morpion file written in the pentasol format."""

        with open(filename, 'r') as psol_file:
            data = psol_file.read()

        tree = self.parser.parse(data)

        return PentasolParser.PentasolTransformer().transform(tree)


class Game:
    """A morpion game."""

    variants = ['5d', '5t']

    @dataclass
    class BBox:
        """A bounding box, as defined in `An upper bound of 84 for Morpion Solitaire 5D` paper."""
        n: int
        e: int
        s: int
        w: int

    def __init__(self, variant='5t', board_size=BBox(15, 15, 15, 15), reference=None):
        self.history = []
        self.variants = ['5d', '5t']
        self.variant = variant

        assert self.variant in self.variants

        self.board_size = board_size

        self.width = board_size.e + board_size.w + 10  # in dots
        self.height = board_size.n + board_size.s + 10  # in dots

        self.dot_n = self.width * self.height
        self.move_n_max = self.dot_n * 4

        # directions: e, se, s, sw
        self.dir = [1, 1 + self.width, self.width, self.width - 1]
        if reference is not None:
            self.reference = reference
        else:
            self.reference = self.pos_from_coords(board_size.w + 3, board_size.n + 3)

        if self.variant == '5d':
            self.exclude_len = 4
        else:
            self.exclude_len = 3

        self.reset()

    def __repr__(self):
        """ASCII Art rendering of the final position."""

        return "TODO(amn)"

    def dot_from_pos(self, pos):
        """Convert pos to Dot."""
        return Dot(*self.coords_from_pos(pos))

    def get_grid(self, last_ply=-1):
        """Return the grid corresponding to position at a given ply."""

        if last_ply < 0:
            last_ply = len(self.history) + last_ply + 1

        grid = Grid()

        # Create cross on the grid.
        for dot in self.cross:
            grid.dots[dot] = "0"

        # Replay history moves
        directions: List[Dot] = [Dot(1, 0), Dot(1, 1), Dot(0, 1), Dot(-1, 1)]

        ply = 1

        for move in self.history:
            if ply > last_ply:
                break

            starting = self.dot_from_pos(self.move_pos(move))
            direction = self.move_dir(move)

            for i in range(5):
                # noinspection PyTypeChecker
                dot_in_move = starting + directions[direction] * i

                if dot_in_move not in grid.dots:
                    # we found dot placed by the move
                    grid.dots[dot_in_move] = str(ply)

                if i < 4:
                    grid.segs.append(Segment(dot_in_move, direction))

            ply = ply + 1

        return grid

    def load(self, file):
        """Load game state from a pentasol file."""
        parser = PentasolParser()

        reference, move_list = parser.parse(file)

        self.get_grid().get_PILImage(800, 800).save('initial.png')

        self.reference = self.pos_from_coords(reference.x, reference.y)
        self.history = []

        # Replay game
        self.reset()

        for move in move_list:
            self.make_move(self.move(self.pos_from_coords(move.segs[0].dot.x,
                                                          move.segs[0].dot.y), move.segs[0].dir))

    def save(self, filename):
        """Save game state to a pentasol file."""

        with open(filename, 'w') as psol_file:
            psol_file.write(str(self.dot_from_pos(self.reference)) + '\n')

            replay = Game(reference=self.reference)

            direction_symbols = ['-', '\\', '|', '/']
            for move in self.history:
                middle = replay.dot_from_pos(self.move_mid(move))
                direction = replay.move_dir(move)
                placed = replay.dot_from_pos(replay.move_dot(move))
                replay.make_move(move)

                if direction == 2:
                    offset = middle.y - placed.y
                else:
                    offset = middle.x - placed.x

                psol_file.write(str(placed) + ' ' + direction_symbols[direction] + ' '
                                + str(offset) + '\n')

    def pos_from_coords(self, coord_x, coord_y):
        """Covert coordinate pair into pos."""
        return coord_x + coord_y * self.width

    def coords_from_pos(self, pos):
        """Convert pos into coordinate pair."""
        return pos % self.width, pos // self.width

    def can_move(self, move):
        """Check if the move is legal."""
        return self.dot_count[move] == 4

    def _inc_dot_count(self, move, count):
        if self.can_move(move):
            # remove m from legal move list
            self.legal_moves.discard(move)
        self.dot_count[move] += count
        if self.can_move(move):
            # put m on legal move list
            self.legal_moves.add(move)

    def put_dot(self, pos, count):
        """Place dot on the grid."""
        self.has_dot[pos] = (count > 0)

        for direction in range(4):
            for i in range(5):
                self._inc_dot_count((pos - self.dir[direction] * i) * 4 + direction, count)

    def make_move(self, move):
        """Make a move."""
        assert move in self.legal_moves

        # invalidate moves along m.dir
        for i in range(1 + 2 * self.exclude_len):
            self._inc_dot_count(move + (i - self.exclude_len) * self.dir[self.move_dir(move)] * 4, 5)

        # put dot
        self.put_dot(self.move_dot(move), 1)

        # record move
        self.history.append(move)

    @staticmethod
    def move_pos(move):
        """Return pos of a starting dot of a move."""
        return move // 4

    def move_dot(self, move):
        """Returns dot placed by a move; valid only when move is legal."""
        for i in range(5):
            if self.has_dot[self.move_pos(move) + self.dir[self.move_dir(move)] * i] == 0:
                return self.move_pos(move) + self.dir[self.move_dir(move)] * i
        return None

    @staticmethod
    def move_dir(move):
        """Return direction of a move."""
        return move % 4

    def move_mid(self, move):
        """Return mid pos of a move."""

        return move // 4 + 2 * self.dir[self.move_dir(move)]

    @staticmethod
    def move(pos, direction):
        """Create a move with starting dot pos in a given direction."""
        return pos * 4 + direction

    def reset(self):
        """Reset game state to the starting position."""
        self.history = []
        self.has_dot = numpy.zeros(self.dot_n, dtype=int)
        self.dot_count = numpy.zeros(self.move_n_max, dtype=int)
        self.legal_moves = set()
        self.cross = []
        for step in [[0, 0], [-1, 0], [-2, 0], [-3, 0], [-3, 1], [-3, 2], [-3, 3], [-2, 3], [-1, 3],
                     [0, 3], [0, 4], [0, 5], [0, 6], [1, 6], [2, 6], [3, 6], [3, 5], [3, 4], [3, 3],
                     [4, 3], [5, 3], [6, 3], [6, 2], [6, 1], [6, 0], [5, 0], [4, 0], [3, 0],
                     [3, -1], [3, -2], [3, -3], [2, -3], [1, -3], [0, -3], [0, -2], [0, -1]]:
            self.put_dot(self.reference + self.pos_from_coords(*step), 1)
            self.cross.append(self.dot_from_pos(self.reference) + Dot(*step))

        self.played_moves = set()

    def max_goedel_number(self):
        """Upper bound on Goedel numbers of moves."""
        return self.width * self.height * 4


def main():
    game = Game(variant='5d')
    game.load('82.psol')
    game.get_grid().get_PILImage(800, 600).save('82.png')


if __name__ == '__main__':
    main()
