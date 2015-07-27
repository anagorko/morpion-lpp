#include "morpion_lpp.h"

void MorpionSolution::getGraph(std::string)
{
/*
    Agraph_t *g = agopen((char *) std::string("Solution").c_str(), Agdirected, NULL);
    
    std::map<std::string, Agnode_t*> nodes;
    
    // create nodes
    for (const auto& v: valuation) {
        try {
            Move m(v.first);
        
            for (const Dot& vec: m.requiredDots())
            {
                nodes["d_" + to_string(vec)] = agnode(g, (char *) ("d_" + to_string(vec)).c_str(), TRUE);
            }
            nodes["d_" + to_string(m.placedDot())] = agnode(g, (char *) ("d_" + to_string(m.placedDot())).c_str(), TRUE);
        } catch (Move::ParseError err) {
            std::cout << "Ignoring variable " << v.first << std::endl;
        }
    }
    
    // create edges
    for (const auto& v: valuation) {
        if (v.second <= 0.00000001) { continue; }
        
        try {
            Move m(v.first);
        
            // connect each required dot with the placed dot
            Dot to = m.placedDot();
            for (const Dot& from: m.requiredDots())
            {
                agedge(g, nodes[(char *) ("d_" + to_string(from)).c_str()], nodes[(char *) ("d_" + to_string(to)).c_str()], (char*) to_string(v.second).c_str(), TRUE);
            }
        } catch (Move::ParseError err) {
        }
    }
    
    FILE* out = fopen(filename.c_str(), "w+");
    agwrite(g, out);
    fclose(out);
*/
}    

//
// MorpionLPP::getLPP()
//

LPP* MorpionLPP::getLPP()
{
    setComment(gameId());

    // each possible move is a structural variable
    // our goal is to maximize sum over all variables

    for (const Move& m: b.getMoveList()) {        
        std::string v_name = to_string(m);

        setVariableBounds(v_name, 0.0, 1.0);
        getObjective().push_back(std::pair<std::string, double>(v_name, 1.0));
        
        // Constraint for exact problems.
        //
        //   mv_ variables are either 0 or 1 (i.e. the move is either played or not)

        if (getFlag("exact") || getFlag("binary_moves")) {
            setVariableBoolean(v_name, true);
        }
    }

    // each segment gives us constraints:
    // (1) if the segment is placed in the starting position
    //    (a) sum of weights of moves that place this segment <= 0
    //    (b) 1 - sum of weights of moves that remove this segment >= 0
    //        i.e. sum of weights of moves that remove this segment <= 1
    // (2) if the segment is not placed in the starting position
    //    (a) sum of weights of moves that place this segment <= 1
    //    (b) sum of weights of moves that place this segment
    //          - sum of weights of moves that remove this segment >= 0
            
    for (const Segment& s: b.getSegmentList()) {
        int x = s.first.x;
        int y = s.first.y;
        int d = s.second;

        std::string constr_name = "segment_" + to_string(x) + "_" + to_string(y) + "_" + to_string(d);

        if (b.hasDot(Dot(x,y))) {
            // (1a)
            {
                Constraint constr;
        
                for (Move& pl: b.getMovesPlacingSegment(s)) {
                    constr.addVariable(to_string(pl), 1.0);
                }
                constr.setName("r1a_" + constr_name);
                constr.setType(Constraint::LT);
                constr.setBound(0.0);
                addConstraint(constr);
            }
            // (1b)
            {
                Constraint constr;
        
                for (Move& rm: b.getMovesRemovingSegment(s)) {
                    constr.addVariable(to_string(rm), 1.0);
                }
                constr.setName("r1b_" + constr_name);
                constr.setType(Constraint::LT);
                constr.setBound(1.0);
                addConstraint(constr);
            }                        
        } else {
            // (2a)
            {
                Constraint constr;
        
                for (Move& pl: b.getMovesPlacingSegment(s)) {
                    constr.addVariable(to_string(pl), 1.0);
                }
                constr.setName("r2a_" + constr_name);
                constr.setType(Constraint::LT);
                constr.setBound(1.0);
                addConstraint(constr);
            }
            // (2b)
            {
                Constraint constr;

                for (Move& pl: b.getMovesPlacingSegment(s)) {
                    constr.addVariable(to_string(pl), 1.0);
                }
                for (Move& rm: b.getMovesRemovingSegment(s)) {
                    constr.addVariable(to_string(rm), -1.0);
                }
                constr.setName("r2b_" + constr_name);
                constr.setType(Constraint::GT);
                constr.setBound(0.0);      
                    
                addConstraint(constr);          
            }
        }        
    }

    // EXTRA CONSTRAINTS:
    //
    // for each move m:
    //   for each dot d required by m:
    //     for each move n placing d that is consistent with m:
    //       weight_m <= sum of weights of n's

    if (getFlag("extra")) {
        for (const Move& m: b.getMoveList()) {
            std::string constr_name = "_" + to_string(m);
        
            for (const Dot& d: m.requiredDots(b.getVariant())) {
                if (b.hasDot(d)) continue;
                
                Constraint constr;
                
                for (const Move& n: b.getMovesPlacingSegment(Segment(d,0))) {
                    if (m.consistentWith(n,b.getVariant())) {
                        constr.addVariable(to_string(n), -1.0);
                    }
                }
                constr.addVariable(to_string(m), 1.0);
                
                constr.setName("extra_" + to_string(d) + constr_name);
                constr.setType(Constraint::LT);
                constr.setBound(0.0);
                
                addConstraint(constr);
            }
        }
    }
    
    // Disallow cycles of length 3 from the solution.
    // Improves fuzzy solutions
    
    if (getFlag("short-cycles")) {         
        for (const Segment& s: b.getSegmentList()) {
        
            int d = s.second;
            
            if (d % 2 == 1) continue;
            
            addConstraints(createCycleConstraints(s, d + 2, d + 5));
            addConstraints(createCycleConstraints(s, d + 6, d + 1));                
        }            
    }
    
    // Constraints for acyclic problems (explanation makes sense for exact problems):
    //
    // OBSOLETE
    //
    // 1. Each move mv_* has accompanying order variable order_*
    // 2. Move that is not picked has always order 0, 
    //    move that is picked has order between 1 and 675
    // 
    // It is accomplished with the condition:
    //      mv_ <= order_ <= 675 * mv_
    //
    // 3. Assume that mv_1 removes segment s and mv_2 places segment s (mv_1 != mv_2). 
    //    Then
    //      order_1 + (1 - mv_1) * 1000 >= order_2 + 1
    //
    //    The (1-mv_1)*1000 term assures that the condition is not enforced
    //      for moves that are not picked (i.e. have mv_1 = 0)
            
    if (getFlag("acyclic")) {
        throw std::string("not implemented");
    }

    // Constraints for symmetric problems.
    //
    // Moves that are symmetric by central symmetry
    //   with center at ref + (1.5,1.5) have to have equal weights
    
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
    }

    // dot-acyclic - better implementation of acyclic problems

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
    
    if (getFlag("dot-acyclic")) {
        for (int x = 0; x < b.getWidth(); x++) {
            for (int y= 0; y < b.getHeight(); y++) {
                if (b.infeasibleDot(Dot(x,y)) || b.hasDot(Dot(x,y))) continue;
                
                setVariableBounds("dot_" + to_string(Dot(x,y)), 0, b.bound());

                if (getFlag("symmetric")) {
                    Constraint c;
                    
                    if (b.infeasibleDot(b.centerSymmetry(Dot(x,y)))) continue;
                    
                    c.setName("dotsym_");
                    c.addVariable("dot_" + to_string(Dot(x,y)), 1.0);
                    c.addVariable("dot_" + to_string(b.centerSymmetry(Dot(x,y))), -1.0);
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
                
                c.setName("dot_");
                c.addVariable("dot_" + to_string(m.placedDot()), 1.0);
                c.addVariable("dot_" + to_string(rq), -1.0);
                c.addVariable(to_string(m), -b.bound());
                c.setBound(1 - b.bound());
                c.setType(Constraint::GT);
                
                addConstraint(c);
            }
        }      
    }

    // Constraints that enforce that the board (if it is rectangle) is 
    // the convex hull of the solution
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
    
    // Constraints that enforce that the board (if it is octagon) is 
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
            
    return this;
}


LPP* PlusPlusLPP::getLPP()
{
    setComment("++LPP " + gameId());
    
    // each line is a structural variable
    
    for (const Line& l: b.getLineList()) {        
        std::string v_name = to_string(l);

        setVariableBounds(v_name, 0.0, 1.0);
        getObjective().push_back(std::pair<std::string, double>(v_name, 1.0));        
    }
    
    // each dot is a structural variable

    for (const Dot& d: b.getDotList()) {
        std::string v_name = "dot_" + to_string(d);
        setVariableBounds(v_name, 0.0, 1.0);
  
        // Constraint for exact problems.
        //
        //   variables are either 0 or 1 (i.e. the move is either played or not)

        if (getFlag("exact")) {
            setVariableBoolean(v_name, true);
            
            setVariableOrd(v_name, 1000-abs(b.getCRef().x - d.x) - abs(b.getCRef().y - d.y));
        }
    }
    
    // constraints:
    //
    //   (1) for each segment, sum of weights of moves that use this segment is <= 1
    //   (2) for each segment, 
    //          dot_weight >= sum of weights of lines that use this segment
    //
    //   (3) sum of dot weights = 36 + sum of line weights
    //
    //   (4) dot_weight for dots from the cross is equal to 1

    //   (1) for each segment, sum of weights of moves that use this segment is <= 1
    //       (IT IS REDUNDANT TO (2) because dot_weight <= 1):
    //   (2) for each segment, 
    //          dot_weight >= sum of weights of moves that remove this segment
    
    for (const Segment& s: b.getSegmentList()) {
        Constraint c;
        c.setName("used_segment_" + to_string(s));
        
        for (const Line& l: b.getLinesUsingSegment(s)) {                        
            c.addVariable(to_string(l), -1.0);
        }
                
        c.addVariable("dot_" + to_string(s.first), 1.0);
        c.setBound(0.0);
        c.setType(Constraint::GT);            
        addConstraint(c);
    }
    
    //   (3) sum of dot weights <= 36 + sum of line weights
    {
        Constraint c;
        c.setName("startingdots");
        for (const Dot& d: b.getDotList()) {
            c.addVariable("dot_" + to_string(d), 1.0);
        }
        for (const Line& l: b.getLineList()) {
            c.addVariable(to_string(l), -1.0);
        }
        c.setBound(36.0);
        c.setType(Constraint::EQ);
        addConstraint(c);
    }
        
    //   (4) dot_weight for dots from the cross is equal to 1
    for (const Dot& d: b.getDotList()) {
        if (b.hasDot(d)) {
            Constraint c;
                        
            c.setName("cross_" + to_string(d));
            c.addVariable("dot_" + to_string(d), 1.0);
            c.setBound(1.0);
            c.setType(Constraint::EQ);            
            addConstraint(c);
        }
    }
    
    // symmetric solutions
    if (getFlag("symmetric")) {
        for (const Line&l: b.getLineList()) {
            Line sl = b.centerSymmetry(l);
            
            Constraint c;
            
            c.addVariable(to_string(l), 1.0);
            c.addVariable(to_string(sl), -1.0);
            c.setType(Constraint::EQ);
            c.setBound(0.0);
            c.setName("sym_" + to_string(l));
            
            addConstraint(c);
        }

        for (const Dot&d: b.getDotList()) {
            Dot sd = b.centerSymmetry(d);
            
            Constraint c;
            
            c.addVariable("dot_"+to_string(d), 1.0);
            c.addVariable("dot_"+to_string(sd), -1.0);
            c.setType(Constraint::EQ);
            c.setBound(0.0);
            c.setName("dotsym_" + to_string(d));
            
            addConstraint(c);
        }
    }

    // exact hull
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

    // dots that are not placed have dot_ variable equal to 0
    // this is important for --hull option
    // and possibly for other reasons as well
    
    for (const Dot& d: b.getDotList())
    {
        Constraint c;
        
        c.setName("dotline_" + to_string(d));
        
        c.addVariable("dot_" + to_string(d), 1.0);
        
        for (const Line& l: b.getLinesUsingDot(d)) {
            c.addVariable(to_string(l), -1.0);
        }
        c.setType(Constraint::LT);
        c.setBound(0.0);
        addConstraint(c);                    
    }

    // extra constraints that remove small gaps from the solution
    // there is always an optimal solution without 1-segment gaps

    if (getFlag("extra")) {
        for (const Line& l: b.getLineList()) {
        {
            Constraint c;
            c.setName("smallgaps1_" + to_string(l));
            c.addVariable(to_string(l), 1.0);
            
            Line ol;
            
            if (b.getVariant() == T5) {
                ol = Line(l.st + direction[l.dir]*5, l.dir); if (b.validLine(ol)) { c.addVariable(to_string(ol), 1.0); }
            } else {
                ol = Line(l.st + direction[l.dir]*6, l.dir); if (b.validLine(ol)) { c.addVariable(to_string(ol), 1.0); }
            }
            
            c.setType(Constraint::LT);
            c.setBound(1.0);
            addConstraint(c);
        }
        {
            Constraint c;
            c.setName("smallgaps2_" + to_string(l));
            c.addVariable(to_string(l), 1.0);
            
            Line ol;
            
            if (b.getVariant() == T5) {
                ol = Line(l.st + direction[l.dir]*(-5), l.dir); if (b.validLine(ol)) { c.addVariable(to_string(ol), 1.0); }
            } else {
                ol = Line(l.st + direction[l.dir]*(-6), l.dir); if (b.validLine(ol)) { c.addVariable(to_string(ol), 1.0); }
            }
            
            c.setType(Constraint::LT);
            c.setBound(1.0);
            addConstraint(c);
        }


        }
    }
    
    return this;
}

//
// Computes value of c(n) - minimal number of dots covered by n lines (with negative sign)
//
// If value is smaller than -36-n, then n is an upper bound for ++ version of the game.
//

void CnLPP::setN(int _n)
{
    n = _n;
}

LPP* CnLPP::getLPP()
{
    setComment("C(n) LPP " + gameId());
    
    // each line is a structural variable
    
    for (const Line& l: b.getLineList()) {        
        std::string v_name = to_string(l);

        setVariableBounds(v_name, 0.0, 1.0);
    }
    
    // each dot is a structural variable

    for (const Dot& d: b.getDotList()) {
        std::string v_name = "dot_" + to_string(d);
        setVariableBounds(v_name, 0.0, 1.0);

        getObjective().push_back(std::pair<std::string, double>(v_name, -1.0));

        // Constraint for exact problems.
        //
        //   variables are either 0 or 1 (i.e. the move is either played or not)

        if (getFlag("exact")) {
            setVariableBoolean(v_name, true);
        }
    }
    
    // constraints:
    //
    //   (1) for each segment, sum of weights of moves that use this segment is <= 1
    //   (2) for each segment, 
    //          dot_weight >= sum of weights of lines that use this segment
    //
    //   (3) sum of line weights = n
    //
    //   (4) dot_weight for dots from the cross is equal to 1

    //   (1) for each segment, sum of weights of moves that use this segment is <= 1
    //       (IT IS REDUNDANT TO (2) because dot_weight <= 1):
    //   (2) for each segment, 
    //          dot_weight >= sum of weights of moves that remove this segment

    for (const Segment& s: b.getSegmentList()) {
        Constraint c;
        c.setName("used_segment_" + to_string(s));
        
        for (const Line& l: b.getLinesUsingSegment(s)) {                        
            c.addVariable(to_string(l), -1.0);
        }
                
        c.addVariable("dot_" + to_string(s.first), 1.0);
        c.setBound(0.0);
        c.setType(Constraint::GT);            
        addConstraint(c);
    }
    
    // (3) sum of line weights = n
    {
        Constraint c;
        c.setName("startingdots");
        for (const Line& l: b.getLineList()) {
            c.addVariable(to_string(l), 1.0);
        }
        c.setBound(n);
        c.setType(Constraint::EQ);
        addConstraint(c);
    }
        
    //   (4) dot_weight for dots from the cross is equal to 1
    for (const Dot& d: b.getDotList()) {
        if (b.hasDot(d)) {
            Constraint c;
                        
            c.setName("cross_" + to_string(d));
            c.addVariable("dot_" + to_string(d), 1.0);
            c.setBound(1.0);
            c.setType(Constraint::EQ);            
            addConstraint(c);
        }
    }
    
    // symmetric solutions
    if (getFlag("symmetric")) {
        for (const Line&l: b.getLineList()) {
            Line sl = b.centerSymmetry(l);
            
            Constraint c;
            
            c.addVariable(to_string(l), 1.0);
            c.addVariable(to_string(sl), -1.0);
            c.setType(Constraint::EQ);
            c.setBound(0.0);
            c.setName("sym_" + to_string(l));
            
            addConstraint(c);
        }
    }

    return this;
}

