#include "morpion_lpp.h"

// Tiered Morpion LPP

std::string tier(int t)
{
    return "t" + to_string(t) + "_";
}

LPP* TieredLPP::getLPP()
{
    int tiers = getValue("tiered") + 1;
    
    std::cout << "Tiers: " << tiers << std::endl;
    std::cout << "Number of dots: " << b.getDotList().size() << std::endl;
    std::cout << "Number of segments: " << b.getSegmentList().size() << std::endl;
    std::cout << "Number of moves: " << b.getMoveList().size() << std::endl;
    
    setComment("Tiered Morpion LPP " + gameId());
    
    // each move is a structural variable
    
    for (const Move& m: b.getMoveList()) {  
        if (b.hasDot(m.placedDot())) continue;
              
        std::string v_name = to_string(m);

        for (int t = 0; t < tiers; t++) {
            if (t == tiers - 1) {
                setVariableBounds(tier(t) + v_name, 0.0, 0.0);
            } else {
                setVariableBounds(tier(t) + v_name, 0.0, 1.0);
            }
            
            getObjective().push_back(std::pair<std::string, double>(tier(t) + v_name, 1.0)); 
        }
    }
    
    // each segment is a structural variable

    for (const Segment& s: b.getSegmentList()) {
        if (b.hasDot(s.first)) continue;
        
        // tier 0
        
        if (b.hasDot(s.first)) {
            setVariableBounds(tier(0) + "sgm" + to_string(s), 1.0, 1.0);
        } else {
            setVariableBounds(tier(0) + "sgm" + to_string(s), 0.0, 0.0);
        }
   
        // tiers > 0
             
        for (int t = 1; t < tiers; t++) {
            setVariableBounds(tier(t) + "sgm" + to_string(s), 0.0, 1.0);
        }
    }
    
    // each dot is structural variable
    for (const Dot &d: b.getDotList()) {
        if (b.hasDot(d)) continue;
        
        setVariableBounds("dot_" + to_string(d), 0.0, 1.0);

        // dots are only boolean variables
        if (getFlag("exact")) {
            setVariableBoolean("dot_" + to_string(d), true);
        }
        
    }
    
    // constraints:
    //
    //
    //  (1) for each segment s, tier t > 0:
    //      s'_t = s_{t-1} - weights of moves_{t-1} that remove but not place s 
    //      s_t = s'_t + weights of moves_{t-1} that place but not remove s
    //      s'_t >= 0
    //
    //      for tier = 0:
    //      s_t = 1 if starting dot is on board, 0 otherwise
    //
    //  (2) for each dot d:
    //      d = sum of moves that place d over all tiers
    //
    
    for (int t = 0; t < tiers; t++) {
        for (const Segment& s: b.getSegmentList()) {
            if (t > 0) {
                {
                Constraint c;
                c.setName("s1a_" + tier(t) + "sgmP" + to_string(s));
                
                c.addVariable(tier(t) + "sgmP" + to_string(s), 1.0);
                c.addVariable(tier(t-1) + "sgm" + to_string(s), -1.0);
                
                for (const Move& m: b.getMovesRemovingSegment(s)) {
                    if (m.placedDot() == s.first) continue;
                    
                    c.addVariable(tier(t-1) + to_string(m), 1.0); 
                }
                
                setVariableBounds(tier(t) + "sgmP" + to_string(s), 0.0, 1.0);
                             
                c.setType(Constraint::EQ);
                c.setBound(0.0);
                addConstraint(c);
                }

                {
                Constraint c;
                c.setName("s1a_" + tier(t) + "sgm" + to_string(s));
                
                c.addVariable(tier(t) + "sgm" + to_string(s), 1.0);
                c.addVariable(tier(t) + "sgmP" + to_string(s), -1.0);
                
                for (const Move& m: b.getMovesPlacingSegment(s)) {
                    if (m.removesSegment(s, b.getVariant())) continue;
                    
                    c.addVariable(tier(t-1) + to_string(m), -1.0); 
                }
                
                c.setType(Constraint::EQ);
                c.setBound(0.0);
                addConstraint(c);
                }

            } else {
            }
        }
    }
    
    // (2)
    
    for (const Dot &d: b.getDotList()) {
        if (b.hasDot(d)) continue;
        
        Constraint c;
        c.setName("dtb_" + to_string(d));
        
        for (const Move& m: b.getMovesPlacingDot(d)) {
            for (int t = 0; t < tiers; t++) {
                c.addVariable(tier(t) + to_string(m), 1.0);
            }
        }
        c.addVariable("dot_" + to_string(d), -1.0);
        c.setType(Constraint::EQ);
        c.setBound(0.0);
        addConstraint(c);
    }
        
    // Constraints that enforce that the board (if it is an octagon) is 
    // the convex hull of the solution
    /*
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
    */
    /*
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
    */

    /*
    if (getFlag("symmetric")) {
        for (int t = 0; t < tiers; t++) {
            for (const Move&m: b.getMoveList()) {
                Move sm = b.centerSymmetry(m);
                
                Constraint c;
                
                c.addVariable("t" + to_string(t) + "_" + to_string(m), 1.0);
                c.addVariable("t" + to_string(t) + "_" + to_string(sm), -1.0);
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
                
                c.addVariable("t" + to_string(t) + "_" + "dot_" + to_string(d), 1.0);
                c.addVariable("t" + to_string(t) + "_" + "dot_" + to_string(sd), -1.0);
                c.setType(Constraint::EQ);
                c.setBound(0.0);
                c.setName("symdot_" + to_string(d));
                
                addConstraint(c);
            }
        }
    }
    */
    
    return this;
}

