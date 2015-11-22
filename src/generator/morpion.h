#ifndef __MORPION__
#define __MORPION__

#include<string>
#include<iostream>
#include<sstream>
#include<map>
#include<boost/program_options.hpp>

namespace po = boost::program_options;

/**
 * Variant names the Morpion variant. Possible values are T5, D5, T4 and D4.
 */
 
enum Variant { T5 = 0, D5, T4, D4 };
const int line_length[4] = { 4, 4, 3, 3 };    // indexed by variants

const int best_upper_bound[4] = { 485, 85, 62, 35 }; /// Upper bounds for Morpion variants.

std::string to_string(const Variant &v);
std::ostream& operator<<(std::ostream& os, Variant v); 

/**
 * Dot represents a point on the Z^2 grid.
 *
 * Supported operators are +, * (scalar multiplication), !=, ==.
 */
 
class Dot {
public:
    int x, y;

    Dot(int _x = 0, int _y = 0) : x(_x), y(_y) {}

    bool operator==(Dot &w) {
        return x == w.x && y == w.y;
    }
       
    /// Lexicographical ordering so we can put Dots into STL containers that require order.
    bool operator<(const Dot& d) const
    {
        return x < d.x || (x == d.x && y < d.y);
    }
    
    int operator*(const Dot &d) {
        return x * d.x + y * d.y;
    }
    
    Dot operator+(const Dot &d) {
        return Dot(x + d.x, y + d.y);
    }

    Dot operator-(const Dot &d) {
        return Dot(x - d.x, y - d.y);
    }
    
    Dot operator*(int m) {
        return Dot(x*m, y*m);
    }
};

Dot operator+(const Dot &a, const Dot &b);
Dot operator*(const Dot &v, int a) ;
bool operator!=(const Dot &v1, const Dot &v2);
bool operator==(const Dot &v1, const Dot &v2);

std::string to_string(const Dot &v);
std::ostream& operator<<(std::ostream& os, Dot v);
void validate(boost::any& v, std::vector<std::string> const& values, Dot*, int);

const Dot direction[] = {
  Dot(1,0), Dot(1,1), Dot(0,1), Dot(-1,1), 
  Dot(-1,0), Dot(-1,-1), Dot(0,-1), Dot(1,-1)
}; /// direction table converts numbers 0..7 into direction std::vectors

/**
 * Segment represents a segment (length one) on the board. Every segment is directed.
 *
 */

class Segment {
public:
    Dot first;
    int second;

    Segment() { }
    
    Segment(Dot v, int d) : first(v), second(d) { }

    /// Lexicographical ordering so we can put Segments into STL containers that require order.
    bool operator<(const Segment& s) const
    {
        return first.x < s.first.x ||
               (first.x == s.first.x && first.y < s.first.y) ||
               (first.x == s.first.x && first.y == s.first.y && second < s.second);               
    }
    
    /// Segment with same endpoints, facing in the other direction.
    Segment opposite() const
    {
        return Segment(first + direction[second], (second + 4) % 8);
    }
};
std::string to_string(const Segment &);
std::ostream& operator<<(std::ostream&, const Segment&);

/**
 * Line represents line on a board (the placed dot is not marked).
 */
 
class Line {
public:
    Dot st;
    int dir;
    
    Line()
    {
    }
    
    Line(Dot _st, int _dir) {
        st = _st;
        dir = _dir;
    }
    
    std::vector<Segment> removedSegments(Variant v) const
    {
        std::vector<Segment> rs;
        
        for (int i = 0; i <= line_length[v]; i++) {
            if (i < line_length[v] || v == D5 || v == D4) {
                rs.push_back(Segment(st + direction[dir]*i, dir));
            }
            if (i > 0 || v == D5 || v == D4) {
                rs.push_back(Segment(st + direction[dir]*i, (dir + 4) % 8));
            }
        }
        
        return rs;
    }

    std::vector<Dot> requiredDots(Variant v) const
    {
        std::vector<Dot> r;
        
        for (int i = 0; i <= line_length[v]; i++) {
            Dot p = st + direction[dir] * i;
            r.push_back(p);
        }
        
        return r;
    }
};
std::string to_string(const Line &);
std::ostream& operator<<(std::ostream&, const Line&);

/**
 * Move represents a Morpion move (both line and the dot placed).
 */
 
class Move {
    int goedel_number;

public:
    Dot st;
    int dir;
    Dot dot;

    class ParseError {
        std::string s;
        
    public:
        ParseError(std::string _s) {
            s = _s;
        }
    };
    
    Move(std::string move_id)
    {
        std::stringstream ss;
        
        ss << move_id;
        
        char c;
        ss >> c; if (c != 'm') { throw ParseError("Expected 'm'"); }
        ss >> c; if (c != 'v') { throw ParseError("Expected 'm'"); }
        ss >> c; if (c != '_') { throw ParseError("Expected 'm'"); }
        
        int x, y, d, dx, dy;
        ss >> x;
        ss >> c; if (c != '_') { throw ParseError("Expected 'm'"); }
        ss >> y;
        ss >> c; if (c != '_') { throw ParseError("Expected 'm'"); }
        ss >> c; if (c != 'D') { throw ParseError("Expected 'D'"); }
        ss >> d;
        ss >> c; if (c != '@') { throw ParseError("Expected '@'"); }
        ss >> dx;
        ss >> c; if (c != '_') { throw ParseError("Expected 'm'"); }
        ss >> dy;
        
        st = Dot(x,y);
        dir = d;
        dot = Dot(dx,dy);
    }
    
    Move(Dot _st, int _dir, Dot _dot)
    {
        st = _st;
        dir = _dir;
        dot = _dot;
    }
    
    Move()
    {
    }
        
    int getGoedelNumber() const
    {
        return goedel_number;
    }
     
    void setGoedelNumber(int gn)
    {
        goedel_number = gn;
    }
        
    std::vector<Segment> removedSegments(Variant v) const
    {
        std::vector<Segment> rs;
        
        for (int i = 0; i <= line_length[v]; i++) {
            if (i < 4 || v == D5 || v == D4) {
                rs.push_back(Segment(st + direction[dir]*i, dir));
            }
            if (i > 0 || v == D5 || v == D4) {
                rs.push_back(Segment(st + direction[dir]*i, (dir + 4) % 8));
            }
        }
        
        return rs;
    }
    
    std::vector<Segment> placedSegments() const
    {
        std::vector<Segment> ps;
        
        for (int d = 0; d < 8; d++) {
            ps.push_back(Segment(dot, d));
        }
        
        return ps;
    }
    
    Dot placedDot() const
    {
        return dot;
    }
    
    std::vector<Dot> requiredDots(Variant v) const
    {
        std::vector<Dot> r;
        
        for (int i = 0; i <= line_length[v]; i++) {
            Dot p = st + direction[dir] * i;
            
            if (p != dot) r.push_back(p);
        }
        
        return r;
    }
    
    std::vector<Dot> coveredDots(Variant v) const
    {
        std::vector<Dot> r;
        
        for (int i = 0; i <= line_length[v]; i++) {
            Dot p = st + direction[dir] * i;
            r.push_back(p);
        }
        
        return r;
    }
    
    bool placesSegment(Dot p, int) const
    {
        return p.x == dot.x && p.y == dot.y;
    }
    
    bool requiresDot(const Dot p, const Variant v) const
    {
        return (p != dot) && (st == p || (st + direction[dir] == p) ||
                              (st + direction[dir] * 2 == p) ||
                              (st + direction[dir] * 3 == p) ||
                              (st + direction[dir] * 4 == p && (v == D5 || v == T5)));
    }
    
    bool removesSegment(Segment s, Variant v) const
    {
        Dot p = s.first;
        int d = s.second;
        
        if (d != dir && ((d + 4) % 8) != dir) return false;

        int ll = (v == D4 || v == T4) ? 4 : 5;
        bool vd = (v == D4 || v == D5);
                
        for (int i = 0; i < ll; i++) {
            Dot x = st + direction[dir] * i;
            
            if ((i < ll-1 || vd) && x == p && d == dir) return true;
            if ((i > 0 || vd) && x == p && ((d+4)%8) == dir) return true;
        }
        
        return false;
    }
    
    bool consistentWith(const Move& m, Variant v) const {
        if (dot == m.dot) return false;
        if (dir != m.dir) return true;
        for (int i = 0; i <= line_length[v]; i++) {
            if (st == m.st + direction[dir] * i) return false;
            if (m.st == st + direction[dir] * i) return false;
        }
        return true;
    }
};
std::string to_string(const Move &);
std::ostream& operator<<(std::ostream&, const Move&);

/**
 * Board represents Morpion board.
 *
 * The class provides a variety of methods to cut the board to the desired shape.
 * It gives methods to access all moves, lines, segments and dots of the board.
 * It gives methods to access moves by placed or required dots or segments.
 */

typedef std::vector<int> Octagon;

class Board
{
public:
    static const int max_w = 128;
    static const int max_h = 128;

    int max_pos;
    int pos_idx[Board::max_w][Board::max_h];

    int& posIdx(Dot v)
    {
        return pos_idx[v.x][v.y];
    }

private:
    Variant v;
    
    int rim;
    int w, h;
    Dot ref, cref;
    Octagon hull;
    bool has_dot[max_w][max_h];
    bool disallowed[max_w][max_h];

    void putDot(Dot);
    void putCross(Dot r);

    std::vector<Move> move_list;
    std::vector<Segment> segment_list;
    std::vector<Dot> dot_list;
    std::vector<Line> line_list;
    std::map<Segment, std::vector<Move> > removing_segment;
    std::map<Segment, std::vector<Move> > placing_segment;
    std::map<Dot, std::vector<Move> > placing_dot;
    std::map<Segment, std::vector<Line> > lines_using_segment;
    std::map<Dot, std::vector<Move> > using_dot;
    std::map<Dot, std::vector<Line> > lines_using_dot;
    
public:

    Board()
    {
        setWidth(16); setHeight(16);
        setRim(0);
        calculateCRef(); 
        setReference(getCRef());
        setVariant(T5);
        
        for (int x = 0; x < max_w; x++) {
            for (int y = 0; y < max_h; y++) {
                disallowed[x][y] = false;
            }
        }
    }
    
    void setVariant(Variant _v) { v = _v; }
    Variant getVariant() const { return v; }

    int bound()
    {
        return best_upper_bound[v];
    }

    /// true if dot is placed on board (either is on cross, or was placed by a move)
    bool hasDot(Dot) const;
    
    void setWidth(int _w) { w = _w; }
    void setHeight(int _h) { h = _h; }
    void setReference(Dot _ref) { ref = _ref; }
    void setRim(int _rim) { rim = _rim; }

    void clear();
    void putCross()
    {
        putCross(ref);
    }
    
/*
    void putShape(MorpionGame g)
    {
        MorpionGame::Position ps = g.ReferencePoint();
        int rx, ry;
        MorpionGame::CoordsOfPosition(g.ReferencePoint(),rx,ry);
        
        for (int x = 0; x < MorpionGame::SIZE; x++) {
            for (int y = 0; y < MorpionGame::SIZE; y++) {
                if (!g.has_dot[MorpionGame::PositionOfCoords(x,y)]) {
                    disallowDot(Dot(getReference().x - rx + x, getReference().y - ry + y));
                }
            }
        }
    }
*/
    
    void putRim();
    void setHull(Octagon lengths);
    void setHalfplaneHull(Octagon distances);
    
    int getWidth() const { return w; }
    int getHeight() const { return h; }
    int getMaxWidth() { return max_w; }
    int getMaxHeight() { return max_h; }
    Dot getReference() const { return ref; }
    int getRim() { return rim; }
    Octagon getHull() { return hull; }

    void disallowDot(Dot v) { 
        if (inBounds(v)) { disallowed[v.x][v.y] = true; }
    }
    void allowDot(Dot v) { 
        if (inBounds(v)) {disallowed[v.x][v.y] = false; }
    }
    
    bool infeasibleDot(const Dot v) const {
        return (!inBounds(v)) || disallowed[v.x][v.y];
    }


    void calculateCRef()
    {
        cref.x = (w - 3) / 2;
        cref.y = (h - 3) / 2;
    }

    void setCRef(Dot d) {
        cref = d;
        std::cout << "setCRef(" << d << ")"<< std::endl;
    }
    
    Dot getCRef() const
    {
        return cref;
    }
    
    bool inBounds(const Dot v) const
    {
        return (v.x >= 0 && v.y >= 0 && v.x < w && v.y < h);
    }

    bool isCrossInBounds() const
    {
        // TODO: check if all dots placed by cross are feasible
        if (getVariant() == T5 || getVariant() == D5) {
            return (ref.x - 3 >= 0 && ref.x + 6 < w && ref.y - 3 >= 0 && ref.y + 6 < h);        
        } else {
            return (ref.x - 2 >= 0 && ref.x + 4 < w && ref.y - 2 >= 0 && ref.y + 4 < h);        
        }
    }
    
    Move centerSymmetry(const Move &m) const
    {
        Move sm;
        
        sm.st = centerSymmetry(m.st) + direction[(m.dir + 4) % 8] * 4;
        sm.dir = m.dir;
        sm.dot = centerSymmetry(m.dot);
        sm.setGoedelNumber(-1);
        
        return sm;
    }
    
    Line centerSymmetry(const Line &l) const
    {
        Line sl;
        
        sl.st = centerSymmetry(l.st) + direction[(l.dir + 4) % 8] * 4;
        sl.dir = l.dir;
        
        return sl;
    }

    Dot centerSymmetry(const Dot &v) const
    {
        if (ref.x == 0 && ref.y == 0) {
            return cref * 2 + v * (-1);
        } else {
            if (getVariant() == D5 || getVariant() == T5) {
                return ref * 2 + Dot(3,3) + v * (-1);
            } else {
                return ref * 2 + Dot(2,2) + v * (-1);
            }
        }
    }

    void createMoveList();
    void createSegmentList();
    void createDotList();
    void createLineList();
    
    void precompute()
    {
        createSegmentList();
        createMoveList();
        createDotList();
        createLineList();
        
        removing_segment.clear();
        placing_segment.clear();
        placing_dot.clear();
        using_dot.clear();
        
        for (const Move& m: getMoveList()) {
            for (const Segment& s: m.removedSegments(v)) {
                removing_segment[s].push_back(m);
            }
            for (const Segment& s: m.placedSegments()) {
                placing_segment[s].push_back(m);
            }
            placing_dot[m.placedDot()].push_back(m);
            for (const Dot &d: m.requiredDots(v)) {
                using_dot[d].push_back(m);
            }
        }
        
        lines_using_segment.clear();        
        lines_using_dot.clear();
        for (const Line& l: line_list) {
            for (const Segment& s: l.removedSegments(v)) {
                lines_using_segment[s].push_back(l);
            }
            for (const Dot& d: l.requiredDots(v)) {
                lines_using_dot[d].push_back(l);
            }
        }
    }
    
    const std::vector<Move>& getMoveList() const { return move_list; }
    const std::vector<Segment>& getSegmentList() const { return segment_list; }
    const std::vector<Dot>& getDotList() const { return dot_list; }
    const std::vector<Line>& getLineList() const { return line_list; }
    const std::vector<Line>& getLinesUsingSegment(Segment s) { return lines_using_segment[s]; }
    const std::vector<Line>& getLinesUsingDot(Dot d) { return lines_using_dot[d]; }
    
    std::vector<Move>& getMovesRemovingSegment(Segment s) { return removing_segment[s]; }
    std::vector<Move>& getMovesPlacingSegment(Segment s) { return placing_segment[s]; }
    std::vector<Move>& getMovesPlacingDot(Dot d) { return placing_dot[d]; }
    std::vector<Move>& getMovesUsingDot(Dot d) { return using_dot[d]; }
    
    bool validSegment(const Segment &s) const
    {
        return !infeasibleDot(s.first);
    }
    
    bool validLine(const Line &l) const
    {
        int d = l.dir;
        
        Dot en = l.st + direction[d] * line_length[getVariant()];

        if (!inBounds(en)) return false;
                
        /*
         * check if any of the dots is disallowed
         */

        bool disallow = false;
        for (int i = 0; i <= line_length[getVariant()]; i++) {
            Dot tmp = l.st + direction[d] * i;
            
            if (infeasibleDot(tmp)) { disallow = true; }
        }
        if (disallow) return false;

        // TODO: check if any of the segments is used
        
        return true;
    }
    
    /// Computes the size of the boundary of the board
    int boundary()
    {
        int cnt = 0;
        
        for (int x = 0; x < getWidth(); x++) {
            for (int y = 0; y < getHeight(); y++) {
                if (!infeasibleDot(Dot(x,y))) {
                    for (int i = 0; i < 8; i++) {
                        if (infeasibleDot(Dot(x,y)+direction[i])) cnt++;
                    }
                }
            }
        }
        return cnt;
    }
    
    std::vector<Dot> getSide(int coef_x, int coef_y, bool max);
};

std::ostream& operator<<(std::ostream&, const Board&);
std::istream& operator>>(std::istream&, Board&);
 
#endif
