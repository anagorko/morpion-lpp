#ifndef __MORPION_H__
#define __MORPION_H__
#include <vector>
#include <string>

const int T5 = 0;
const int D5 = 1;

class MorpionGame
{
public:
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

protected:
    /*  o-
     * /|\ */
    
    enum { RIGHT = 0, DOWN = 2, LEFT = 4, UP = 6 };
    typedef HistoryMove Undo;
    
    static const int DIRS = 4;
    static const int SIZE = 64;
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

public:
    static const int max_goedel_number = DIRS * ARRAY_SIZE;
    static inline int goedel_number(const Move &m)
    {
        return m.dir * ARRAY_SIZE + m.pos;
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

