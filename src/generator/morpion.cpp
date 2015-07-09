#include "morpion_lpp.h"

std::ostream& operator<<(std::ostream& os, Variant v)
{
    switch (v) {
        case D5: os << "5D"; break;
        case T5: os << "5T"; break;
        case D4: os << "4D"; break;
        case T4: os << "4T"; break;
    }
    
    return os;
}

Dot operator+(const Dot &a, const Dot &b)
{
    return Dot(a.x + b.x, a.y + b.y);
}

Dot operator*(const Dot &v, int a) 
{
    return Dot(a * v.x, a * v.y);
}

bool operator!=(const Dot &v1, const Dot &v2)
{
    return v1.x != v2.x || v1.y != v2.y;
}

bool operator==(const Dot &v1, const Dot &v2)
{
    return v1.x == v2.x && v1.y == v2.y;
}

std::ostream& operator<<(std::ostream& os, Dot v)
{
    os << v.x << "," << v.y;
        
    return os;
}

void validate(boost::any& v, 
              std::vector<std::string> const& values,
              Dot*,
              int)
{
    po::validators::check_first_occurrence(v);
    std::string const& s = po::validators::get_single_string(values);

    std::stringstream ss(s);
    char c;
    Dot vec;
    
    ss >> vec.x;
    ss >> c;
    if (c != ',') {
        throw po::validation_error(po::validation_error::invalid_option_value);    
    }    
    ss >> vec.y;
    
    v = boost::any(vec);
}
    
void Board::clear()
{
    for (int x = 0; x < w; x++) {
        for (int y = 0; y < h; y++) {
            has_dot[x][y] = false;
        }
    }
}

void Board::putDot(Dot v)
{
    allowDot(v);
    has_dot[v.x][v.y] = true;
}

bool Board::hasDot(Dot v) const
{
    return has_dot[v.x][v.y];
}

void Board::putCross(Dot r)
{
//    clear();

    enum { RIGHT = 0, DOWN = 2, LEFT = 4, UP = 6 };
    
    static const int cross[] = {
        UP, RIGHT, DOWN, RIGHT, DOWN, LEFT, DOWN, LEFT, UP, LEFT, UP, RIGHT
    };

    Dot p = r;
    
    for(const int &i: cross) {
        for (int j = 0; j < line_length[getVariant()] - 1;j ++) {
            p = p + direction[i];
            putDot(p);
        }
    }
}

void Board::createDotList()
{
    dot_list.clear();
    
    for (int x = 0; x < getWidth(); x++) {
        for (int y= 0; y < getHeight(); y++) {
            if (infeasibleDot(Dot(x,y))) continue;
            
            dot_list.push_back(Dot(x,y));
        }
    }
}

void Board::createLineList()
{
    line_list.clear();
    
    for (int x = 0; x < w; x++) {
        for (int y = 0; y < h; y++) {
            Line l;
            
            l.st = Dot(x,y);

            for (int d = 0; d < 4; d++) {
                l.dir = d;
                
                Dot en = l.st + direction[d] * 4;

                if (!inBounds(en)) continue;
                
                /*
                 * check if any of the dots is disallowed
                 */

                bool disallow = false;
                for (int i = 0; i <= 4; i++) {
                    Dot tmp = l.st + direction[d] * i;
                    
                    if (infeasibleDot(tmp)) { disallow = true; }
                }
                if (disallow) continue;

                // TODO: check if any of the segments is used (for non-empty board)
                
                line_list.push_back(l);
            }
        }
    }          
}

void Board::createSegmentList()
{
    segment_list.clear();
    
    for (int x = 0; x < w; x++) {
        for (int y = 0; y < h; y++) {
            if (infeasibleDot(Dot(x,y))) continue;
            
            for (int d = 0; d < 8; d++) {
                segment_list.push_back(Segment(Dot(x,y), d));
            }
        }
    }
}

void Board::createMoveList()
{
    move_list.clear();

    max_pos = 0;

//    clear(); putCross(ref);
    
    int gn = 1;
    
    for (int x = 0; x < w; x++) {
        for (int y = 0; y < h; y++) {
            Move m;
            
            m.st = Dot(x,y);

            if (hasDot(m.st) || infeasibleDot(m.st))
                posIdx(m.st) = 0;
            else
                posIdx(m.st) = ++max_pos;

            for (int d = 0; d < 4; d++) {
                Dot en = m.st + direction[d] * line_length[getVariant()];
                
                if (!inBounds(en)) continue;
                
                /*
                 * check if any of the dots is disallowed
                 */

                bool disallow = false;
                for (int i = 0; i <= line_length[getVariant()]; i++) {
                    Dot tmp = m.st + direction[d] * i;
                    
                    if (infeasibleDot(tmp)) { disallow = true; }
                }
                if (disallow) continue;
                
                // TODO: check if any of the segments is used 

                for (int i = 0; i <= line_length[getVariant()]; i++) {
                    m.dot = m.st + direction[d] * i;
                    m.dir = d;
                    
                    if (hasDot(m.dot)) continue;                         
                    
                    m.setGoedelNumber(gn++);
                    move_list.push_back(m);
                }
            }
        }
    }
}

void Board::setHull(Octagon lengths)
{
    hull = lengths;
    
    int lowWidth = lengths[7]+lengths[0]+lengths[1];
    // this must be equal to int highWidth = lengths[3]+lengths[4]+lengths[5];
    int leftHeight = lengths[5]+lengths[6]+lengths[7];
    // this must be equal to int rightHeight = lengths[2]+length[3]+length[4];
    int leftBorder = (getWidth()-lowWidth)/2;
    int rightBorder = leftBorder+lowWidth;
    int lowBorder = (getHeight()-leftHeight)/2;
    int highBorder= lowBorder+leftHeight;
    
    std::cout << "Leftborder: " << leftBorder << "\n";
    std::cout << "Rightborder: " << rightBorder << "\n";
    std::cout << "Lowborder: " << lowBorder << "\n";
    std::cout << "Highborder: " << highBorder << "\n";
    std::cout << "Lengths: " << lengths[0] << " " << lengths[7] << "\n";
    
    for (int x = 0; x < getWidth(); x++) {
        for (int y = 0; y < getHeight(); y++) {
            if (x < leftBorder || x > rightBorder ||
                y < lowBorder || y > highBorder )
                disallowDot(Dot(x,y));
            else {
            if( x >= leftBorder && x <= leftBorder + lengths[5] && 
                y >= highBorder - lengths[5] && y <= highBorder &&    
                highBorder - leftBorder - lengths[5] < y - x )  
                 disallowDot(Dot(x,y));
            if( x >= leftBorder && x <= leftBorder + lengths[7] && 
                y >= lowBorder && y <= lowBorder + lengths[7] &&    
                lowBorder + leftBorder + lengths[7] > y + x )  
                 disallowDot(Dot(x,y));
            if( x >= rightBorder-lengths[1] && x <= rightBorder && 
                y >= lowBorder && y <= lowBorder + lengths[1] &&    
                lowBorder + lengths[1] - rightBorder > y - x )  
                 disallowDot(Dot(x,y));     
            if( x >= rightBorder-lengths[3] && x <= rightBorder && 
                y >= highBorder-lengths[3] && y <= highBorder &&    
                highBorder + rightBorder - lengths[3] < x + y )  
                 disallowDot(Dot(x,y));     
            }
        }
    }
}

void Board::putRim()
{
    bool t[getWidth()][getHeight()];
    for (int x = 0; x < getWidth(); x++) {
        for (int y = 0; y < getHeight(); y++) {
            t[x][y] = false;
        }
    }
            
    for (int x = 0; x < getWidth(); x++) {
        for (int y = 0; y < getHeight(); y++) {
            if (infeasibleDot(Dot(x,y))) continue;
            for (int i = -getRim(); i <= getRim(); i++) {
                for (int j = -getRim(); j <= getRim(); j++) {
                    t[x+i][y+j] = true;
                }
            }
        }
    }

    for (int x = 0; x < getWidth(); x++) {
        for (int y = 0; y < getHeight(); y++) {
            if (t[x][y]) {
                allowDot(Dot(x,y));
            } else {
                disallowDot(Dot(x,y));
            }
        }
    }
}

std::ostream& operator<<(std::ostream& os, const Board& b)
{
    os << "+"; 
    for (int i = 0; i < b.getWidth(); i++) { os << "-"; }
    os << "+" << std::endl;
    
    for (int y = 0; y < b.getHeight(); y++) {
        os << "|";
        for (int x = 0; x < b.getWidth(); x++) {
            os << (b.infeasibleDot(Dot(x,y)) ? " " : (b.hasDot(Dot(x,y)) ? (b.getReference().x == x && b.getReference().y == y ? "R" : "*") : "."));
        }
        os << "|" << std::endl;
    }

    os << "+"; 
    for (int i = 0; i < b.getWidth(); i++) { os << "-"; }
    os << "+" << std::endl;
    
    return os;
}

std::string to_string(const Dot &v)
{
    std::stringstream ss;
    ss << v;
    return ss.str();
}

std::ostream& operator<<(std::ostream& os, const Line& l)
{
    os << "ln_" << l.st.x << "_" << l.st.y << "_D" << l.dir;
    return os;
}

std::string to_string(const Line &l)
{
    std::stringstream ss;
    ss << l;
    return ss.str();
}

std::ostream& operator<<(std::ostream& os, const Move& mv)
{
    os << "mv_" << mv.st.x << "_" << mv.st.y << "_D" << mv.dir 
                << "@" << mv.dot.x << "_" << mv.dot.y;
    return os;
}

std::string to_string(const Move&mv)
{
    std::stringstream ss;
    ss << mv;
    return ss.str();
}

std::ostream& operator<<(std::ostream& os, const Segment& s)
{
    os << s.first.x << "_" << s.first.y << "_D" << s.second;
    return os;
}

std::string to_string(const Segment &s)
{
    std::stringstream ss;
    ss << s;
    return ss.str();
}

std::vector<Dot> Board::getSide(int coef_x, int coef_y, bool max)
{
    std::vector<Dot> v;
    
    int m = max ? -getWidth()-getHeight()-1 : getWidth()+getHeight()+1;
    
    for (int x = 0; x < getWidth(); x++) {
        for (int y = 0; y < getHeight(); y++) {
            if (infeasibleDot(Dot(x,y))) { continue; }
            
            if ((max && coef_x * x + coef_y * y > m) ||
                (!max && coef_x * x + coef_y * y < m)) {
                    m = coef_x * x + coef_y * y;
            }
        }
    }

    for (int x = 0; x < getWidth(); x++) {
        for (int y = 0; y < getHeight(); y++) {
            if (infeasibleDot(Dot(x,y))) { continue; }
            
            if (coef_x * x + coef_y * y == m) {
                v.push_back(Dot(x,y));
            }
        }
    }
    
    return v;
}
