#include "morpiongame.h"
#include <cstring>
#include <fstream>
#include <boost/regex.hpp>
#include <iostream>
#include <cassert>

using namespace std;

const int MorpionGame::dir[MorpionGame::DIRS] = {1, SIZE + 1, SIZE, SIZE - 1};

MorpionGame::MorpionGame()
{
    memset(has_dot, 0, sizeof(has_dot));
    memset(dots_count, 0, sizeof(dots_count));
    memset(move_index, 0, sizeof(move_index));
    static const int cross[] = {
        RIGHT, UP, RIGHT, DOWN, RIGHT, DOWN, LEFT, DOWN, LEFT, UP, LEFT, UP
    };
    static const int ARMLEN = LINE - 2;
    Position p =
        PositionOfCoords((SIZE - 3 * ARMLEN) / 2,
                         (SIZE - ARMLEN) / 2);
    for (int i = 0; i < 12; i++)
    {
        int d = ShiftFromDir(cross[i]);
        for (int j = 0; j < ARMLEN; j++)
        {
            p += d;
            PutDot(p, 1);
        }
    }

	// Invalidate moves that are outside of the octagonal board
	for (Position p = 0; p < ARRAY_SIZE; p++) {
		for (Direction d = 0; d < DIRS; d++) {
			if (!LineInsideBoard(p,d)) {
				IncDotCount(p,d,LINE);
			}
		}
	}
}

void MorpionGame::IncDotCount(Position pos, Direction d, int count)
{
    if (CanMove(pos, d))
    {
        int idx = move_index[pos][d];
        Move& back = legal_moves.back();
        move_index[back.pos][back.dir] = idx;
        legal_moves[idx] = back;
        legal_moves.pop_back();
    }
    dots_count[pos][d] += count;
    if (CanMove(pos, d))
    {
        move_index[pos][d] = legal_moves.size();
        legal_moves.push_back(Move(pos, d));
    }
}

void MorpionGame::PutDot(Position pos, int count)
{
    has_dot[pos] = count > 0;
    for (Direction d = 0; d < DIRS; d++)
    {
        Position p = pos;
        for (int i = 0; i < LINE; i++)
        {
            IncDotCount(p, d, count);
            p -= dir[d];
        }
    }
}

void MorpionGame::MakeMove(const Move& move)
{
    Undo undo;
    undo.move = move;
    /* Block moves overlaping with segments added by the move */
    for (int i = -(LINE - 2 + variant); i <= LINE - 2 + variant; i++)
        IncDotCount(move.pos + dir[move.dir] * i, move.dir, LINE);
    /* Find dot and put it */
    for (int i = 0; i < LINE; i++)
    {
        Position p = move.pos + dir[move.dir] * i;
        if (!has_dot[p])
        {
            PutDot(p, 1);
            undo.dot = p;
        }
    }
    history.push_back(undo);
}

void MorpionGame::UndoMove()
{
    Undo undo = history.back();
    history.pop_back();
    Move move = undo.move;
    /* Remove dot */
    PutDot(undo.dot, -1);
    /* Unblock moves overlaping with segments added by the move */
    for (int i = -(LINE - 2 + variant); i <= LINE - 2 + variant; i++)
        IncDotCount(move.pos + dir[move.dir] * i, move.dir, -LINE);
}

MorpionGame::Position MorpionGame::ReferencePoint()
//     XXXX
//     X  X
//     X  X
//  XXXR  XXXX
//  X        x
//  X        x
//  XXXX  XXXX
//     X  X
//     X  X
//     XXXX
//
//     R - reference point
{
    const int ARMLEN = LINE - 2;
    const int MIDDLE = (SIZE - ARMLEN) / 2;
    return PositionOfCoords(MIDDLE, MIDDLE);
}

void MorpionGame::Init(const vector<MorpionGame::HistoryMove> & history)
{
    for (MorpionGame::HistoryMove hm : history)
    {
        assert(CanMove(hm.move.pos, hm.move.dir));
        MakeMove(hm.move);
    }
}

vector<MorpionGame::HistoryMove> MorpionGame::GetResults()
{
    return history;
}

int MorpionGame::CharDirToIntDir(char c)
{
    switch (c) {
        case '-':
            return 0;
        case '\\':
            return 1;
        case '|':
            return 2;
        case '/':
            return 3;
        default:
            return -1;
    };
}

char MorpionGame::IntDirToCharDir(int dir)
{
    static const char mapping[] = {'-', '\\', '|', '/'};
    return mapping[dir];
}

int MorpionGame::TryParseReferencePoint(const string & line)
{
    boost::regex reference_regex("\\s*\\(\\s*(-?\\d+)\\s*,\\s*(-?\\d+)");
    boost::cmatch reference_match_result;
    if (!boost::regex_search(line.c_str(), reference_match_result, reference_regex))
    {
        cerr << "Line: \"" << line << "\" does not match data pattern" << endl;
	return -1;
    }
    else
    {
        int x = stoi(reference_match_result[1]);
        int y = stoi(reference_match_result[2]);
        return ReferencePoint() - PositionOfCoords(x, y);
    }
}

MorpionGame::HistoryMove MorpionGame::TryParseHistoryMove(const string & line, int reference_delta)
{
    boost::regex data_regex("\\s*\\(\\s*(-?\\d+)\\s*,\\s*(-?\\d+)\\s*\\)\\s+([\\|\\\\/-])\\s+([-\\+]?[012])");
    boost::cmatch data_match_results;
    if (!boost::regex_search(line.c_str(), data_match_results, data_regex))
    {
        cerr << "Line: \"" << line << "\" does not match data pattern" << endl;
	
	throw string("error");
    }
    else
    {
        int x = stoi(data_match_results[1]);
        int y = stoi(data_match_results[2]);
        char c_dir = string(data_match_results[3])[0];
        int dir = CharDirToIntDir(c_dir);
        int rel = stoi(data_match_results[4]);
        int dist = -2 + rel;

        if (c_dir == '/')
        {
            dist = -(4 + dist);
        }

        MorpionGame::HistoryMove history_move;
        history_move.dot = PositionOfCoords(x, y) + reference_delta;

        history_move.move.pos = dist * ShiftFromDir(dir) + history_move.dot;
        history_move.move.dir = dir;
        return history_move;
    } 
}

vector<MorpionGame::HistoryMove> MorpionGame::LoadMovesFile(const string &filename)
{
    ifstream moves_file;
    moves_file.open(filename);
    string line;

    vector<MorpionGame::HistoryMove> result;

    bool reference_set = false;
    int reference_delta;

    while (getline(moves_file, line))
    {
        boost::regex comment_regex("\\s*#"), only_white_regex("^\\s*$");
        if (!boost::regex_search(line, comment_regex) && !boost::regex_search(line, only_white_regex))
        {
            if (!reference_set)
            {
                reference_delta = TryParseReferencePoint(line);
                reference_set = true;
            }
            else 
            {
                result.push_back(TryParseHistoryMove(line, reference_delta));
            }
        } else {
        }
    }
    return result;
}

void MorpionGame::SaveMovesFile(const vector<MorpionGame::HistoryMove> & history, const string & filename) {
    string content;
    auto add_point = [&](Position p) {
        int x, y;
        CoordsOfPosition(p, x, y);
        content += "(" + to_string(x) + "," + to_string(y) + ")";
    };
    add_point(ReferencePoint());
    content += '\n';

    for (MorpionGame::HistoryMove h : history)
    {
        add_point(h.dot);
        content += ' ';
        content += IntDirToCharDir(h.move.dir);
        int rel = 2 - (h.dot - h.move.pos) / ShiftFromDir(h.move.dir);
//        int q = (h.dot - h.move.pos) / ShiftFromDir(h.move.dir);
        if (h.move.dir == 3)
            rel *= -1;
        content += ' ' + string(rel > 0 ? "+" : "") + to_string(rel);
        content += '\n';
    }
    ofstream f;
    f.open(filename);
    f << content;
}


