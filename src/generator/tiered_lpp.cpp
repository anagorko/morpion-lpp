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
            setVariableBounds(tier(t) + v_name, 0.0, 1.0);

            if (getFlag("exact")) {
//                setVariableBoolean(tier(t) + v_name, true);
            }
        }
    }

    // each dot is structural variable
    for (const Dot &d: b.getDotList()) {
        if (b.hasDot(d)) continue;
        
        setVariableBounds("dot_" + to_string(d), 0.0, 1.0);

        getObjective().push_back(std::pair<std::string, double>("dot_" + to_string(d), 1.0)); 

        if (getFlag("exact")) {
            setVariableBoolean("dot_" + to_string(d), true);
        }
        
    }
    
    // constraints:
    //
    //
    //  (1) for each dot d:
    //      d = sum of moves that place d over all tiers
    //
    //  (2) for each segment s:
    //          for each tier t:
    //              initial s value - weights of moves that remove but not place s (tiers <= t) +
    //                  weights of moves that place but not remove s (tiers < t) >= 0
    //
    //  (3) for each tier t > 0:
    //          sum of weights of all moves_t >= 1
    
    // (3)
    /*
    for (int t = 1; t < tiers; t++) {
        Constraint c;
        c.setName("fulltier_" + tier(t));
        
        for (const Move&m : b.getMoveList()) {
            c.addVariable(tier(t) + to_string(m), 1.0);
        }

        c.setType(Constraint::GT);
        c.setBound(1.0);
        addConstraint(c);
    }
    */
            
    // (2)
    
    for (const Segment &s: b.getSegmentList()) {
        for (int t = 0; t < tiers; t++) {
            Constraint c;
            c.setName("sgm_" + tier(t) + to_string(s));
            
            for (const Move&m: b.getMovesRemovingSegment(s)) {
                if (m.placesSegment(s.first, s.second)) continue;
                for (int i = 0; i <= t; i++) {
                    c.addVariable(tier(i) + to_string(m), -1.0);
                }
            }
            
            for (const Move&m: b.getMovesPlacingSegment(s)) {
                if (m.removesSegment(s, b.getVariant())) continue;
                for (int i = 0; i < t; i++) {
                    c.addVariable(tier(i) + to_string(m), 1.0);
                }
            }
            
            c.setType(Constraint::GT);
            if (b.hasDot(s.first)) {
                c.setBound(-1.0);
            } else {
                c.setBound(0);
            }
            addConstraint(c);
        }
    }
        
    // (1)
    
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
        for (int t = 0; t < tiers; t++) {
            for (const Move&m: b.getMoveList()) {
                Move sm = b.centerSymmetry(m);
                
                Constraint c;
                
                c.addVariable(tier(t) + to_string(m), 1.0);
                c.addVariable(tier(t) + to_string(sm), -1.0);
                c.setType(Constraint::EQ);
                c.setBound(0.0);
                c.setName("sym_" + to_string(m));
                
                addConstraint(c);
            }
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
    
    return this;
}

