#include "morpion_lpp.h"

// Potential LPP

LPP* PotentialLPP::getLPP()
{
    setComment("PotentialLPP " + gameId());
    
    // each move is a structural variable
    
    for (const Move& l: b.getMoveList()) {        
        std::string v_name = to_string(l);

        setVariableBounds(v_name, 0.0, 1.0);

        if (getFlag("exact")) {
            setVariableBoolean(v_name, true);
            setVariableOrd(v_name, 0);
        }
    }
    
    // each dot is a structural variable

    for (const Dot& d: b.getDotList()) {
        std::string v_name = "dot_" + to_string(d);

        if (!b.hasDot(d)) {
            getObjective().push_back(std::pair<std::string, double>(v_name, 1.0)); 
        }
            
        // dots are the only boolean variables        
        if (getFlag("exact")) {
            setVariableBoolean(v_name, true);
            setVariableOrd(v_name, 1);
        }

        // dots that are placed on board have dot variable equal to 1
        if (b.hasDot(d)) {
            setVariableBounds(v_name, 1.0, 1.0);
        } else {
            setVariableBounds(v_name, 0.0, 1.0);
        }
    }
                
    // constraints:
    //
    //   (1) dot at beginning of segment - moves removing segment starting from dot >= 0
    //
    //   (2) dot == sum of weights of moves that place dot
    //
    //   (3) sum of weights of dots - sum of weights of moves <= 36

    // (1)
    
    for (const Segment& s: b.getSegmentList()) {
        Constraint c;
        c.setName("sgm_" + to_string(s));        

        c.addVariable("dot_" + to_string(s.first), 1.0);
        for (const Move& m: b.getMovesRemovingSegment(s)) {
            c.addVariable(to_string(m), -1.0);
        }
        c.setType(Constraint::GT);
        c.setBound(0.0);
        addConstraint(c);        
    }
    
    // (2)
    
    for (const Dot &d: b.getDotList()) {
        if (b.hasDot(d)) continue;
        
        Constraint c;
        c.setName("dotmv_" + to_string(d));
        
        for (const Move &m: b.getMovesPlacingDot(d)) {
            c.addVariable(to_string(m), -1.0);
        }
        c.addVariable("dot_" + to_string(d), 1.0);
        c.setBound(0.0);
        c.setType(Constraint::EQ);
        addConstraint(c);
    }
    
    // (3)

    {
    Constraint c;
    c.setName("dots_moves");
    
    for (const Dot &d: b.getDotList()) {
        c.addVariable("dot_"+to_string(d), 1.0);
    }    
    for (const Move &m: b.getMoveList()) {
        c.addVariable(to_string(m), -1.0);
    }
    c.setBound(36.0);
    c.setType(Constraint::LT);
    addConstraint(c);
    
    }

    // dots that are not placed have dot_ variable equal to 0
    // this is important for --hull option
    // use this not only for dot-acyclic problems
    
    for (const Dot& d: b.getDotList())
    {
        if (b.hasDot(d)) continue;
        
        Constraint c;
        
        c.setName("dotmove_" + to_string(d));
        
        c.addVariable("dot_" + to_string(d), 1.0);
        
        for (const Move& m: b.getMovesPlacingDot(d)) {
            c.addVariable(to_string(m), -b.bound());
        }
        c.setType(Constraint::LT);
        c.setBound(0.0);
        addConstraint(c);                    
    }

    // Constraints that enforce that the board (if it is an octagon) is 
    // the convex hull of the solution

    if (getFlag("hull")) {
        // right side
        addConstraint(getSideConstraint(1,0,true));        
        // left side
        addConstraint(getSideConstraint(1,0,false));
        // lower right side
        addConstraint(getSideConstraint(1,-1,false));
        // upper left side
        addConstraint(getSideConstraint(1,-1,true));
        // top side
        addConstraint(getSideConstraint(0,1,true));
        // bottom side
        addConstraint(getSideConstraint(0,1,false));
        // lower left side
        addConstraint(getSideConstraint(1,1,false));
        // upper right side
        addConstraint(getSideConstraint(1,1,true));
    }

    if (getFlag("rhull")) {
        // right side
        addConstraint(getSideConstraint(1,0,true));        
        // left side
        addConstraint(getSideConstraint(1,0,false));
        // top side
        addConstraint(getSideConstraint(0,1,true));
        // bottom side
        addConstraint(getSideConstraint(0,1,false));
    }

    if (getFlag("rside")) {
        // right side
        addConstraint(getSideConstraint(1,0,true));        
    }
    
    if (getFlag("symmetric")) {
        for (const Move&m: b.getMoveList()) {
            Move sm = b.centerSymmetry(m);
            
            Constraint c;
            
            c.addVariable(to_string(m), 1.0);
            c.addVariable(to_string(sm), -1.0);
            c.setType(Constraint::EQ);
            c.setBound(0.0);
            c.setName("sym_" + to_string(m));
            
            addConstraint(c);
        }

        for (const Dot& d: b.getDotList()) {
            std::string v_name = "symdot_" + to_string(d);

            Dot sd = b.centerSymmetry(d);
            
            if (b.hasDot(d) || b.hasDot(sd)) {
                continue;
            }
            
            Constraint c;
            
            c.addVariable("dot_" + to_string(d), 1.0);
            c.addVariable("dot_" + to_string(sd), -1.0);
            c.setType(Constraint::EQ);
            c.setBound(0.0);
            c.setName("symdot_" + to_string(d));
            
            addConstraint(c);
        }
    }
    
    // create only acyclic solutions
    
    if (getFlag("dot-acyclic")) {
        for (int x = 0; x < b.getWidth(); x++) {
            for (int y= 0; y < b.getHeight(); y++) {
                if (b.infeasibleDot(Dot(x,y)) || b.hasDot(Dot(x,y))) continue;
                
                setVariableBounds("ord_" + to_string(Dot(x,y)), 0, b.bound());

                if (getFlag("symmetric")) {
                    Constraint c;
                    
                    if (b.infeasibleDot(b.centerSymmetry(Dot(x,y)))) continue;
                    
                    c.setName("ordsym_");
                    c.addVariable("ord_" + to_string(Dot(x,y)), 1.0);
                    c.addVariable("ord_" + to_string(b.centerSymmetry(Dot(x,y))), -1.0);
                    c.setType(Constraint::EQ);
                    c.setBound(0.0);
                    addConstraint(c);
                }                
            }
        }
        
        for (const Move& m: b.getMoveList()) {
            for (const Dot& rq: m.requiredDots(b.getVariant())) {
                if (b.hasDot(rq)) continue;
                
                Constraint c;
                
                // dot_placed >= dot_required + 1 - (1 - m) * bound
                // i.e. dot_placed - dot_required - bound * m >= 1 - bound
                
                c.setName("ord_" + to_string(m) + "_" + to_string(rq));
                c.addVariable("ord_" + to_string(m.placedDot()), 1.0);
                c.addVariable("ord_" + to_string(rq), -1.0);
                c.addVariable(to_string(m), -b.bound());
                c.setBound(1 - b.bound());
                c.setType(Constraint::GT);
                
                addConstraint(c);
            }
        }      
    }

    return this;
}

