#include "morpion_lpp.h"

// Tiered Morpion LPP

LPP* TieredLPP::getLPP()
{
    int tiers = getValue("tiered") + 1;
    
    std::cout << "Tiers: " << tiers << std::endl;
    
    setComment("Tiered Morpion LPP " + gameId());
    
    // each move is a structural variable
    
    for (const Move& l: b.getMoveList()) {        
        std::string v_name = to_string(l);

        for (int i = 0; i < tiers; i++) {
            setVariableBounds("t" + to_string(i) + "_" + v_name, 0.0, 1.0);
        }
    }
    
    // each dot is a structural variable

    for (const Dot& d: b.getDotList()) {
        std::string v_name = "dot_" + to_string(d);

        for (int i = 0; i < tiers; i++) {
            std::string tier_v_name = "t" + to_string(i) + "_" + v_name;
            
            
            // dots are the only boolean variables        
            if (getFlag("exact") && i == tiers-1) {
                setVariableBoolean(tier_v_name, true);
            }

        // dots that are placed on board have dot variable equal to 1
            if (b.hasDot(d)) {
                setVariableBounds(tier_v_name, 1.0, 1.0);
            } else {
                if (i > 0) {
                    setVariableBounds(tier_v_name, 0.0, 1.0);
                } else {
                    setVariableBounds(tier_v_name, 0.0, 0.0);
                }
            }
        }

        if (!b.hasDot(d)) {
            getObjective().push_back(std::pair<std::string, double>("t" + to_string(tiers-1) + "_" + v_name, 1.0)); 
        }
    }
                
    // constraints:
    //
    //   (1) for each tier tier:
    //          for each segment s with starting dot d:
    //              d_tier - sum t <= tier: sum of moves_t that remove but not place s >= 0
    //
    //   (2) for each tier t:
    //         for each dot d:
    //           d_t = d_{t-1} + sum of weights of moves_{t-1} that place the dot             
    //
    //   (3) sum of weights of dots_tier - (sum t < tier: sum of weights of moves_t) <= 36
    //
    //   (4) for tier t:
    //          for move m:
    //              for each required dot d:
    //                  m_t <= sum over moves n placing dots required by m: n_{t-1}

    // (1)
    
    for (int tier = 0; tier < tiers; tier++) {
        for (const Segment& s: b.getSegmentList()) {
            Constraint c;
            c.setName("sgm_t" + to_string(tier) + "_" + to_string(s));        

            c.addVariable("t" + to_string(tier) + "_dot_" + to_string(s.first), 1.0);
            
            for (const Move& m: b.getMovesRemovingSegment(s)) {
                for (int t = 0; t <= tier; t++) {
                    if (m.placesSegment(s.first, s.second) && t == tier) continue;
                
                    c.addVariable("t" + to_string(t) + "_" + to_string(m), -1.0);
                }
            }
            
            c.setType(Constraint::GT);
            c.setBound(0.0);
            addConstraint(c);        
        }
    }
    
    // (2)
    
    for (int t = 1; t < tiers; t++) {
        for (const Dot &d: b.getDotList()) {
            if (b.hasDot(d)) continue;
            
            Constraint c;
            c.setName("dotmv_" + to_string(t) + "_" + to_string(d));
            
            for (const Move &m: b.getMovesPlacingDot(d)) {
                c.addVariable("t" + to_string(t-1) + "_" + to_string(m), -1.0);
            }
            c.addVariable("t" + to_string(t-1) + "_dot_" + to_string(d), -1.0);
            c.addVariable("t" + to_string(t) + "_dot_" + to_string(d), 1.0);
            c.setBound(0.0);
            c.setType(Constraint::EQ);
            addConstraint(c);
        }
    }
        
    // (3)
/*
    for (int tier = 0; tier < tiers; tier++)
    {
        Constraint c;
        c.setName("dots_moves");
        
        for (const Dot &d: b.getDotList()) {
            c.addVariable("t" + to_string(tier) + "_dot_"+to_string(d), 1.0);
        }    
        for (const Move &m: b.getMoveList()) {
            for (int t = 0; t < tier; t++) {
                c.addVariable("t" + to_string(t) + "_" + to_string(m), -1.0);
            }
        }
        c.setBound(36.0);
        c.setType(Constraint::LT); // EQ?
        addConstraint(c);
    }
*/
    //   (4) for tier t:
    //          for move m:
    //              for each required dot d:
    //                  m_t <= sum over moves n placing dots required by m: n_{t-1}

    for (int t = 1; t < tiers; t++) {
        for (const Move& m: b.getMoveList()) {
            Constraint c;
            c.setName("nogaps_t" + to_string(t) + "_" + to_string(m));
            
            c.addVariable("t" + to_string(t) + "_" + to_string(m), 1.0);
            
            for (const Dot& d: m.requiredDots(b.getVariant())) {
                for (const Move& n: b.getMovesPlacingDot(d)) {
                    c.addVariable("t" + to_string(t-1) + "_" + to_string(n), -1.0);
                }
            }
            
            c.setBound(0.0);
            c.setType(Constraint::LT);
            addConstraint(c);
        }
    }

    // dots that are not placed have dot_ variable equal to 0
    // this is important for --hull option
    // use this not only for dot-acyclic problems

    /*    
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
    */
    
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
    
    return this;
}

