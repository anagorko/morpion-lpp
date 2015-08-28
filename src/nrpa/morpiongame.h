#ifndef __MORPION_H__
#define __MORPION_H__
#include <vector>
#include <string>
#include <iostream>
#include <string.h>

class MorpionGame
{
	int octagon[8] = { 22, 24, 30, 48, 34, 28, 26, 40 };

public:
	static const int T5 = 0;
	static const int D5 = 1;

    static const int variant = D5;

    typedef int Direction;
    typedef int Position;

    Position PositionOfCoords(int x, int y);
    void CoordsOfPosition(Position p, int & x, int & y);

    struct Move
    {
        Position pos;
        Direction dir;
        Move() {}
        Move(Position pos, Direction dir) : pos(pos), dir(dir) {}
    };

    struct HistoryMove {
        Move move;
        Position dot;
    };

    struct NotImplementedError{};

    MorpionGame();
    std::vector<Move> Moves() const;
    void MakeMove(const Move& move);
    void UndoMove();

    void Init(const std::vector<HistoryMove> & history);

    std::vector<HistoryMove> GetResults();

    std::vector<HistoryMove> LoadMovesFile(const std::string & filename);
    void SaveMovesFile(const std::vector<HistoryMove> & history, const std::string & filename);

	MorpionGame(const MorpionGame& g)
	{
		memcpy(has_dot, g.has_dot, sizeof(has_dot));
		memcpy(dots_count, g.dots_count, sizeof(dots_count));
		memcpy(move_index, g.move_index, sizeof(move_index));
		legal_moves = g.legal_moves;
		history = g.history;
	}

protected:
    /*  o-
     * /|\ */
    
    enum { RIGHT = 0, DOWN = 2, LEFT = 4, UP = 6 };
    typedef HistoryMove Undo;
    
    static const int DIRS = 4;
    static const int SIZE = 24;
    static const int ARRAY_SIZE = SIZE * SIZE;
    static const int LINE = 5; // in number of dots
    static const int dir[DIRS];

    bool has_dot[ARRAY_SIZE];
    int dots_count[ARRAY_SIZE][DIRS];
    int move_index[ARRAY_SIZE][DIRS];
    std::vector<Move> legal_moves;
    std::vector<Undo> history;
    
    bool CanMove(Position pos, Direction d) const
    {
        return dots_count[pos][d] == LINE - 1;
    }
    
    void IncDotCount(Position pos, Direction d, int count);
    void PutDot(Position pos, int count);

    int ShiftFromDir(int d) { return d < DIRS ? dir[d] : -dir[d - DIRS]; }

    Position ReferencePoint();

    HistoryMove TryParseHistoryMove(const std::string & line, int reference_delta);
    int TryParseReferencePoint(const std::string & line);

    int CharDirToIntDir(char c);
    char IntDirToCharDir(int dir);

	// Directions:
	//   N NE E SE S SW W NW
    //   0 1  2 3  4 5  6 7

	const int nx[8] = { 0, 2, 2, 2,  0,  -2, -2, -2 };
	const int ny[8] = { 2, 2, 0, -2, -2, -2, 0,  2  };

    int DistanceFromOrigin(Position p, int dir)
    {
		int px, py; 
		CoordsOfPosition(p, px, py);

		int rx, ry;
		CoordsOfPosition(ReferencePoint(), rx, ry);

		return (2*(px - rx) - 3) * nx[dir] + (2*(py - ry) - 3) * ny[dir];
    }

	bool InsideBoard(Position p)
	{
		for (int dir = 0; dir < 8; dir++) {
			if (DistanceFromOrigin(p, dir) > octagon[dir]) return false;
		}
		return true;
	}

	bool LineInsideBoard(Position p, Direction d)
	{
 		return InsideBoard(p) && InsideBoard(p + dir[d] * (LINE - 1));
	}

public:
    static const int max_goedel_number = DIRS * ARRAY_SIZE;
    static inline int goedel_number(const Move &m)
    {
        return m.dir * ARRAY_SIZE + m.pos;
    }

	void print()
	{
		for (int y = 0; y < SIZE; y++) {
			for (int x = 0; x < SIZE; x++) {
				if (PositionOfCoords(x,y) == ReferencePoint()) {
					std::cout << "R";
				} else if (has_dot[PositionOfCoords(x,y)]) {
					std::cout << "*";
				} else if (InsideBoard(PositionOfCoords(x,y))) {
					std::cout << ".";
				} else {
					std::cout << " ";
				}
			}
			std::cout << std::endl;
		}
	}

};

inline std::vector<MorpionGame::Move> MorpionGame::Moves() const
{
    return legal_moves;
}

inline MorpionGame::Position MorpionGame::PositionOfCoords(int x, int y)
{
    return x + y * SIZE;
}

inline void MorpionGame::CoordsOfPosition(Position p, int & x, int & y)
{
    x = p % SIZE;
    y = p / SIZE;
}

#endif

