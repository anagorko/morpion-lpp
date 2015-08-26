#include "lpp.h"

Solution LPP::solve_gurobi()
{
    Solution sol;
    
    return sol;
}

Solution LPP::solve_glpk()
{
/*
    glp_prob *lp = glp_create_prob();
    
    glp_set_prob_name(lp, getComment().c_str());
    
    glp_set_obj_dir(lp, GLP_MAX);

    int n = 1;
    bool continuous = true;
    
    // add variables (columns)
    
    glp_add_cols(lp, vars.size());
    for (auto &v: vars)
    {            
        v.second.setGoedelNumber(n);
        glp_set_col_name(lp, n, v.first.c_str());
        glp_set_col_bnds(lp, n, GLP_DB, v.second.getLowerBound(), v.second.getUpperBound());           

        if (v.second.getBoolean()) {
            glp_set_col_kind(lp, n, GLP_BV);
            continuous = false;
        }
        n++;
    }
    
    // add objective coefficients
    for (const std::pair<std::string, double>& v: getObjective())
    {
        glp_set_obj_coef(lp, vars[v.first].getGoedelNumber(), v.second);
    }
    
    // add constraints (rows)
    glp_add_rows(lp, constr.size());
    
    n = 1;
    for (const Constraint& c: constr)
    {
        glp_set_row_name(lp, n, c.getName().c_str());

        int l = 1;
        int ind[c.getLinearCombination().size() + 1];
        double val[c.getLinearCombination().size() + 1];
        
        for (auto& iv: c.getLinearCombination())
        {
            ind[l] = vars[iv.first].getGoedelNumber();
            val[l] = iv.second;
                            
            l++;
        }
        
        glp_set_mat_row(lp, n, l-1, ind, val);
        switch (c.getType()) {
        case Constraint::LT: glp_set_row_bnds(lp, n, GLP_UP, 0.0, c.getBound()); break;
        case Constraint::GT: glp_set_row_bnds(lp, n, GLP_LO, c.getBound(), 0.0); break;
        case Constraint::EQ: glp_set_row_bnds(lp, n, GLP_FX, c.getBound(), c.getBound()); break;
        }
        n++;
    }
    
    Solution sol;
    
    if (continuous) {
        glp_simplex(lp, NULL);
            
        sol.obj = glp_get_obj_val(lp);
        
        switch (glp_get_status(lp)) {
        case GLP_OPT: sol.status = Solution::OPT; break;
        case GLP_FEAS: sol.status = Solution::FEAS; break;
        case GLP_INFEAS: sol.status = Solution::INFEAS; break;
        case GLP_NOFEAS: sol.status = Solution::NOFEAS; break;
        case GLP_UNBND: sol.status = Solution::UNBND; break;
        case GLP_UNDEF: sol.status = Solution::UNDEF; break;
        }
        
        for (const auto& v: vars) {
            sol.valuation[v.first] = glp_get_col_prim(lp, v.second.getGoedelNumber());
        }
    } else {
        glp_iocp *parm = new glp_iocp;
        
        glp_init_iocp(parm);
        parm -> presolve = GLP_ON;
        
        glp_intopt(lp, parm);
        
        sol.obj = glp_mip_obj_val(lp);

        switch (glp_mip_status(lp)) {
        case GLP_OPT: sol.status = Solution::OPT; break;
        case GLP_FEAS: sol.status = Solution::FEAS; break;
        case GLP_INFEAS: sol.status = Solution::INFEAS; break;
        case GLP_NOFEAS: sol.status = Solution::NOFEAS; break;
        case GLP_UNBND: sol.status = Solution::UNBND; break;
        case GLP_UNDEF: sol.status = Solution::UNDEF; break;
        }
        
        for (const auto& v: vars) {
            sol.valuation[v.first] = glp_mip_col_val(lp, v.second.getGoedelNumber());
        }
    }
    
    return sol;
*/
}

std::ostream& operator<<(std::ostream& os, const Solution& sol)
{
    os << "Objective value: " << sol.obj << std::endl;            
    
    return os;
}

std::ostream& operator<<(std::ostream& os, const LinearCombination& lc)
{
    int ll = 1;
    
    for (const std::pair<std::string, double>& v: lc) {
        if (v.second == 0.0) continue;
        
        std::string out = (v.second > 0 ? "+" : "") + (v.second != 1.0 ? std::to_string(v.second) : "") + " " + v.first + " ";
        
        if (ll + out.length() > 75) {
            os << std::endl << " ";
            ll = 1;
        }
                
        os << out;
        
        ll += out.length();
    }
    
    return os;
}

std::ostream& operator<<(std::ostream& os, LPP& lpp)
{
    // collect variables
    lpp.addVariables(lpp.getObjective());
    for (const Constraint& c: lpp.constr) {
        lpp.addVariables(c.getLinearCombination());
    }
    
    // comment
    
    os << "\\*" << lpp.getComment() << "*\\" << std::endl;

    // objective
    
    os << std::endl 
       << "Maximize" << std::endl
       << " obj: ";
    
    os << lpp.getObjective();
    
    // constraints

    os << std::endl
       << std::endl
       << "Subject To" << std::endl;

    for (const Constraint& c: lpp.constr) {
        os << c << std::endl << std::endl;
        
    }
    
    // variable bounds
        
    os << std::endl
       << std::endl
       << "Bounds" << std::endl;
       
    for (auto vs: lpp.vars) {
        Variable& v = vs.second;
        
        os << " " << v.getLowerBound() << " <= " << v.getName() << " <= " << v.getUpperBound() << std::endl;
    }

    // list boolean variables
    
    os << std::endl
       << std::endl
       << "Generals" << std::endl;
    
    for (auto vs: lpp.vars) {
        Variable& v = vs.second;
        
        if (v.getBoolean()) {
            os << " " << v.getName() << std::endl;
        }
    }
    return os;
}

std::ostream& operator<<(std::ostream& os, const Constraint &c)
{
    os << c.getName() << ": ";
    os << c.getLinearCombination();
    
    switch (c.getType()) {
    case Constraint::LT: os << " <= "; break;
    case Constraint::GT: os << " >= "; break;
    case Constraint::EQ: os << " = "; break;
    }
    
    os << c.getBound();

    return os;
}

void LPP::saveOrd(std::ostream& os)
{
    for (auto vs: vars) {
        Variable& v = vs.second;
        
        if (v.getBoolean()) {
            os << " " << v.getName() << " " << v.getOrd() << std::endl;
        }
    }
}

